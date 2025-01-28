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

class Memory
{
public:
    struct GLYNX_Disassembler_Record
    {
        u32 address;
        u8 bank;
        char name[64];
        char bytes[25];
        char segment[5];
        u8 opcodes[7];
        int size;
        bool jump;
        u16 jump_address;
        u8 jump_bank;
        bool subroutine;
        int irq;
    };

public:
    Memory(Cartridge* cartridge, Input* input, Audio* audio);
    ~Memory();
    void Init();
    void Reset();
    void LoadBios(const char* file_path);
    bool IsBiosLoaded();
    u8 Read(u16 address, bool block_transfer = false);
    void Write(u16 address, u8 value);
    GLYNX_Disassembler_Record* GetDisassemblerRecord(u16 address);
    GLYNX_Disassembler_Record* GetOrCreateDisassemblerRecord(u16 address);
    void ResetDisassemblerRecords();
    GLYNX_Disassembler_Record** GetAllDisassemblerRecords();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    Cartridge* m_cartridge;
    Input* m_input;
    Audio* m_audio;
    GLYNX_Disassembler_Record** m_disassembler;
    u8* m_test_memory;
    u8* m_bios;
    bool m_bios_loaded;
};

#include "memory_inline.h"

#endif /* MEMORY_H */