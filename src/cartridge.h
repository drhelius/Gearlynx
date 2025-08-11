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
    bool IsReady();
    bool IsBiosLoaded();
    bool IsBiosValid();
    int GetROMSize();
    u32 GetCRC();
    u16 GetBank0Size();
    u16 GetBank1Size();
    u8 GetVersion();
    const char* GetName();
    const char* GetManufacturer();
    GLYNX_Cartridge_Rotation GetRotation();
    bool GetAUDIN();
    GLYNX_Cartridge_EEPROM GetEEPROM();
    const char* GetFilePath();
    const char* GetFileDirectory();
    const char* GetFileName();
    const char* GetFileExtension();
    bool LoadFromFile(const char* path);
    bool LoadFromBuffer(const u8* buffer, int size, const char* path);
    bool LoadBios(const char* path);

private:
    bool LoadFromZipFile(const u8* buffer, int size, const char* path);
    void GatherCartridgeInfoFromDB();
    void GatherBIOSInfoFromDB();
    bool GatherHeader(const u8* buffer);
    void GatherDataFromPath(const char* path);
    bool CheckMissingInfo();
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
    u16 m_bank0_size;
    u16 m_bank1_size;
    u8 m_version;
    char m_name[128];
    char m_manufacturer[32];
    GLYNX_Cartridge_Rotation m_rotation;
    bool m_audin;
    GLYNX_Cartridge_EEPROM m_eeprom;
    u32 m_crc;
};

#endif /* CARTRIDGE_H */