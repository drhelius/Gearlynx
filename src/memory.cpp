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
#include "memory.h"
#include "cartridge.h"
#include "input.h"
#include "audio.h"
#include "game_db.h"

Memory::Memory(Cartridge* cartridge, Input* input, Audio* audio)
{
    m_cartridge = cartridge;
    m_input = input;
    m_audio = audio;
    InitPointer(m_disassembler);
    InitPointer(m_test_memory);
    InitPointer(m_bios);
}

Memory::~Memory()
{
    SafeDeleteArray(m_bios);
    SafeDeleteArray(m_test_memory);
    if (IsValidPointer(m_disassembler))
    {
        for (int i = 0; i < 0x200000; i++)
        {
            SafeDelete(m_disassembler[i]);
        }
        SafeDeleteArray(m_disassembler);
    }
}

void Memory::Init()
{
    m_bios_loaded = false;
    m_bios = new u8[0x200];
    for (int i = 0; i < 0x200; i++)
        m_bios[i] = 0;

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_disassembler = new GLYNX_Disassembler_Record*[0x200000];
    for (int i = 0; i < 0x200000; i++)
    {
        InitPointer(m_disassembler[i]);
    }
#endif

#if defined(GLYNX_TESTING)
    m_test_memory = new u8[0x10000];
#endif

    Reset();
}

void Memory::Reset()
{
#if defined(GLYNX_TESTING)
    for (int i = 0; i < 0x10000; i++)
        m_test_memory[i] = rand() & 0xFF;
#endif
}

void Memory::LoadBios(const char* file_path)
{
    using namespace std;

    m_bios_loaded = false;

    ifstream file(file_path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = static_cast<int> (file.tellg());

        if (size != 0x200)
        {
            Log("Incorrect BIOS size %d: %s", size, file_path);
            return;
        }

        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char*>(m_bios), size);
        file.close();

        u32 crc = CalculateCRC32(0, m_bios, size);

        if (crc != GLYNX_DB_BIOS_CRC)
        {
            Log("Incorrect BIOS CRC %08X: %s", crc, file_path);
        }

        m_bios_loaded = true;

        Log("BIOS %s loaded (%d bytes)", file_path, size);
    }
    else
    {
        Log("ERROR: There was a problem opening the file %s", file_path);
    }
}

bool Memory::IsBiosLoaded()
{
    return m_bios_loaded;
}

GLYNX_Disassembler_Record* Memory::GetDisassemblerRecord(u16 address)
{
    return m_disassembler[address];
}

GLYNX_Disassembler_Record* Memory::GetOrCreateDisassemblerRecord(u16 address)
{
    //TODO: implement disassembler
    GLYNX_Disassembler_Record* record = m_disassembler[address];

    if (!IsValidPointer(record))
    {
        record = new GLYNX_Disassembler_Record();
        record->address = address;
        record->segment[0] = 0;
        record->name[0] = 0;
        record->bytes[0] = 0;
        record->size = 0;
        for (int i = 0; i < 7; i++)
            record->opcodes[i] = 0;
        record->jump = false;
        record->jump_address = 0;
        record->jump_bank = 0;
        record->subroutine = false;
        record->irq = 0;
        m_disassembler[address] = record;
    }

    return record;
}

void Memory::ResetDisassemblerRecords()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    for (int i = 0; i < 0x200000; i++)
    {
        SafeDelete(m_disassembler[i]);
    }
#endif
}

GLYNX_Disassembler_Record** Memory::GetAllDisassemblerRecords()
{
    return m_disassembler;
}

void Memory::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Memory::LoadState(std::istream& stream)
{
    UNUSED(stream);
}