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

#include <string>
#include <fstream>
#include <algorithm>
#include <assert.h>
#include "cartridge.h"
#include "miniz.h"
#include "crc.h"
#include "game_db.h"

Cartridge::Cartridge()
{
    InitPointer(m_rom);
    m_rom_size = 0;
    m_is_bios_loaded = false;
    m_is_bios_valid = false;
    m_ready = false;
    m_file_path[0] = 0;
    m_file_directory[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    InitPointer(m_bank0);
    InitPointer(m_bank1);
    m_rotation = NO_ROTATION;
    m_eeprom = NO_EEPROM;
    m_crc = 0;
}

Cartridge::~Cartridge()
{
    SafeDeleteArray(m_rom);
}

void Cartridge::Init()
{
    Reset();
}

void Cartridge::Reset()
{
    SafeDeleteArray(m_rom);
    m_rom_size = 0;
    m_ready = false;
    m_file_path[0] = 0;
    m_file_directory[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    memset(&m_header, 0, sizeof(m_header));
    InitPointer(m_bank0);
    InitPointer(m_bank1);
    m_rotation = NO_ROTATION;
    m_eeprom = NO_EEPROM;
    m_crc = 0;
}

u32 Cartridge::GetCRC()
{
    return m_crc;
}

bool Cartridge::IsReady()
{
    return m_ready;
}

bool Cartridge::IsBiosLoaded()
{
    return m_is_bios_loaded;
}

bool Cartridge::IsBiosValid()
{
    return m_is_bios_valid;
}

int Cartridge::GetROMSize()
{
    return m_rom_size;
}

const char* Cartridge::GetFilePath()
{
    return m_file_path;
}

const char* Cartridge::GetFileDirectory()
{
    return m_file_directory;
}

const char* Cartridge::GetFileName()
{
    return m_file_name;
}

const char* Cartridge::GetFileExtension()
{
    return m_file_extension;
}

u8* Cartridge::GetROM()
{
    return m_rom;
}

u8* Cartridge::GetBIOS()
{
    return m_bios;
}

GLYNX_Cartridge_Header* Cartridge::GetHeader()
{
    return &m_header;
}

Cartridge::GLYNX_Cartridge_Rotation Cartridge::GetRotation()
{
    return m_rotation;
}

Cartridge::GLYNX_Cartridge_EEPROM Cartridge::GetEEPROM()
{
    return m_eeprom;
}

bool Cartridge::LoadFromFile(const char* path)
{
    using namespace std;

    Log("Loading %s...", path);

    if (!IsValidFile(path))
        return false;

    Reset();

    GatherDataFromPath(path);

    ifstream file(path, ios::in | ios::binary | ios::ate);
    int size = (int)(file.tellg());

    if (file.is_open())
    {
        char* buffer = new char[size];
        file.seekg(0, ios::beg);
        file.read(buffer, size);
        file.close();

        bool is_empty = false;

        for (int i = 0; i < size; i++)
        {
            if (buffer[i] != 0)
                break;

            if (i == size - 1)
            {
                Error("File %s is empty!", path);
                is_empty = true;
                m_ready = false;
            }
        }

        if (!is_empty)
        {
            if (strcmp(m_file_extension, "zip") == 0)
                m_ready = LoadFromZipFile((u8*)(buffer), size, path);
            else
                m_ready = LoadFromBuffer((u8*)(buffer), size, path);
        }

        SafeDeleteArray(buffer);
    }
    else
    {
        Error("There was a problem loading the file %s...", path);
        m_ready = false;
    }

    if (!m_ready)
        Reset();

    return m_ready;
}

bool Cartridge::LoadFromBuffer(const u8* buffer, int size, const char* path)
{
    Log("Loading ROM from buffer... Size: %d", size);

    if (!IsValidPointer(buffer) || size <= 0)
    {
        Error("Unable to load ROM from buffer: Buffer invalid %p. Size: %d", buffer, size);
        return false;
    }

    Reset();

    GatherDataFromPath(path);

    if (size & 0x40)
    {
        Debug("Header expected");
    }

    if (GatherHeader(buffer))
    {
        size -= 0x40;
        buffer += 0x40;
    }
    else
    {
        Debug("WARNING: Unable to gather ROM header");
    }

    m_rom_size = size;
    Log("ROM Size: %d KB, %d bytes (0x%0X)", m_rom_size / 1024, m_rom_size, m_rom_size);

    m_rom = new u8[m_rom_size];
    memcpy(m_rom, buffer, m_rom_size);

    m_crc = CalculateCRC32(0, m_rom, m_rom_size);
    Log("ROM CRC32: %08X", m_crc);

    GatherCartridgeInfoFromDB();

    m_ready = true;

    Debug("ROM loaded from buffer. Size: %d bytes", m_rom_size);

    if (!m_ready)
        Reset();

    return m_ready;
}

bool Cartridge::LoadBios(const char* path)
{
    using namespace std;

    m_is_bios_loaded = false;
    m_is_bios_valid = false;

    ifstream file(path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = static_cast<int> (file.tellg());

        if (size != 0x200)
        {
            Error("Incorrect BIOS size %d: %s", size, path);
            return false;
        }

        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char*>(m_bios), size);
        file.close();

        u32 crc = CalculateCRC32(0, m_bios, size);

        m_is_bios_valid = (crc == GLYNX_DB_BIOS_CRC);

        if (!m_is_bios_valid)
        {
            Log("Incorrect BIOS CRC %08X: %s", crc, path);
        }

        m_is_bios_loaded = true;

        Log("BIOS %s loaded (%d bytes)", path, size);
    }
    else
    {
        Error("There was a problem opening the file %s", path);
    }

    return m_is_bios_loaded;
}

bool Cartridge::LoadFromZipFile(const u8* buffer, int size, const char* path)
{
    Debug("Loading from ZIP file... Size: %d, File: %s", size, path);

    using namespace std;

    mz_zip_archive zip_archive;
    mz_bool status;
    memset(&zip_archive, 0, sizeof (zip_archive));

    status = mz_zip_reader_init_mem(&zip_archive, (void*) buffer, size, 0);
    if (!status)
    {
        Error("mz_zip_reader_init_mem() failed!");
        return false;
    }

    for (unsigned int i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            Error("mz_zip_reader_file_stat() failed!");
            mz_zip_reader_end(&zip_archive);
            return false;
        }

        Debug("ZIP Content - Filename: \"%s\", Comment: \"%s\", Uncompressed size: %u, Compressed size: %u", file_stat.m_filename, file_stat.m_comment, (unsigned int) file_stat.m_uncomp_size, (unsigned int) file_stat.m_comp_size);

        string fn((const char*) file_stat.m_filename);
        string extension = fn.substr(fn.find_last_of(".") + 1);
        transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);

        if ((extension == "lnx") || (extension == "lyx"))
        {
            void *p;
            size_t uncomp_size;

            p = mz_zip_reader_extract_file_to_heap(&zip_archive, file_stat.m_filename, &uncomp_size, 0);
            if (!p)
            {
                Error("mz_zip_reader_extract_file_to_heap() failed!");
                mz_zip_reader_end(&zip_archive);
                return false;
            }

            bool ok = LoadFromBuffer((const u8*) p, (int)uncomp_size, fn.c_str());

            free(p);
            mz_zip_reader_end(&zip_archive);

            return ok;
        }
    }

    return false;
}

void Cartridge::GatherCartridgeInfoFromDB()
{
    int i = 0;
    bool found = false;

    while(!found && (k_game_database[i].title != 0))
    {
        u32 db_crc = k_game_database[i].crc;

        if (db_crc == m_crc)
        {
            found = true;
            Log("ROM found in database: %s. CRC: %08X", k_game_database[i].title, m_crc);

            if (m_rom_size == k_game_database[i].file_size)
            {
                Debug("ROM size matches database: %d bytes", m_rom_size);
            }
            else
            {
                Debug("WARNING: ROM size mismatch. Database: %d bytes, ROM: %d bytes", k_game_database[i].file_size, m_rom_size);
                Debug("Forcing ROM size to database value");
                m_rom_size = k_game_database[i].file_size;
            }

            if (k_game_database[i].bank0_page_size != 0)
            {
                Debug("Forcing bank0 size to database value: %d", k_game_database[i].bank0_page_size);
                m_header.bank0_page_size = k_game_database[i].bank0_page_size;
            }

            if (k_game_database[i].bank1_page_size != 0)
            {
                Debug("Forcing bank1 size to database value: %d", k_game_database[i].bank1_page_size);
                m_header.bank1_page_size = k_game_database[i].bank1_page_size;
            }

            if (k_game_database[i].flags & GLYNX_DB_FLAG_ROTATE_LEFT)
            {
                Debug("Forcing rotation to database value: Rotate left");
                m_header.rotation = ROTATE_LEFT;
                m_rotation = ROTATE_LEFT;
            }
            else if (k_game_database[i].flags & GLYNX_DB_FLAG_ROTATE_RIGHT)
            {
                Debug("Forcing rotation to database value: Rotate right");
                m_header.rotation = ROTATE_RIGHT;
                m_rotation = ROTATE_RIGHT;
            }

            if (k_game_database[i].flags & GLYNX_DB_FLAG_AUDIN)
            {
                Debug("Forcing AUDIN to database value: true");
                m_header.audin = 1;
            }

            if (k_game_database[i].flags & GLYNX_DB_FLAG_EEPROM_93C46)
            {
                Debug("Forcing EEPROM to database value: 93C46");
                m_header.eeprom = EEPROM_93C46;
                m_eeprom = EEPROM_93C46;
            }
        }
        else
            i++;
    }

    if (!found)
    {
        Debug("ROM not found in database. CRC: %08X", m_crc);
    }
}

bool Cartridge::GatherHeader(const u8* buffer)
{
    const u8* p = buffer;
    memset(&m_header, 0, sizeof(m_header));

    if (p[0] != 'L' || p[1] != 'Y' || p[2] != 'N' || p[3] != 'X')
    {
        Error("Invalid header magic: %c%c%c%c", p[0], p[1], p[2], p[3]);
        return false;
    }

    memcpy(m_header.magic, p, 4);
    p += 4;

    m_header.bank0_page_size = read_u16_le(p);
    p += 2;
    m_header.bank1_page_size = read_u16_le(p);
    p += 2;

    m_header.version = read_u16_le(p);
    p += 2;

    if (m_header.version != 1)
    {
        Error("Invalid header version: %d", m_header.version);
    }

    memcpy(m_header.name, p, 32);
    m_header.name[31] = 0;
    p += 32;

    memcpy(m_header.manufacturer, p, 16);
    m_header.manufacturer[15] = 0;
    p += 16;

    m_header.rotation = *p;
    p++;
    m_rotation = ReadHeaderRotation(m_header.rotation);

    m_header.audin = (*p & 0x01) != 0;
    p++;

    m_eeprom = ReadHeaderEEPROM(*p);

    Debug("Header magic: %c%c%c%c", m_header.magic[0], m_header.magic[1], m_header.magic[2], m_header.magic[3]);
    Debug("Header bank0 page size: %d", m_header.bank0_page_size);
    Debug("Header bank1 page size: %d", m_header.bank1_page_size);
    Debug("Header version: %d", m_header.version);
    Debug("Header name: %s", m_header.name);
    Debug("Header manufacturer: %s", m_header.manufacturer);
    Debug("Header rotation: %d", m_header.rotation);
    Debug("Header audin: %d", m_header.audin);
    Debug("Header EEPROM: %d", m_eeprom);

    return true;
}

void Cartridge::GatherDataFromPath(const char* path)
{
    if (!IsValidPointer(path))
    {
        m_file_path[0] = 0;
        m_file_directory[0] = 0;
        m_file_name[0] = 0;
        m_file_extension[0] = 0;
        return;
    }

    using namespace std;

    string fullpath(path);
    string directory;
    string filename;
    string extension;

    size_t pos = fullpath.find_last_of("/\\");
    if (pos != string::npos)
    {
        filename = fullpath.substr(pos + 1);
        directory = fullpath.substr(0, pos);
    }
    else
    {
        filename = fullpath;
        directory = "";
    }

    extension = fullpath.substr(fullpath.find_last_of(".") + 1);
    transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);

    snprintf(m_file_path, sizeof(m_file_path), "%s", path);
    snprintf(m_file_directory, sizeof(m_file_directory), "%s", directory.c_str());
    snprintf(m_file_name, sizeof(m_file_name), "%s", filename.c_str());
    snprintf(m_file_extension, sizeof(m_file_extension), "%s", extension.c_str());
}

Cartridge::GLYNX_Cartridge_Rotation Cartridge::ReadHeaderRotation(u8 rotation)
{
    switch (rotation)
    {
        case 0:
            Debug("Header rotation: No rotation");
            return NO_ROTATION;
        case 1:
            Debug("Header rotation: Rotate left");
            return ROTATE_LEFT;
        case 2:
            Debug("Header rotation: Rotate right");
            return ROTATE_RIGHT;
        default:
            Debug("Invalid rotation value in header: %d", rotation);
            return NO_ROTATION;
    }
}

Cartridge::GLYNX_Cartridge_EEPROM Cartridge::ReadHeaderEEPROM(u8 eeprom)
{
    switch (eeprom)
    {
        case 0:
            Debug("Header EEPROM: No EEPROM");
            return NO_EEPROM;
        case 1:
            Debug("Header EEPROM: 93C46");
            return EEPROM_93C46;
        case 2:
            Debug("Header EEPROM: 93C56");
            return EEPROM_93C56;
        case 3:
            Debug("Header EEPROM: 93C66");
            return EEPROM_93C66;
        case 4:
            Debug("Header EEPROM: 93C76");
            return EEPROM_93C76;
        case 5:
            Debug("Header EEPROM: 93C86");
            return EEPROM_93C86;
        case 0x40:
            Debug("Header EEPROM: SD");
            return EEPROM_SD;
        case 0x80:
            Debug("Header EEPROM: 8-bit");
            return EEPROM_8BIT;
        default:
            Debug("Invalid EEPROM value in header: %d", eeprom);
            return NO_EEPROM;
    }
}

bool Cartridge::IsValidFile(const char* path)
{
    using namespace std;

    if (!IsValidPointer(path))
    {
        Error("Invalid path %s", path);
        return false;
    }

    ifstream file(path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = static_cast<int> (file.tellg());

        if (size <= 0)
        {
            Error("Unable to open file %s. Size: %d", path, size);
            file.close();
            return false;
        }

        if (file.bad() || file.fail() || !file.good() || file.eof())
        {
            Error("Unable to open file %s. Bad file!", path);
            file.close();
            return false;
        }

        file.close();
        return true;
    }
    else
    {
        Error("Unable to open file %s", path);
        return false;
    }
}
