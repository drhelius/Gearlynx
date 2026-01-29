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

#ifndef MEDIA_INLINE_H
#define MEDIA_INLINE_H

#include <assert.h>
#include "media.h"
#include "eeprom.h"

INLINE u32 Media::GetCRC()
{
    return m_crc;
}

INLINE bool Media::IsReady()
{
    return m_ready;
}

INLINE bool Media::IsBiosLoaded()
{
    return m_is_bios_loaded;
}

INLINE bool Media::IsBiosValid()
{
    return m_is_bios_valid;
}

INLINE int Media::GetROMSize()
{
    return m_rom_size;
}

INLINE const char* Media::GetFilePath()
{
    return m_file_path;
}

INLINE const char* Media::GetFileDirectory()
{
    return m_file_directory;
}

INLINE const char* Media::GetFileName()
{
    return m_file_name;
}

INLINE const char* Media::GetFileExtension()
{
    return m_file_extension;
}

INLINE const char* Media::GetHeaderName()
{
    return m_header_name;
}

INLINE const char* Media::GetHeaderManufacturer()
{
    return m_header_manufacturer;
}

INLINE u16 Media::GetHeaderBank0PageSize()
{
    return (u16)m_bank_page_size[0];
}

INLINE u16 Media::GetHeaderBank1PageSize()
{
    return (u16)m_bank_page_size[1];
}

INLINE u8* Media::GetROM()
{
    return m_rom;
}

INLINE u8* Media::GetBIOS()
{
    return m_bios;
}

INLINE void Media::ForceRotation(GLYNX_Rotation rotation)
{
    m_forced_rotation = rotation;
}

INLINE GLYNX_Rotation Media::GetRotation()
{
    if (m_forced_rotation != GLYNX_ROTATION_AUTO)
        return m_forced_rotation;
    else if (m_rotation != GLYNX_ROTATION_AUTO)
        return m_rotation;
    else
        return GLYNX_ROTATION_DISABLED;
}

INLINE void Media::ForceConsoleType(GLYNX_Console_Type type)
{
    m_forced_console_type = type;
}

INLINE GLYNX_Console_Type Media::GetConsoleType()
{
    if (m_forced_console_type != GLYNX_CONSOLE_AUTO)
        return m_forced_console_type;
    else if (m_console_type != GLYNX_CONSOLE_AUTO)
        return m_console_type;
    else
        return GLYNX_CONSOLE_MODEL_II;
}

INLINE GLYNX_EEPROM Media::GetEEPROM()
{
    return m_eeprom;
}

INLINE Media::GLYNX_Media_Type Media::GetType()
{
    return m_type;
}

INLINE u16 Media::GetHomebrewBootAddress()
{
    return m_homebrew_boot_address;
}

INLINE int Media::GetEpyxHeaderless()
{
    return m_epyx_headerless;
}

INLINE bool Media::GetAudin()
{
    return m_audin;
}

INLINE bool Media::GetAudinValue()
{
    return m_audin_value;
}

INLINE void Media::SetAudinValue(bool value)
{
    m_audin_value = value;
}

INLINE u16 Media::GetCounterValue()
{
    return (u16)(m_page_offset & 0x7FF);
}

INLINE u32 Media::GetAddressShift()
{
    return m_address_shift;
}

INLINE bool Media::GetShiftRegisterStrobe()
{
    return m_shift_register_strobe;
}

INLINE bool Media::GetShiftRegisterBit()
{
    return m_shift_register_bit;
}

INLINE u32 Media::GetBankSize(int bank)
{
    if (bank < 0 || bank > 1)
        return 0;
    return m_bank_size[bank];
}

INLINE u32 Media::GetBankMask(int bank)
{
    if (bank < 0 || bank > 1)
        return 0;
    return m_bank_mask[bank];
}

INLINE u32 Media::GetAddressShiftBits(int bank)
{
    if (bank < 0 || bank > 1)
        return 0;
    return m_address_shift_bits[bank];
}

INLINE u32 Media::GetPageOffsetMask(int bank)
{
    if (bank < 0 || bank > 1)
        return 0;
    return m_page_offset_mask[bank];
}

INLINE void Media::SetBank1WriteEnable(bool enable)
{
    m_bank1_write_enable = enable;
}

INLINE bool Media::IsBank1WriteEnabled()
{
    return m_bank1_write_enable;
}

INLINE void Media::ShiftRegisterStrobe(bool strobe)
{
    if (strobe)
    {
        m_page_offset = 0;
        if (m_eeprom_instance->IsAvailable())
            m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
    }

    // Detect rising edge (0 -> 1)
    if (strobe && !m_shift_register_strobe)
    {
        // Serially shift in a bit to the address shift register
        m_address_shift <<= 1;
        m_address_shift |= (m_shift_register_bit ? 1 : 0);
        m_address_shift &= 0xFF;
    }

    m_shift_register_strobe = strobe;
}

INLINE void Media::ShiftRegisterBit(bool bit)
{
    m_shift_register_bit = bit;
}

INLINE u8 Media::ReadBank0()
{
    assert(m_bank_data[0] != NULL && m_bank_size[0] > 0);

    u32 address = (m_address_shift << m_address_shift_bits[0]) | (m_page_offset & m_page_offset_mask[0]);
    u8 data = m_bank_data[0][address & m_bank_mask[0]];

    if (!m_shift_register_strobe)
    {
        m_page_offset = (m_page_offset + 1) & 0x7FF;
        if (m_eeprom_instance->IsAvailable())
            m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
    }

    return data;
}

INLINE u8 Media::ReadBank1()
{
    if (m_bank_data[1] == NULL || m_bank_size[1] == 0)
        return 0xFF;

    u32 address = (m_address_shift << m_address_shift_bits[1]) | (m_page_offset & m_page_offset_mask[1]);
    u8 data = m_bank_data[1][address & m_bank_mask[1]];

    if (!m_shift_register_strobe)
    {
        m_page_offset = (m_page_offset + 1) & 0x7FF;
        if (m_eeprom_instance->IsAvailable())
            m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
    }

    return data;
}

INLINE u8 Media::PeekBank0()
{
    if (m_bank_data[0] == NULL || m_bank_size[0] == 0)
        return 0xFF;

    u32 address = (m_address_shift << m_address_shift_bits[0]) | (m_page_offset & m_page_offset_mask[0]);
    return m_bank_data[0][address & m_bank_mask[0]];
}

INLINE u8 Media::PeekBank1()
{
    if (m_bank_data[1] == NULL || m_bank_size[1] == 0)
        return 0xFF;

    u32 address = (m_address_shift << m_address_shift_bits[1]) | (m_page_offset & m_page_offset_mask[1]);
    return m_bank_data[1][address & m_bank_mask[1]];
}

INLINE void Media::WriteBank0(u8 value)
{
    UNUSED(value);
    // Bank0 is ROM, writes are ignored but counter still advances
    // This is used by games for EEPROM clocking
    if (!m_shift_register_strobe)
    {
        m_page_offset = (m_page_offset + 1) & 0x7FF;
        if (m_eeprom_instance->IsAvailable())
            m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
    }
}

INLINE void Media::WriteBank1(u8 value)
{
    if (!m_bank1_is_ram || m_bank_data[1] == NULL || m_bank_size[1] == 0)
    {
        Debug("WARNING: WriteBank1 called but bank1 is not RAM. Value: %02X", value);
        return;
    }

    if (!m_bank1_write_enable)
    {
        Debug("WARNING: WriteBank1 called but bank1 write is disabled. Value: %02X", value);
        if (!m_shift_register_strobe)
        {
            m_page_offset = (m_page_offset + 1) & 0x7FF;
            if (m_eeprom_instance->IsAvailable())
                m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
        }
        return;
    }

    m_bank1_dirty = true;

    u32 address = (m_address_shift << m_address_shift_bits[1]) | (m_page_offset & m_page_offset_mask[1]);
    m_bank_data[1][address & m_bank_mask[1]] = value;

    if (!m_shift_register_strobe)
    {
        m_page_offset = (m_page_offset + 1) & 0x7FF;
        if (m_eeprom_instance->IsAvailable())
            m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
    }
}

INLINE u8 Media::ReadBank0A()
{
    if (m_bank_data_a[0] == NULL || m_bank_size[0] == 0)
        return ReadBank0();

    u32 address = (m_address_shift << m_address_shift_bits[0]) | (m_page_offset & m_page_offset_mask[0]);
    u8 data = m_bank_data_a[0][address & m_bank_mask[0]];

    if (!m_shift_register_strobe)
    {
        m_page_offset = (m_page_offset + 1) & 0x7FF;
        if (m_eeprom_instance->IsAvailable())
            m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
    }

    return data;
}

INLINE u8 Media::ReadBank1A()
{
    if (m_bank_data_a[1] == NULL || m_bank_size[1] == 0)
        return ReadBank1();

    u32 address = (m_address_shift << m_address_shift_bits[1]) | (m_page_offset & m_page_offset_mask[1]);
    u8 data = m_bank_data_a[1][address & m_bank_mask[1]];

    if (!m_shift_register_strobe)
    {
        m_page_offset = (m_page_offset + 1) & 0x7FF;
        if (m_eeprom_instance->IsAvailable())
            m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
    }

    return data;
}

INLINE u8 Media::PeekBank0A()
{
    if (m_bank_data_a[0] == NULL || m_bank_size[0] == 0)
        return PeekBank0();

    u32 address = (m_address_shift << m_address_shift_bits[0]) | (m_page_offset & m_page_offset_mask[0]);
    return m_bank_data_a[0][address & m_bank_mask[0]];
}

INLINE u8 Media::PeekBank1A()
{
    if (m_bank_data_a[1] == NULL || m_bank_size[1] == 0)
        return PeekBank1();

    u32 address = (m_address_shift << m_address_shift_bits[1]) | (m_page_offset & m_page_offset_mask[1]);
    return m_bank_data_a[1][address & m_bank_mask[1]];
}

INLINE void Media::WriteBank0A(u8 value)
{
    UNUSED(value);
    // Bank0A is ROM, writes are ignored but counter still advances
    // This is used by games for EEPROM clocking
    if (!m_shift_register_strobe)
    {
        m_page_offset = (m_page_offset + 1) & 0x7FF;
        if (m_eeprom_instance->IsAvailable())
            m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
    }
}

INLINE void Media::WriteBank1A(u8 value)
{
    if (m_bank_data_a[1] == NULL || m_bank_size[1] == 0)
    {
        WriteBank1(value);
        return;
    }

    if (!m_bank1_write_enable)
    {
        if (!m_shift_register_strobe)
        {
            m_page_offset = (m_page_offset + 1) & 0x7FF;
            if (m_eeprom_instance->IsAvailable())
                m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
        }
        return;
    }

    m_bank1_dirty = true;

    u32 address = (m_address_shift << m_address_shift_bits[1]) | (m_page_offset & m_page_offset_mask[1]);
    m_bank_data_a[1][address & m_bank_mask[1]] = value;

    if (!m_shift_register_strobe)
    {
        m_page_offset = (m_page_offset + 1) & 0x7FF;
        if (m_eeprom_instance->IsAvailable())
            m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
    }
}

INLINE u8* Media::GetBankData(int bank)
{
    if (bank < 0 || bank > 1)
        return NULL;
    return m_bank_data[bank];
}

INLINE u8* Media::GetBankDataA(int bank)
{
    if (bank < 0 || bank > 1)
        return NULL;
    return m_bank_data_a[bank];
}

INLINE u8* Media::GetBank1Data()
{
    return m_bank_data[1];
}

INLINE u32 Media::GetBank1Size()
{
    return m_bank_size[1];
}

INLINE bool Media::IsBank1RAM()
{
    return m_bank1_is_ram;
}

INLINE bool Media::IsBank1Dirty()
{
    return m_bank1_dirty;
}

INLINE void Media::ClearBank1Dirty()
{
    m_bank1_dirty = false;
}

INLINE EEPROM* Media::GetEEPROMInstance()
{
    return m_eeprom_instance;
}

#endif /* MEDIA_INLINE_H */
