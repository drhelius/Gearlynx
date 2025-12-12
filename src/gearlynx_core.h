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

#ifndef GEARLYNX_CORE_H
#define GEARLYNX_CORE_H

#include <iostream>
#include <fstream>
#include "common.h"

class Audio;
class Bus;
class Input;
class Memory;
class Media;
class M6502;
class Suzy;
class Mikey;

class GearlynxCore
{
public:

    struct GLYNX_Debug_Run
    {
        bool step_debugger;
        bool stop_on_breakpoint;
        bool stop_on_run_to_breakpoint;
        u8 stop_on_irq;
    };

    typedef void (*GLYNX_Debug_Callback)();

public:
    GearlynxCore();
    ~GearlynxCore();
    void Init(GLYNX_Pixel_Format pixel_format = GLYNX_PIXEL_RGBA8888);
    bool RunToVBlank(u8* frame_buffer, s16* sample_buffer, int* sample_count, GLYNX_Debug_Run* debug = NULL);
    bool LoadROM(const char* file_path);
    bool LoadROMFromBuffer(const u8* buffer, int size, const char* file_path = NULL);
    GLYNX_Bios_State LoadBios(const char* file_path);
    void ResetROM(bool preserve_ram);
    void KeyPressed(GLYNX_Keys key);
    void KeyReleased(GLYNX_Keys key);
    void Pause(bool paused);
    bool IsPaused();
    // void SaveRam();
    // void SaveRam(const char* path, bool full_path = false);
    // void LoadRam();
    // void LoadRam(const char* path, bool full_path = false);
    bool SaveState(const char* path = NULL, int index = -1, bool screenshot = false);
    bool SaveState(u8* buffer, size_t& size, bool screenshot = false);
    bool LoadState(const char* path = NULL, int index = -1);
    bool LoadState(const u8* buffer, size_t size);
    bool GetSaveStateHeader(int index, const char* path, GLYNX_SaveState_Header* header);
    bool GetSaveStateScreenshot(int index, const char* path, GLYNX_SaveState_Screenshot* screenshot);
    void ResetSound();
    bool GetRuntimeInfo(GLYNX_Runtime_Info& runtime_info);
    Memory* GetMemory();
    Media* GetMedia();
    Audio* GetAudio();
    Input* GetInput();
    M6502* GetM6502();
    Suzy* GetSuzy();
    Mikey* GetMikey();
    Bus* GetBus();
    void SetDebugCallback(GLYNX_Debug_Callback callback);

private:
    void Reset();
    template<bool debugger>
    bool RunToVBlankTemplate(u8* frame_buffer, s16* sample_buffer, int* sample_count, GLYNX_Debug_Run* debug);
    void PrepareForHomebrew();
    bool SaveState(std::ostream& stream, size_t& size, bool screenshot);
    bool LoadState(std::istream& stream);
    std::string GetSaveStatePath(const char* path, int index);

private:
    Memory* m_memory;
    Audio* m_audio;
    Bus* m_bus;
    Input* m_input;
    Media* m_media;
    M6502* m_m6502;
    Suzy* m_suzy;
    Mikey* m_mikey;
    bool m_paused;
    GLYNX_Debug_Callback m_debug_callback;
};

#include "gearlynx_core_inline.h"

#endif /* GEARLYNX_CORE_H */
