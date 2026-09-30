/*
 * Gearlynx - Lynx Emulator
 * Copyright (C) 2025  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#include "libretro_link.h"
#include <string.h>

static u64 LinkCycle(u64 cycle, u64 origin, unsigned index)
{
    // Stagger power-on by one millisecond per player on the shared timeline
    return cycle - origin + (u64)index * (GLYNX_MASTER_CLOCK / 1000);
}

LibretroLink::LibretroLink(LibretroInstance* instances, unsigned count)
{
    m_instances = instances;
    m_count = count;
    m_save_path[0] = 0;
    memset(&m_runtime, 0, sizeof(m_runtime));

    for (unsigned i = 0; i < m_count; i++)
    {
        m_endpoint[i].link = this;
        m_endpoint[i].index = i;
        m_instances[i].core->SetComLynxCallbacks(PublishCallback, SampleCallback, BreakCallback, NULL, &m_endpoint[i]);
    }
}

LibretroLink::~LibretroLink()
{
    for (unsigned i = 0; i < m_count; i++)
    {
        m_instances[i].core->SetComLynxCableConnected(false);
        m_instances[i].core->SetComLynxCallbacks(NULL, NULL, NULL, NULL, NULL);
    }
}

void LibretroLink::Reset()
{
    memset(&m_runtime, 0, sizeof(m_runtime));

    for (unsigned i = 0; i < m_count; i++)
    {
        m_runtime.origin[i] = m_instances[i].core->GetComLynxCycle();
        m_instances[i].core->SetComLynxCableConnected(true);
    }
}

u64 LibretroLink::Cycle(unsigned index)
{
    return LinkCycle(m_instances[index].core->GetComLynxCycle(), m_runtime.origin[index], index);
}

void LibretroLink::PublishCallback(u64 start_cycle, u32 bit_cycles, u16 bits, void* data)
{
    Endpoint* endpoint = static_cast<Endpoint*>(data);
    Runtime& runtime = endpoint->link->m_runtime;
    unsigned index = endpoint->index;

    Frame& frame = runtime.frames[index][runtime.frame_count[index]++ % 16];
    frame.start_cycle = LinkCycle(start_cycle, runtime.origin[index], index);
    frame.bit_cycles = bit_cycles;
    frame.bits = bits;
}

void LibretroLink::BreakCallback(bool asserted, u64 cycle, void* data)
{
    Endpoint* endpoint = static_cast<Endpoint*>(data);
    Runtime& runtime = endpoint->link->m_runtime;
    unsigned index = endpoint->index;

    Break& state = runtime.breaks[index][runtime.break_count[index]++ % 16];
    state.cycle = LinkCycle(cycle, runtime.origin[index], index);
    state.asserted = asserted;
}

bool LibretroLink::SampleCallback(u64 cycle, void* data)
{
    Endpoint* endpoint = static_cast<Endpoint*>(data);
    LibretroLink* link = endpoint->link;
    Runtime& runtime = link->m_runtime;
    cycle = LinkCycle(cycle, runtime.origin[endpoint->index], endpoint->index);

    for (unsigned i = 0; i < link->m_count; i++)
    {
        if (i == endpoint->index || !link->m_instances[i].core->GetMikey()->IsPoweredOn())
            continue;

        u32 count = MIN(runtime.break_count[i], 16u);
        for (u32 j = 0; j < count; j++)
        {
            const Break& state = runtime.breaks[i][(runtime.break_count[i] - 1 - j) % 16];
            if (state.cycle > cycle)
                continue;
            if (state.asserted)
                return false;
            break;
        }

        count = MIN(runtime.frame_count[i], 16u);
        for (u32 j = 0; j < count; j++)
        {
            const Frame& frame = runtime.frames[i][(runtime.frame_count[i] - 1 - j) % 16];
            if (frame.bit_cycles == 0 || cycle < frame.start_cycle)
                continue;

            u64 bit = (cycle - frame.start_cycle) / frame.bit_cycles;
            if (bit < 11 && (frame.bits & (1u << bit)) == 0)
                return false;
        }
    }

    return true;
}

static bool CanRun(GearlynxCore* core)
{
    return core->GetMedia()->IsReady() && core->GetMedia()->IsBiosLoaded() &&
        core->GetMikey()->IsPoweredOn() && !core->IsPaused();
}

void LibretroLink::RunFrame()
{
    bool running[GEARLYNX_LINK_MAX_PLAYERS];
    for (unsigned i = 0; i < m_count; i++)
    {
        m_instances[i].sample_count = 0;
        running[i] = CanRun(m_instances[i].core);
        if (!running[i])
            m_instances[i].core->RunToVBlank((u8*)m_instances[i].frame_buffer,
                m_instances[i].audio_buffer, &m_instances[i].sample_count);
    }

    u64 master_start = Cycle(0);
    u64 target = master_start + 450000;
    if (!running[0])
    {
        GLYNX_Runtime_Info info;
        m_instances[0].core->GetRuntimeInfo(info);
        u32 cycles = info.frame_time > 0.0f ?
            (u32)MIN(info.frame_time * (GLYNX_MASTER_CLOCK / 1000.0f), 450000.0f) : GLYNX_MASTER_CLOCK / 60;
        target = m_runtime.frame_cycle + MAX(cycles, 1u);
    }

    for (;;)
    {
        unsigned index = m_count;
        u64 first_cycle = target;
        for (unsigned i = 0; i < m_count; i++)
        {
            if (running[i] && Cycle(i) < first_cycle)
            {
                index = i;
                first_cycle = Cycle(i);
            }
        }
        if (index == m_count)
            break;

        u32 clocks;
        bool vblank = m_instances[index].core->RunCycle(clocks);
        if (vblank)
            m_instances[index].core->RenderFrameBuffer((u8*)m_instances[index].frame_buffer);

        running[index] = clocks != 0 && CanRun(m_instances[index].core);
        if (index == 0 && (vblank || !running[0] || Cycle(0) - master_start > 450000))
            target = Cycle(0);
    }

    m_runtime.frame_cycle = target;
    for (unsigned i = 0; i < m_count; i++)
        m_instances[i].core->EndFrame(m_instances[i].audio_buffer, &m_instances[i].sample_count);
}

void LibretroLink::Layout(int placement, unsigned* columns, unsigned* rows, unsigned* width, unsigned* height)
{
    *columns = placement == LINK_VERTICAL ? 1 : (placement == LINK_GRID ? 2 : m_count);
    *rows = (m_count + *columns - 1) / *columns;
    *width = 0;
    *height = 0;
    for (unsigned i = 0; i < m_count; i++)
    {
        GLYNX_Runtime_Info info;
        m_instances[i].core->GetRuntimeInfo(info);
        *width = MAX(*width, (unsigned)info.screen_width);
        *height = MAX(*height, (unsigned)info.screen_height);
    }
}

void LibretroLink::Geometry(int placement, int selection, unsigned* width, unsigned* height)
{
    if (selection > 0 && (unsigned)selection <= m_count)
    {
        GLYNX_Runtime_Info info;
        m_instances[selection - 1].core->GetRuntimeInfo(info);
        *width = info.screen_width;
        *height = info.screen_height;
        return;
    }

    unsigned columns, rows;
    Layout(placement, &columns, &rows, width, height);
    *width *= columns;
    *height *= rows;
}

const u16* LibretroLink::Video(int placement, bool switched, int selection)
{
    if (selection > 0 && (unsigned)selection <= m_count)
        return m_instances[selection - 1].frame_buffer;

    unsigned columns, rows, cell_width, cell_height;
    Layout(placement, &columns, &rows, &cell_width, &cell_height);
    unsigned width = columns * cell_width;
    memset(m_video, 0, width * rows * cell_height * sizeof(u16));

    for (unsigned i = 0; i < m_count; i++)
    {
        unsigned index = switched ? m_count - 1 - i : i;
        GLYNX_Runtime_Info info;
        m_instances[index].core->GetRuntimeInfo(info);
        unsigned x = (i % columns) * cell_width + (cell_width - info.screen_width) / 2;
        unsigned y = (i / columns) * cell_height + (cell_height - info.screen_height) / 2;
        for (int line = 0; line < info.screen_height; line++)
            memcpy(m_video + (y + line) * width + x,
                m_instances[index].frame_buffer + line * info.screen_width, info.screen_width * sizeof(u16));
    }

    return m_video;
}

const s16* LibretroLink::Audio(int selection, int* count)
{
    if (selection != GEARLYNX_LINK_MAX_PLAYERS)
    {
        if (selection < 0 || (unsigned)selection >= m_count)
            selection = 0;
        *count = m_instances[selection].sample_count;
        return m_instances[selection].audio_buffer;
    }

    *count = 0;
    for (unsigned i = 0; i < m_count; i++)
        *count = MAX(*count, m_instances[i].sample_count);

    for (int i = 0; i < *count; i++)
    {
        int sample = 0;
        for (unsigned j = 0; j < m_count; j++)
            sample += i < m_instances[j].sample_count ? m_instances[j].audio_buffer[i] : 0;
        m_mix[i] = (s16)(sample / (int)m_count);
    }

    return m_mix;
}

void LibretroLink::SetSavePath(const char* content_path, const char* save_directory)
{
    m_save_path[0] = 0;

    if (!content_path || !content_path[0])
        return;

    const char* filename = strrchr(content_path, '/');
    const char* backslash = strrchr(content_path, '\\');

    if (backslash && (!filename || backslash > filename))
        filename = backslash;

    filename = filename ? filename + 1 : content_path;
    int count;

    if (save_directory && save_directory[0])
        count = snprintf(m_save_path, sizeof(m_save_path), "%s/%s", save_directory, filename);
    else
        count = snprintf(m_save_path, sizeof(m_save_path), "%s", content_path);

    if (count < 0 || (size_t)count >= sizeof(m_save_path))
    {
        m_save_path[0] = 0;
        Log("Link save path is too long");
        return;
    }

    char* extension = strrchr(m_save_path, '.');
    char* separator = strrchr(m_save_path, '/');
    char* windows_separator = strrchr(m_save_path, '\\');

    if (windows_separator && (!separator || windows_separator > separator))
        separator = windows_separator;

    if (extension && (!separator || extension > separator))
        *extension = 0;
}

void LibretroLink::PersistentMemory(bool write, const retro_vfs_interface* vfs)
{
    if (!m_save_path[0])
        return;

    for (unsigned i = 1; i < m_count; i++)
    {
        char path[4112];
        snprintf(path, sizeof(path), "%s.srm%u", m_save_path, i + 1);
        if (!PersistentMemory(path, i, write, vfs) && write)
            Log("Could not save screen %u memory: %s", i + 1, path);
    }
}

bool LibretroLink::PersistentMemory(const char* path, unsigned index, bool write, const retro_vfs_interface* vfs)
{
    Media* media = m_instances[index].core->GetMedia();
    size_t size = media->GetSaveMemorySize();
    void* data = media->GetSaveMemoryPointer();

    if (!size || !data)
        return true;

    u8* buffer = new u8[size];

    if (write)
        memcpy(buffer, data, size);

    size_t total = 0;
    bool closed = false;

    if (vfs)
    {
        if (!vfs->open || !vfs->close || (write ? !vfs->write : !vfs->read))
        {
            delete[] buffer;
            return false;
        }

        retro_vfs_file_handle* file = vfs->open(path, write ? RETRO_VFS_FILE_ACCESS_WRITE : RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);

        if (file)
        {
            while (total < size)
            {
                int64_t count = write ? vfs->write(file, buffer + total, size - total) : vfs->read(file, buffer + total, size - total);

                if (count <= 0 || (uint64_t)count > size - total)
                    break;

                total += (size_t)count;
            }

            closed = vfs->close(file) == 0;
        }
    }
    else
    {
        FILE* file = fopen_utf8(path, write ? "wb" : "rb");

        if (file)
        {
            total = write ? fwrite(buffer, 1, size, file) : fread(buffer, 1, size, file);
            closed = fclose(file) == 0;
        }
    }

    bool success = closed && total == size;

    if (!write && success)
        memcpy(data, buffer, size);

    delete[] buffer;
    return success;
}
