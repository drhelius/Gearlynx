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
#include "suzy.h"
#include "mikey.h"

Memory::Memory(Cartridge* cartridge, Input* input, Audio* audio, Suzy* suzy, Mikey* mikey)
{
    m_cartridge = cartridge;
    m_input = input;
    m_audio = audio;
    m_suzy = suzy;
    m_mikey = mikey;
    InitPointer(m_disassembler);
    InitPointer(m_memory);
}

Memory::~Memory()
{
    SafeDeleteArray(m_memory);

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
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_disassembler = new GLYNX_Disassembler_Record*[0x200000];
    for (int i = 0; i < 0x200000; i++)
    {
        InitPointer(m_disassembler[i]);
    }
#endif

    m_memory = new u8[0x10000];

    Reset();
}

void Memory::Reset()
{
    m_mapctl = 0;

    for (int i = 0; i < 0x10000; i++)
        m_memory[i] = rand() & 0xFF;

    SetupDefaultMemoryMap();
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

void Memory::SetupDefaultMemoryMap()
{
    for (int i = 0; i < 0xFF; i++)
    {
        m_read_page[i] = m_memory + (i << 8);
        m_write_page[i] = m_memory + (i << 8);
        m_read_fn[i] = NULL;
        m_write_fn[i] = NULL;
    }

    m_read_page[0xFF] = NULL;
    m_write_page[0xFF] = NULL;
    m_read_fn[0xFF] = &Memory::LastPageRead;
    m_write_fn[0xFF] = &Memory::LastPageWrite;

    RebuildMemoryMap();
}

u8 Memory::SuzyRead(u16 address)
{
    Debug("SuzyRead called with address: %04X", address);
    return m_suzy->Read(address);
}

void Memory::SuzyWrite(u16 address, u8 value)
{
    Debug("SuzyWrite called with address: %04X, value: %02X", address, value);
    m_suzy->Write(address, value);
}

u8 Memory::MikeyRead(u16 address)
{
    Debug("MikeyRead called with address: %04X", address);
    return m_mikey->Read(address);
}

void Memory::MikeyWrite(u16 address, u8 value)
{
    Debug("MikeyWrite called with address: %04X, value: %02X", address, value);
    m_mikey->Write(address, value);
}

u8 Memory::BiosRead(u16 address)
{
    Debug("BiosRead called with address: %04X", address);
    return 0;
}

void Memory::BiosWrite(u16 address, u8 value)
{
    Debug("BiosWrite called with address: %04X, value: %02X", address, value);
}

u8 Memory::LastPageRead(u16 address)
{
    Debug("LastPageRead called with address: %04X", address);

    if (unlikely(address == 0xFFF8))
        return m_memory[address];

    // BIOS not visible
    if (IS_SET_BIT(m_mapctl, 2))
    {
        return m_memory[address];
    }
    // BIOS visible
    else
    {
        u8* bios = m_cartridge->GetBIOS();
        return bios[address & 0x1FF];
    }
}

void Memory::LastPageWrite(u16 address, u8 value)
{
    Debug("LastPageWrite called with address: %04X, value: %02X", address, value);

    if (unlikely(address == 0xFFF8))
        m_memory[address] = value;

    // BIOS not visible
    if (IS_SET_BIT(m_mapctl, 2))
    {
        m_memory[address] = value;
    }
    // BIOS visible
    else
    {
        Debug("Writing to BIOS address %04X with value %02X is not allowed", address, value);
    }
}

void Memory::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Memory::LoadState(std::istream& stream)
{
    UNUSED(stream);
}