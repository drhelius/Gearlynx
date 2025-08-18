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

#ifndef MEMORY_H
#define MEMORY_H

#include <iostream>
#include <fstream>
#include "common.h"

class Cartridge;
class Input;
class Audio;
class Suzy;
class Mikey;

class Memory
{
public:
    struct Memory_State
    {
        u8 MAPCTL;
        u8* ram;
    };

public:
    Memory(Cartridge* cartridge, Input* input, Audio* audio, Suzy* suzy, Mikey* mikey);
    ~Memory();
    void Init();
    void Reset();
    u8* GetRAM();
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    Memory_State* GetState();
    GLYNX_Disassembler_Record* GetDisassemblerRecord(u16 address);
    GLYNX_Disassembler_Record* GetOrCreateDisassemblerRecord(u16 address);
    void ResetDisassemblerRecords();
    GLYNX_Disassembler_Record** GetAllDisassemblerRecords();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void SetMapCtl(u8 mapctl);
    void SetupDefaultMemoryMap();
    void RebuildMemoryMap();
    u8 SuzyRead(u16 address);
    void SuzyWrite(u16 address, u8 value);
    u8 MikeyRead(u16 address);
    void MikeyWrite(u16 address, u8 value);
    u8 BiosRead(u16 address);
    void BiosWrite(u16 address, u8 value);
    u8 LastPageRead(u16 address);
    void LastPageWrite(u16 address, u8 value);

private:
    Cartridge* m_cartridge;
    Input* m_input;
    Audio* m_audio;
    Suzy* m_suzy;
    Mikey* m_mikey;
    Memory_State m_state;
    GLYNX_Disassembler_Record** m_disassembler;
    u8* m_read_page[256];
    u8* m_write_page[256];
    typedef u8 (Memory::*PageReadFn)(u16 addr);
    typedef void (Memory::*PageWriteFn)(u16 addr, u8 v);
    PageReadFn m_read_fn[256];
    PageWriteFn m_write_fn[256];
};

#include "memory_inline.h"

#endif /* MEMORY_H */