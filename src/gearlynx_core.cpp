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
#include "cartridge.h"
#include "memory.h"
#include "audio.h"
#include "input.h"
#include "m6502.h"

GearlynxCore::GearlynxCore()
{
    InitPointer(m_memory);
    InitPointer(m_audio);
    InitPointer(m_input);
    InitPointer(m_cartridge);
    InitPointer(m_m6502);
    InitPointer(m_debug_callback);
    m_paused = true;
    m_clock = 0;
}

GearlynxCore::~GearlynxCore()
{
    SafeDelete(m_m6502);
    SafeDelete(m_cartridge);
    SafeDelete(m_input);
    SafeDelete(m_audio);
    SafeDelete(m_memory);
}

void GearlynxCore::Init(GLYNX_Pixel_Format pixel_format)
{
    Log("Loading %s core %s by Ignacio Sanchez", GLYNX_TITLE, GLYNX_VERSION);

    srand((unsigned int)time(NULL));

    m_cartridge = new Cartridge();
    m_input = new Input();
    m_audio = new Audio();
    m_memory = new Memory(m_cartridge, m_input, m_audio);
    m_m6502 = new M6502();

    m_cartridge->Init();
    m_memory->Init();
    m_audio->Init();
    m_input->Init();
    m_m6502->Init(m_memory);
}

bool GearlynxCore::RunToVBlank(u8* frame_buffer, s16* sample_buffer, int* sample_count, GLYNX_Debug_Run* debug)
{
    if (m_paused || !m_cartridge->IsReady())
        return false;

    if (!m_memory->IsBiosLoaded())
    {
        //RenderFrameBuffer(pFrameBuffer);
        return false;
    }

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    GLYNX_Debug_State debug_state;
    bool get_debug_state = true;
    bool debug_enable = false;
    bool instruction_completed = false;
    if (IsValidPointer(debug))
    {
        debug_enable = true;
        m_m6502->EnableBreakpoints(debug->stop_on_breakpoint, debug->stop_on_irq);
    }
#else
    UNUSED(debug);
#endif

    //TODO: implement video
    //m_huc6260->SetBuffer(frame_buffer);
    bool stop = false;

    do
    {
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        if (get_debug_state)
        {
            get_debug_state = false;
            debug_state.PC = m_m6502->GetState()->PC->GetValue();
            debug_state.P = m_m6502->GetState()->P->GetValue();
            debug_state.A = m_m6502->GetState()->A->GetValue();
            debug_state.X = m_m6502->GetState()->X->GetValue();
            debug_state.Y = m_m6502->GetState()->Y->GetValue();
            debug_state.S = m_m6502->GetState()->S->GetValue();
        }

        instruction_completed = m_m6502->Clock();
#else
        m_m6502->Clock();
#endif

        //TODO: implement video
        //stop = m_huc6260->Clock();
        stop = true;

        if (m_clock % 6 == 0)
            m_audio->Clock();

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        if (debug_enable && debug->step_debugger && instruction_completed)
            stop = true;

        if (debug_enable && instruction_completed && m_m6502->BreakpointHit())
            stop = true;

        if (debug_enable && debug->stop_on_run_to_breakpoint && instruction_completed && m_m6502->RunToBreakpointHit())
            stop = true;

        if (debug_enable && instruction_completed && IsValidPointer(m_debug_callback))
        {
            debug_state.cycles = *m_m6502->GetState()->CYCLES;
            m_debug_callback(&debug_state);
            get_debug_state = true;
        }
#endif

        m_clock = (m_clock + 1) % 12;
    }
    while (!stop);

    m_audio->EndFrame(sample_buffer, sample_count);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    return m_m6502->BreakpointHit() || m_m6502->RunToBreakpointHit();
#else
    return false;
#endif
}

bool GearlynxCore::LoadROM(const char* file_path)
{
    if (m_cartridge->LoadFromFile(file_path))
    {
        m_memory->ResetDisassemblerRecords();
        Reset();
        return true;
    }
    else
        return false;
}

bool GearlynxCore::LoadROMFromBuffer(const u8* buffer, int size)
{
    if (m_cartridge->LoadFromBuffer(buffer, size))
    {
        m_memory->ResetDisassemblerRecords();
        Reset();
        return true;
    }
    else
        return false;
}

bool GearlynxCore::GetRuntimeInfo(GLYNX_Runtime_Info& runtime_info)
{

    // TODO: Implement runtime info
    // runtime_info.screen_width = m_huc6260->GetCurrentLineWidth();
    // runtime_info.screen_height = m_huc6260->GetCurrentHeight();
    runtime_info.screen_width = 0;
    runtime_info.screen_height = 0;

    if (m_cartridge->IsReady())
    {
    //     // if (m_video->GetOverscan() == Video::OverscanFull284)
    //     //     runtime_info.screen_width = GC_RESOLUTION_WIDTH + GC_RESOLUTION_SMS_OVERSCAN_H_284_L + GC_RESOLUTION_SMS_OVERSCAN_H_284_R;
    //     // if (m_video->GetOverscan() == Video::OverscanFull320)
    //     //     runtime_info.screen_width = GC_RESOLUTION_WIDTH + GC_RESOLUTION_SMS_OVERSCAN_H_320_L + GC_RESOLUTION_SMS_OVERSCAN_H_320_R;
    //     // if (m_video->GetOverscan() != Video::OverscanDisabled)
    //     //     runtime_info.screen_height = GC_RESOLUTION_HEIGHT + (2 * (m_cartridge->IsPAL() ? GC_RESOLUTION_OVERSCAN_V_PAL : GC_RESOLUTION_OVERSCAN_V));
        return true;
    }

    return false;
}

Memory* GearlynxCore::GetMemory()
{
    return m_memory;
}

Cartridge* GearlynxCore::GetCartridge()
{
    return m_cartridge;
}

Audio* GearlynxCore::GetAudio()
{
    return m_audio;
}

Input* GearlynxCore::GetInput()
{
    return m_input;
}

M6502* GearlynxCore::GetM6502()
{
    return m_m6502;
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

    if (m_cartridge->IsReady())
    {
        Log("Gearlynx RESET");
        Reset();
        m_m6502->DisassembleNextOPCode();
    }
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
    using namespace std;
    string full_path;

    if (IsValidPointer(path))
    {
        full_path = path;
        full_path += "/";
        full_path += m_cartridge->GetFileName();
    }
    else
        full_path = m_cartridge->GetFilePath();

    string::size_type dot_index = full_path.rfind('.');

    if (dot_index != string::npos)
        full_path.replace(dot_index + 1, full_path.length() - dot_index - 1, "state");

    if (index >= 0)
        full_path += to_string(index);

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
        Log("ERROR: Failed to save state to %s", full_path.c_str());
    return ret;
}

bool GearlynxCore::SaveState(u8* buffer, size_t& size, bool screenshot)
{
    using namespace std;

    Debug("Saving state to buffer [%d bytes]...", size);

    if (!m_cartridge->IsReady())
    {
        Log("ERROR: Cartridge is not ready when trying to save state");
        return false;
    }

    if (!IsValidPointer(buffer))
    {
        Log("ERROR: Invalid save state buffer");
        return false;
    }

    stringstream stream;
    size_t expected_size = 0;

    if (SaveState(stream, expected_size, screenshot))
    {
        if (size >= expected_size)
        {
            memcpy(buffer, stream.str().c_str(), expected_size);
            Log("Save state saved to buffer [%d bytes]", expected_size);
        }
        else
        {
            Debug("Calculating state size: %d bytes", expected_size);
        }
        size = expected_size;
    }

    return true;
}

bool GearlynxCore::SaveState(std::ostream& stream, size_t& size, bool screenshot)
{
    using namespace std;

    if (!m_cartridge->IsReady())
    {
        Log("ERROR: Cartridge is not ready when trying to save state");
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

    GLYNX_SaveState_Header header;
    header.magic = GLYNX_SAVESTATE_MAGIC;
    header.version = GLYNX_SAVESTATE_VERSION;
    header.timestamp = time(NULL);
    strncpy(header.rom_name, m_cartridge->GetFileName(), sizeof(header.rom_name));
    header.rom_crc = m_cartridge->GetCRC();

    Debug("Save state header magic: 0x%08x", header.magic);
    Debug("Save state header version: %d", header.version);
    Debug("Save state header timestamp: %d", header.timestamp);
    Debug("Save state header rom name: %s", header.rom_name);
    Debug("Save state header rom crc: 0x%08x", header.rom_crc);

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

    size = static_cast<size_t>(stream.tellp());
    size += sizeof(header);
    header.size = static_cast<u32>(size);

    Debug("Save state header size: %d", header.size);

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
            Log("ERROR: Failed to load state from %s", full_path.c_str());
    }
    else
    {
        Log("ERROR: Load state file doesn't exist: %s", full_path.c_str());
    }

    stream.close();
    return ret;
}

bool GearlynxCore::LoadState(const u8* buffer, size_t size)
{
    using namespace std;

    Debug("Loading state to buffer [%d bytes]...", size);

    if (!m_cartridge->IsReady())
    {
        Log("ERROR: Cartridge is not ready when trying to load state");
        return false;
    }

    if (!IsValidPointer(buffer) || (size == 0))
    {
        Log("ERROR: Invalid load state buffer");
        return false;
    }

    stringstream stream;
    stream.write(reinterpret_cast<const char*> (buffer), size);

    bool ret = LoadState(stream);
    Log("Save state loaded from buffer [%d bytes]", size);
    return ret;
}

bool GearlynxCore::LoadState(std::istream& stream)
{
    using namespace std;

    if (!m_cartridge->IsReady())
    {
        Log("ERROR: Cartridge is not ready when trying to load state");
        return false;
    }

    GLYNX_SaveState_Header header;

    stream.seekg(0, ios::end);
    size_t size = static_cast<size_t>(stream.tellg());
    stream.seekg(0, ios::beg);

    stream.seekg(size - sizeof(header), ios::beg);
    stream.read(reinterpret_cast<char*> (&header), sizeof(header));
    stream.seekg(0, ios::beg);

    Debug("Load state header magic: 0x%08x", header.magic);
    Debug("Load state header version: %d", header.version);
    Debug("Load state header size: %d", header.size);
    Debug("Load state header timestamp: %d", header.timestamp);
    Debug("Load state header rom name: %s", header.rom_name);
    Debug("Load state header rom crc: 0x%08x", header.rom_crc);
    Debug("Load state header screenshot size: %d", header.screenshot_size);
    Debug("Load state header screenshot width: %d", header.screenshot_width);
    Debug("Load state header screenshot height: %d", header.screenshot_height);

    if ((header.magic != GLYNX_SAVESTATE_MAGIC))
    {
        Log("Invalid save state: 0x%08x", header.magic);
        return false;
    }

    if (header.version != GLYNX_SAVESTATE_VERSION)
    {
        Log("Invalid save state version: %d", header.version);
        return false;
    }

    if (header.size != size)
    {
        Log("Invalid save state size: %d", header.size);
        return false;
    }

    if (header.rom_crc != m_cartridge->GetCRC())
    {
        Log("Invalid save state rom crc: 0x%08x", header.rom_crc);
        return false;
    }

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
        Log("ERROR: Invalid save state screenshot buffer");
        return false;
    }

    string full_path = GetSaveStatePath(path, index);
    Debug("Loading state screenshot from %s...", full_path.c_str());

    ifstream stream;
    stream.open(full_path.c_str(), ios::in | ios::binary);

    if (stream.fail())
    {
        Log("ERROR: Savestate file doesn't exist %s", full_path.c_str());
        stream.close();
        return false;
    }

    GLYNX_SaveState_Header header;

    stream.seekg(0, ios::end);
    size_t savestate_size = static_cast<size_t>(stream.tellg());
    stream.seekg(0, ios::beg);

    stream.seekg(savestate_size - sizeof(header), ios::beg);
    stream.read(reinterpret_cast<char*> (&header), sizeof(header));
    stream.seekg(0, ios::beg);

    if (header.screenshot_size == 0)
    {
        Debug("No screenshot data");
        stream.close();
        return false;
    }

    if (screenshot->size < header.screenshot_size)
    {
        Log("ERROR: Invalid screenshot buffer size %d < %d", screenshot->size, header.screenshot_size);
        stream.close();
        return false;
    }

    screenshot->size = header.screenshot_size;
    screenshot->width = header.screenshot_width;
    screenshot->height = header.screenshot_height;

    Debug("Screenshot size: %d bytes", screenshot->size);
    Debug("Screenshot width: %d", screenshot->width);
    Debug("Screenshot height: %d", screenshot->height);

    stream.seekg(savestate_size - sizeof(header) - screenshot->size, ios::beg);
    stream.read(reinterpret_cast<char*> (screenshot->data), screenshot->size);
    stream.close();

    return true;
}

void GearlynxCore::Reset()
{
    m_clock = 0;
    m_paused = false;
    m_memory->Reset();
    m_audio->Reset();
    m_input->Reset();
}
