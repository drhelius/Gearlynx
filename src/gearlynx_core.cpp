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

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "gearlynx_core.h"
#include "common.h"
#include "media.h"
#include "memory.h"
#include "audio.h"
#include "input.h"
#include "m6502.h"
#include "suzy.h"
#include "mikey.h"
#include "memory_stream.h"

GearlynxCore::GearlynxCore()
{
    InitPointer(m_memory);
    InitPointer(m_audio);
    InitPointer(m_input);
    InitPointer(m_media);
    InitPointer(m_m6502);
    InitPointer(m_suzy);
    InitPointer(m_mikey);
    InitPointer(m_debug_callback);
    m_paused = true;
    m_clock = 0;
}

GearlynxCore::~GearlynxCore()
{
    SafeDelete(m_m6502);
    SafeDelete(m_media);
    SafeDelete(m_input);
    SafeDelete(m_audio);
    SafeDelete(m_memory);
    SafeDelete(m_suzy);
    SafeDelete(m_mikey);
}

void GearlynxCore::Init(GLYNX_Pixel_Format pixel_format)
{
    Log("Loading %s core %s by Ignacio Sanchez", GLYNX_TITLE, GLYNX_VERSION);

    srand((unsigned int)time(NULL));

    m_media = new Media();
    m_input = new Input();
    m_m6502 = new M6502();
    m_suzy = new Suzy(m_media, m_m6502, m_input);
    m_mikey = new Mikey(m_media, m_m6502);
    m_memory = new Memory(m_media, m_input, m_suzy, m_mikey, m_m6502);
    m_audio = new Audio(m_mikey);

    m_media->Init();
    m_memory->Init();
    m_audio->Init();
    m_input->Init();
    m_suzy->Init(m_mikey, m_memory);
    m_mikey->Init(m_suzy, m_memory, pixel_format);
    m_m6502->Init(m_memory);
}

bool GearlynxCore::LoadROM(const char* file_path)
{
    if (m_media->LoadFromFile(file_path))
    {
        m_memory->ResetDisassemblerRecords();
        Reset();
        return true;
    }
    else
        return false;
}

bool GearlynxCore::LoadROMFromBuffer(const u8* buffer, int size, const char* file_path)
{
    if (m_media->LoadFromBuffer(buffer, size, file_path))
    {
        m_memory->ResetDisassemblerRecords();
        Reset();
        return true;
    }
    else
        return false;
}

bool GearlynxCore::LoadBios(const char* file_path)
{
    return m_media->LoadBios(file_path);
}

bool GearlynxCore::GetRuntimeInfo(GLYNX_Runtime_Info& runtime_info)
{
    runtime_info.screen_width = GLYNX_SCREEN_WIDTH;
    runtime_info.screen_height = GLYNX_SCREEN_HEIGHT;

    Mikey::Mikey_State* mikey_state = m_mikey->GetState();
    float t0_backup = (float)mikey_state->timers[0].backup;
    float t2_backup = (float)mikey_state->timers[2].backup;

    u8 t0_prescaler = mikey_state->timers[0].control_a & 0x07;
    float tick_T0_us = (float)k_mikey_timerX_period_us[t0_prescaler];

    float frame_time_ms = ((t0_backup + 1.0f) * tick_T0_us * (t2_backup + 1.0f)) / 1000.0f;

    // Clamp to (50, 80) FPS range
    runtime_info.frame_time = CLAMP(frame_time_ms, 12.5f, 20.0f);

    return m_media->IsReady();
}

void GearlynxCore::SetDebugCallback(GLYNX_Debug_Callback callback)
{
    m_debug_callback = callback;
}

void GearlynxCore::KeyPressed(GLYNX_Keys key)
{
    m_input->KeyPressed(key);
}

void GearlynxCore::KeyReleased(GLYNX_Keys key)
{
    m_input->KeyReleased(key);
}

void GearlynxCore::Pause(bool paused)
{
    if (!m_paused && paused)
    {
        Debug("Core paused");
    }
    else if (m_paused && !paused)
    {
        Debug("Core resumed");
    }

    m_paused = paused;
}

bool GearlynxCore::IsPaused()
{
    return m_paused;
}

void GearlynxCore::ResetROM(bool preserve_ram)
{
    UNUSED(preserve_ram);

    if (!m_media->IsReady())
        return;

    Log("Gearlynx RESET");
    Reset();
    m_m6502->DisassembleNextOPCode();
}

void GearlynxCore::ResetSound()
{
    m_audio->Reset();
}

// void GearlynxCore::SaveRam()
// {
//     SaveRam(NULL);
// }

// void GearlynxCore::SaveRam(const char*, bool)
// {
//     // TODO Implement save ram
// }

// void GearlynxCore::LoadRam()
// {
//     LoadRam(NULL);
// }

// void GearlynxCore::LoadRam(const char*, bool)
// {
//     // TODO Implement load ram
// }

std::string GearlynxCore::GetSaveStatePath(const char* path, int index)
{
    if (index < 0)
        return path;

    using namespace std;
    string full_path;

    if (IsValidPointer(path))
    {
        full_path = path;
        full_path += "/";
        full_path += m_media->GetFileName();
    }
    else
        full_path = m_media->GetFilePath();

    string::size_type dot_index = full_path.rfind('.');

    if (dot_index != string::npos)
        full_path.replace(dot_index + 1, full_path.length() - dot_index - 1, "state");

    stringstream ss;
    ss << index;
    full_path += ss.str();

    return full_path;
}

bool GearlynxCore::SaveState(const char* path, int index, bool screenshot)
{
    using namespace std;

    string full_path = GetSaveStatePath(path, index);
    Debug("Saving state to %s...", full_path.c_str());

    ofstream stream(full_path.c_str(), ios::out | ios::binary);

    size_t size;
    bool ret = SaveState(stream, size, screenshot);
    if (ret)
        Log("Saved state to %s", full_path.c_str());
    else
        Error("Failed to save state to %s", full_path.c_str());
    return ret;
}

bool GearlynxCore::SaveState(u8* buffer, size_t& size, bool screenshot)
{
    using namespace std;

    Debug("Saving state to buffer [%d bytes]...", size);

    if (!m_media->IsReady())
    {
        Error("Media is not ready when trying to save state");
        return false;
    }

    if (!IsValidPointer(buffer))
    {
        stringstream stream;
        if (!SaveState(stream, size, screenshot))
        {
            Error("Failed to save state to stream to calculate size");
            return false;
        }
        return true;
    }
    else
    {
        memory_stream direct_stream(reinterpret_cast<char*>(buffer), size);

        if (!SaveState(direct_stream, size, screenshot))
        {
            Error("Failed to save state to buffer");
            return false;
        }

        size = direct_stream.size();
        return true;
    }
}

bool GearlynxCore::SaveState(std::ostream& stream, size_t& size, bool screenshot)
{
    using namespace std;

    if (!m_media->IsReady())
    {
        Error("Media is not ready when trying to save state");
        return false;
    }

    Debug("Serializing save state...");

    // stream.write(reinterpret_cast<const char*> (&m_clock), sizeof(m_clock));
    // m_memory->SaveState(stream);
    // m_huc6260->SaveState(stream);
    // m_huc6270->SaveState(stream);
    // m_m6502->SaveState(stream);
    // m_audio->SaveState(stream);
    // m_input->SaveState(stream);

#if defined(__LIBRETRO__)
    GLYNX_SaveState_Header_Libretro header;
    header.magic = GLYNX_SAVESTATE_MAGIC;
    header.version = GLYNX_SAVESTATE_VERSION;
    Debug("Save state header magic: 0x%08x", header.magic);
    Debug("Save state header version: %d", header.version);
#else
    GLYNX_SaveState_Header header;
    header.magic = GLYNX_SAVESTATE_MAGIC;
    header.version = GLYNX_SAVESTATE_VERSION;

    header.timestamp = time(NULL);
    strncpy_fit(header.rom_name, m_media->GetFileName(), sizeof(header.rom_name));
    header.rom_crc = m_media->GetCRC();
    strncpy_fit(header.emu_build, GLYNX_VERSION, sizeof(header.emu_build));

    Debug("Save state header magic: 0x%08x", header.magic);
    Debug("Save state header version: %d", header.version);
    Debug("Save state header timestamp: %d", header.timestamp);
    Debug("Save state header rom name: %s", header.rom_name);
    Debug("Save state header rom crc: 0x%08x", header.rom_crc);
    Debug("Save state header emu build: %s", header.emu_build);

    if (screenshot)
    {
        //TODO: Implement screenshot
        // header.screenshot_width = m_huc6260->GetCurrentLineWidth();
        // header.screenshot_height = m_huc6260->GetCurrentHeight();

        // int bytes_per_pixel = 2;
        // if (m_huc6260->GetPixelFormat() == GLYNX_PIXEL_RGBA8888 || m_huc6260->GetPixelFormat() == GLYNX_PIXEL_BGRA8888)
        //     bytes_per_pixel = 4;

        // u8* frame_buffer = m_huc6260->GetBuffer();

        // header.screenshot_size = header.screenshot_width * header.screenshot_height * bytes_per_pixel;
        // stream.write(reinterpret_cast<const char*>(frame_buffer), header.screenshot_size);
        header.screenshot_size = 0;
        header.screenshot_width = 0;
        header.screenshot_height = 0;
    }
    else
    {
        header.screenshot_size = 0;
        header.screenshot_width = 0;
        header.screenshot_height = 0;
    }

    Debug("Save state header screenshot size: %d", header.screenshot_size);
    Debug("Save state header screenshot width: %d", header.screenshot_width);
    Debug("Save state header screenshot height: %d", header.screenshot_height);
#endif

    size = static_cast<size_t>(stream.tellp());
    size += sizeof(header);

#if !defined(__LIBRETRO__)
    header.size = static_cast<u32>(size);
    Debug("Save state header size: %d", header.size);
#endif

    stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
    return true;
}

bool GearlynxCore::LoadState(const char* path, int index)
{
    using namespace std;
    bool ret = false;

    string full_path = GetSaveStatePath(path, index);
    Debug("Loading state from %s...", full_path.c_str());

    ifstream stream;
    stream.open(full_path.c_str(), ios::in | ios::binary);

    if (!stream.fail())
    {
        ret = LoadState(stream);

        if (ret)
            Log("Loaded state from %s", full_path.c_str());
        else
            Error("Failed to load state from %s", full_path.c_str());
    }
    else
    {
        Error("Load state file doesn't exist: %s", full_path.c_str());
    }

    stream.close();
    return ret;
}

bool GearlynxCore::LoadState(const u8* buffer, size_t size)
{
    using namespace std;

    Debug("Loading state to buffer [%d bytes]...", size);

    if (!m_media->IsReady())
    {
        Error("Media is not ready when trying to load state");
        return false;
    }

    if (!IsValidPointer(buffer) || (size == 0))
    {
        Error("Invalid load state buffer");
        return false;
    }

    memory_input_stream direct_stream(reinterpret_cast<const char*>(buffer), size);
    return LoadState(direct_stream);
}

bool GearlynxCore::LoadState(std::istream& stream)
{
    using namespace std;

    if (!m_media->IsReady())
    {
        Error("Media is not ready when trying to load state");
        return false;
    }

#if defined(__LIBRETRO__)
    GLYNX_SaveState_Header_Libretro header;
#else
    GLYNX_SaveState_Header header;
#endif

    stream.seekg(0, ios::end);
    size_t size = static_cast<size_t>(stream.tellg());
    stream.seekg(0, ios::beg);

    stream.seekg(size - sizeof(header), ios::beg);
    stream.read(reinterpret_cast<char*> (&header), sizeof(header));
    stream.seekg(0, ios::beg);

    Debug("Load state header magic: 0x%08x", header.magic);
    Debug("Load state header version: %d", header.version);

    if ((header.magic != GLYNX_SAVESTATE_MAGIC))
    {
        Log("Invalid save state: 0x%08x", header.magic);
        return false;
    }

    if (header.version != GLYNX_SAVESTATE_VERSION)
    {
        Error("Invalid save state version: %d", header.version);
        return false;
    }

#if !defined(__LIBRETRO__)
    Debug("Load state header size: %d", header.size);
    Debug("Load state header timestamp: %d", header.timestamp);
    Debug("Load state header rom name: %s", header.rom_name);
    Debug("Load state header rom crc: 0x%08x", header.rom_crc);
    Debug("Load state header screenshot size: %d", header.screenshot_size);
    Debug("Load state header screenshot width: %d", header.screenshot_width);
    Debug("Load state header screenshot height: %d", header.screenshot_height);
    Debug("Load state header emu build: %s", header.emu_build);

    if ((header.magic != GLYNX_SAVESTATE_MAGIC))
    {
        Log("Invalid save state: 0x%08x", header.magic);
        return false;
    }

    if (header.version != GLYNX_SAVESTATE_VERSION)
    {
        Error("Invalid save state version: %d", header.version);
        return false;
    }

    if (header.size != size)
    {
        Error("Invalid save state size: %d", header.size);
        return false;
    }

    if (header.rom_crc != m_media->GetCRC())
    {
        Error("Invalid save state rom crc: 0x%08x", header.rom_crc);
        return false;
    }
#endif

    Debug("Unserializing save state...");

    // stream.read(reinterpret_cast<char*> (&m_clock), sizeof(m_clock));
    // m_memory->LoadState(stream);
    // m_huc6260->LoadState(stream);
    // m_huc6270->LoadState(stream);
    // m_m6502->LoadState(stream);
    // m_audio->LoadState(stream);
    // m_input->LoadState(stream);

    return true;
}

bool GearlynxCore::GetSaveStateHeader(int index, const char* path, GLYNX_SaveState_Header* header)
{
    using namespace std;

    string full_path = GetSaveStatePath(path, index);
    Debug("Loading state header from %s...", full_path.c_str());

    ifstream stream;
    stream.open(full_path.c_str(), ios::in | ios::binary);

    if (stream.fail())
    {
        Debug("ERROR: Savestate file doesn't exist %s", full_path.c_str());
        stream.close();
        return false;
    }

    stream.seekg(0, ios::end);
    size_t savestate_size = static_cast<size_t>(stream.tellg());
    stream.seekg(0, ios::beg);

    stream.seekg(savestate_size - sizeof(GLYNX_SaveState_Header), ios::beg);
    stream.read(reinterpret_cast<char*> (header), sizeof(GLYNX_SaveState_Header));
    stream.seekg(0, ios::beg);

    return true;
}

bool GearlynxCore::GetSaveStateScreenshot(int index, const char* path, GLYNX_SaveState_Screenshot* screenshot)
{
    using namespace std;

    if (!IsValidPointer(screenshot->data) || (screenshot->size == 0))
    {
        Error("Invalid save state screenshot buffer");
        return false;
    }

    string full_path = GetSaveStatePath(path, index);
    Debug("Loading state screenshot from %s...", full_path.c_str());

    ifstream stream;
    stream.open(full_path.c_str(), ios::in | ios::binary);

    if (stream.fail())
    {
        Error("Savestate file doesn't exist %s", full_path.c_str());
        stream.close();
        return false;
    }

    GLYNX_SaveState_Header header;
    GetSaveStateHeader(index, path, &header);

    if (header.screenshot_size == 0)
    {
        Debug("No screenshot data");
        stream.close();
        return false;
    }

    if (screenshot->size < header.screenshot_size)
    {
        Error("Invalid screenshot buffer size %d < %d", screenshot->size, header.screenshot_size);
        stream.close();
        return false;
    }

    screenshot->size = header.screenshot_size;
    screenshot->width = header.screenshot_width;
    screenshot->height = header.screenshot_height;

    Debug("Screenshot size: %d bytes", screenshot->size);
    Debug("Screenshot width: %d", screenshot->width);
    Debug("Screenshot height: %d", screenshot->height);

    stream.seekg(header.size - sizeof(header) - screenshot->size, ios::beg);
    stream.read(reinterpret_cast<char*> (screenshot->data), screenshot->size);
    stream.close();

    return true;
}

void GearlynxCore::Reset()
{
    m_clock = 0;
    m_paused = false;

    m_suzy->Reset();
    m_mikey->Reset();
    m_memory->Reset();
    m_m6502->Reset();
    m_audio->Reset();
    m_input->Reset();

    if (m_media->GetType() == Media::MEDIA_HOMEBREW)
        PrepareForHomebrew();
}

void GearlynxCore::PrepareForHomebrew()
{
    u16 boot_address = m_media->GetHomebrewBootAddress();
    int size = m_media->GetROMSize();

    if (size <= 0)
        return;

    u8* ram = m_memory->GetRAM();
    u8* rom = m_media->GetROM();

    const int ram_size = 0x10000;
    const int start = (int)boot_address;
    const int first = MIN(size, ram_size - start);
    const int left  = size - first;

    memset(ram, 0, ram_size);

    if (first > 0)
        memcpy(ram + start, rom, first);
    if (left  > 0)
        memcpy(ram, rom + first, MIN(left, ram_size));

    m_m6502->GetState()->PC.SetValue(boot_address);
    m_m6502->DisassembleNextOPCode();

    m_mikey->GetState()->timers[0].backup = 0x9E;
    m_mikey->GetState()->timers[0].control_a = 0x18;
    m_mikey->GetState()->timers[2].backup = 0x68;
    m_mikey->GetState()->timers[2].control_a = 0x1F;

    m_mikey->GetState()->DISPCTL = 0x09;
}
