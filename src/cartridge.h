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

#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "common.h"

class Cartridge
{
public:
    enum GLYNX_Cartridge_Rotation
    {
        NO_ROTATION = 0,
        ROTATE_LEFT = 1,
        ROTATE_RIGHT = 2
    };

    enum GLYNX_Cartridge_EEPROM
    {
        NO_EEPROM = 0,
        EEPROM_93C46 = 1,
        EEPROM_93C56 = 2,
        EEPROM_93C66 = 3,
        EEPROM_93C76 = 4,
        EEPROM_93C86 = 5,
        EEPROM_SD = 0x40,
        EEPROM_8BIT = 0x80
    };

public:
    Cartridge();
    ~Cartridge();
    void Init();
    void Reset();
    u8* GetROM();
    u8* GetBIOS();
    bool IsReady();
    bool IsBiosLoaded();
    bool IsBiosValid();
    int GetROMSize();
    u32 GetCRC();
    GLYNX_Cartridge_Header* GetHeader();
    GLYNX_Cartridge_Rotation GetRotation();
    GLYNX_Cartridge_EEPROM GetEEPROM();
    const char* GetFilePath();
    const char* GetFileDirectory();
    const char* GetFileName();
    const char* GetFileExtension();
    bool LoadFromFile(const char* path);
    bool LoadFromBuffer(const u8* buffer, int size, const char* path);
    bool LoadBios(const char* path);
    u8 ReadBank0();
    u8 ReadBank1();
    void WriteBank0(u8 value);
    void WriteBank1(u8 value);
    void ShiftRegisterStrobe(bool strobe);
    void ShiftRegisterBit(bool bit);

private:
    bool LoadFromZipFile(const u8* buffer, int size);
    void GatherCartridgeInfoFromDB();
    bool GatherHeader(const u8* buffer);
    void DefaultHeader();
    void SetupBanks();
    void GatherDataFromPath(const char* path);
    GLYNX_Cartridge_Rotation ReadHeaderRotation(u8 rotation);
    GLYNX_Cartridge_EEPROM ReadHeaderEEPROM(u8 eeprom);
    bool IsValidFile(const char* path);

private:
    u8* m_rom;
    u32 m_rom_size;
    u8 m_bios[GLYNX_BIOS_SIZE] = {};
    bool m_is_bios_loaded;
    bool m_is_bios_valid;
    bool m_ready;
    char m_file_path[512];
    char m_file_directory[512];
    char m_file_name[512];
    char m_file_extension[512];
    GLYNX_Cartridge_Header m_header;
    u8* m_bank_data[2];
    u32 m_bank_size[2];
    u32 m_bank_mask[2];
    u32 m_address_shift;
    u32 m_address_shift_bits[2];
    u32 m_page_offset;
    u32 m_page_offset_mask[2];
    bool m_shift_register_strobe;
    bool m_shift_register_bit;
    GLYNX_Cartridge_Rotation m_rotation;
    GLYNX_Cartridge_EEPROM m_eeprom;
    u32 m_crc;
};

#endif /* CARTRIDGE_H */