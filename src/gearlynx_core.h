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
#include "types.h"

class Audio;
class Input;
class Memory;
class Cartridge;
class M6502;

class GearlynxCore
{
public:

    struct GLYNX_Debug_Run
    {
        bool step_debugger;
        bool stop_on_breakpoint;
        bool stop_on_run_to_breakpoint;
        bool stop_on_irq;
    };

    struct GLYNX_Debug_State
    {
        u16 PC;
        u8 P;
        u8 A;
        u8 X;
        u8 Y;
        u8 S;
        int cycles;
    };

    typedef void (*GLYNX_Debug_Callback)(GLYNX_Debug_State* state);

public:
    GearlynxCore();
    ~GearlynxCore();
    void Init(GLYNX_Pixel_Format pixel_format = GLYNX_PIXEL_RGBA8888);
    bool RunToVBlank(u8* frame_buffer, s16* sample_buffer, int* sample_count, GLYNX_Debug_Run* debug = NULL);
    bool LoadROM(const char* file_path);
    bool LoadROMFromBuffer(const u8* buffer, int size);
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
    Cartridge* GetCartridge();
    Audio* GetAudio();
    Input* GetInput();
    M6502* GetM6502();
    void SetDebugCallback(GLYNX_Debug_Callback callback);

private:
    void Reset();
    bool SaveState(std::ostream& stream, size_t& size, bool screenshot);
    bool LoadState(std::istream& stream);
    std::string GetSaveStatePath(const char* path, int index);

private:
    Memory* m_memory;
    Audio* m_audio;
    Input* m_input;
    Cartridge* m_cartridge;
    M6502* m_m6502;
    bool m_paused;
    u64 m_clock;
    GLYNX_Debug_Callback m_debug_callback;
};

#endif /* GEARLYNX_CORE_H */