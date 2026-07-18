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

INLINE bool Media::IsInGameDatabase()
{
    return m_is_in_game_database;
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

INLINE const char* Media::GetFormatName()
{
    if (m_type == MEDIA_EPYX_HEADERLESS)
        return "EPYX headerless";
    else if (m_type == MEDIA_HOMEBREW)
        return "BS93";
    else if (m_is_lnx2)
        return "LNX2";
    else if (m_missing_header)
        return "Missing header";

    return "LYNX";
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

INLINE u8* Media::GetCartBankData(int bank)
{
    if (bank < 0 || bank >= CART_BANK_COUNT)
        return NULL;
    return m_cart_bank_data[bank];
}

INLINE u32 Media::GetCartBankSize(int bank)
{
    if (bank < 0 || bank >= CART_BANK_COUNT)
        return 0;
    return m_cart_bank_size[bank];
}

INLINE u32 Media::GetCartBankBlockSize(int bank)
{
    if (bank < 0 || bank >= CART_BANK_COUNT)
        return 0;
    return m_cart_bank_block_size[bank];
}

INLINE u32 Media::GetCartBankBlockCount(int bank)
{
    if (bank < 0 || bank >= CART_BANK_COUNT)
        return 0;
    return m_cart_bank_block_count[bank];
}

INLINE GLYNX_Cartridge_Bank_Type Media::GetCartBankType(int bank)
{
    if (bank < 0 || bank >= CART_BANK_COUNT)
        return GLYNX_CART_BANK_UNUSED;
    return m_cart_bank_type[bank];
}

INLINE bool Media::IsCartBankWritable(int bank)
{
    GLYNX_Cartridge_Bank_Type type = GetCartBankType(bank);
    return (type == GLYNX_CART_BANK_RAM || type == GLYNX_CART_BANK_RAM_PERSISTENT);
}

INLINE bool Media::IsCartBankPersistent(int bank)
{
    return GetCartBankType(bank) == GLYNX_CART_BANK_RAM_PERSISTENT;
}

INLINE const char* Media::GetCartBankName(int bank)
{
    switch (bank)
    {
        case CART_BANK_0:
            return "BANK0";
        case CART_BANK_0_A:
            return "BANK0A";
        case CART_BANK_1:
            return "BANK1";
        case CART_BANK_1_A:
            return "BANK1A";
        default:
            return "BANK";
    }
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

INLINE void Media::AdvanceCounter()
{
    if (!m_shift_register_strobe)
    {
        m_page_offset = (m_page_offset + 1) & 0x7FF;
        if (m_eeprom_instance->IsAvailable())
            m_eeprom_instance->ProcessEepromCounter((u16)m_page_offset);
    }
}

INLINE u32 Media::GetCartBankAddress(int bank)
{
    if (bank < 0 || bank >= CART_BANK_COUNT)
        return 0;

    u32 address = (m_address_shift << m_address_shift_bits[bank]) | (m_page_offset & m_page_offset_mask[bank]);
    return address & m_cart_bank_mask[bank];
}

INLINE u8 Media::ReadCartBank(int bank)
{
    if (m_cart_bank_data[bank] == NULL || m_cart_bank_size[bank] == 0)
    {
        AdvanceCounter();
        return 0xFF;
    }

    u8 data = m_cart_bank_data[bank][GetCartBankAddress(bank)];
    AdvanceCounter();
    return data;
}

INLINE u8 Media::PeekCartBank(int bank)
{
    if (m_cart_bank_data[bank] == NULL || m_cart_bank_size[bank] == 0)
        return 0xFF;

    return m_cart_bank_data[bank][GetCartBankAddress(bank)];
}

INLINE void Media::WriteCartBank(int bank, u8 value)
{
    if (!IsCartBankWritable(bank) || m_cart_bank_data[bank] == NULL || m_cart_bank_size[bank] == 0)
    {
        AdvanceCounter();
        return;
    }

    m_cart_bank_data[bank][GetCartBankAddress(bank)] = value;
    if (IsCartBankPersistent(bank))
        m_save_memory_dirty = true;
    AdvanceCounter();
}

INLINE u8 Media::ReadBank0()
{
    return ReadCartBank(CART_BANK_0);
}

INLINE u8 Media::ReadBank1()
{
    return ReadCartBank(CART_BANK_1);
}

INLINE u8 Media::PeekBank0()
{
    return PeekCartBank(CART_BANK_0);
}

INLINE u8 Media::PeekBank1()
{
    return PeekCartBank(CART_BANK_1);
}

INLINE void Media::WriteBank0(u8 value)
{
    WriteCartBank(CART_BANK_0, value);
}

INLINE void Media::WriteBank1(u8 value)
{
    WriteCartBank(CART_BANK_1, value);
}

INLINE u8 Media::ReadBank0A()
{
    if (m_cart_bank_data[CART_BANK_0_A] == NULL || m_cart_bank_size[CART_BANK_0_A] == 0)
        return ReadBank0();

    return ReadCartBank(CART_BANK_0_A);
}

INLINE u8 Media::ReadBank1A()
{
    if (m_cart_bank_data[CART_BANK_1_A] == NULL || m_cart_bank_size[CART_BANK_1_A] == 0)
        return ReadBank1();

    return ReadCartBank(CART_BANK_1_A);
}

INLINE u8 Media::PeekBank0A()
{
    if (m_cart_bank_data[CART_BANK_0_A] == NULL || m_cart_bank_size[CART_BANK_0_A] == 0)
        return PeekBank0();

    return PeekCartBank(CART_BANK_0_A);
}

INLINE u8 Media::PeekBank1A()
{
    if (m_cart_bank_data[CART_BANK_1_A] == NULL || m_cart_bank_size[CART_BANK_1_A] == 0)
        return PeekBank1();

    return PeekCartBank(CART_BANK_1_A);
}

INLINE void Media::WriteBank0A(u8 value)
{
    if (m_cart_bank_data[CART_BANK_0_A] == NULL || m_cart_bank_size[CART_BANK_0_A] == 0)
        WriteBank0(value);
    else
        WriteCartBank(CART_BANK_0_A, value);
}

INLINE void Media::WriteBank1A(u8 value)
{
    if (m_cart_bank_data[CART_BANK_1_A] == NULL || m_cart_bank_size[CART_BANK_1_A] == 0)
        WriteBank1(value);
    else
        WriteCartBank(CART_BANK_1_A, value);
}

INLINE EEPROM* Media::GetEEPROMInstance()
{
    return m_eeprom_instance;
}

INLINE u8* Media::GetSaveMemoryPointer()
{
    // EEPROM has priority over NVRAM
    if (m_eeprom_instance && m_eeprom_instance->IsAvailable())
        return m_eeprom_instance->GetData();
    else if (m_nvram_enabled)
        return m_nvram;
    else if (m_persistent_ram_size > 0)
        return m_persistent_ram;

    return NULL;
}

INLINE s32 Media::GetSaveMemorySize()
{
    // EEPROM has priority over NVRAM
    if (m_eeprom_instance && m_eeprom_instance->IsAvailable())
        return m_eeprom_instance->GetSize();
    else if (m_nvram_enabled)
        return NVRAM_SIZE;
    else if (m_persistent_ram_size > 0)
        return (s32)m_persistent_ram_size;

    return 0;
}

#endif /* MEDIA_INLINE_H */
