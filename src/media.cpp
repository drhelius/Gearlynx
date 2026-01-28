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
#include "media.h"
#include "eeprom.h"
#include "miniz.h"
#include "crc.h"
#include "game_db.h"
#include "state_serializer.h"

Media::Media()
{
    InitPointer(m_rom);
    InitPointer(m_bank_data[0]);
    InitPointer(m_bank_data[1]);
    InitPointer(m_shadow_ram);
    InitPointer(m_eeprom_instance);
    m_is_bios_loaded = false;
    m_is_bios_valid = false;
    m_forced_rotation = GLYNX_ROTATION_AUTO;
    m_forced_console_type = GLYNX_CONSOLE_AUTO;
    HardReset();
}

Media::~Media()
{
    SafeDeleteArray(m_rom);
    SafeDeleteArray(m_shadow_ram);
    SafeDelete(m_eeprom_instance);
}

void Media::Init()
{
    m_eeprom_instance = new EEPROM();
    m_shadow_ram = new u8[SHADOW_RAM_SIZE];
    memset(m_shadow_ram, 0, SHADOW_RAM_SIZE);
    HardReset();
}

void Media::Reset()
{
    m_address_shift = 0;
    m_page_offset = 0;
    m_shift_register_strobe = false;
    m_shift_register_bit = false;
    m_bank1_write_enable = false;
    m_eeprom_instance->SetType(m_eeprom);
    m_eeprom_instance->Reset();
    memset(m_shadow_ram, 0, SHADOW_RAM_SIZE);
}

void Media::HardReset()
{
    SafeDeleteArray(m_rom);
    m_rom_size = 0;
    m_ready = false;
    m_file_path[0] = 0;
    m_file_directory[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_header_name[0] = 0;
    m_header_manufacturer[0] = 0;
    InitPointer(m_bank_data[0]);
    InitPointer(m_bank_data[1]);
    InitPointer(m_bank_data_a[0]);
    InitPointer(m_bank_data_a[1]);
    m_bank_size[0] = 0;
    m_bank_size[1] = 0;
    m_bank_mask[0] = 0;
    m_bank_mask[1] = 0;
    m_bank_page_size[0] = 0;
    m_bank_page_size[1] = 0;
    m_address_shift = 0;
    m_address_shift_bits[0] = 0;
    m_address_shift_bits[1] = 0;
    m_page_offset = 0;
    m_page_offset_mask[0] = 0;
    m_page_offset_mask[1] = 0;
    m_shift_register_strobe = false;
    m_shift_register_bit = false;
    m_bank1_is_ram = false;
    m_bank1_dirty = false;
    m_bank1_write_enable = false;
    m_rotation = GLYNX_ROTATION_AUTO;
    m_console_type = GLYNX_CONSOLE_AUTO;
    m_eeprom = GLYNX_EEPROM_NONE;
    m_type = MEDIA_LYNX;
    m_audin = false;
    m_audin_value = false;
    m_homebrew_boot_address = 0;
    m_homebrew_size = 0;
    m_epyx_headerless = 0;
    m_crc = 0;
}

bool Media::LoadFromFile(const char* path)
{
    using namespace std;

    Log("Loading %s...", path);

    if (!IsValidFile(path))
        return false;

    HardReset();

    GatherDataFromPath(path);

    ifstream file;
    open_ifstream_utf8(file, path, ios::in | ios::binary | ios::ate);
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
                m_ready = LoadFromZipFile((u8*)(buffer), size);
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
        HardReset();

    return m_ready;
}

bool Media::LoadFromBuffer(const u8* buffer, int size, const char* path)
{
    Log("Loading ROM from buffer... Size: %d", size);

    if (!IsValidPointer(buffer) || size <= 0)
    {
        Error("Unable to load ROM from buffer: Buffer invalid %p. Size: %d", buffer, size);
        return false;
    }

    HardReset();

    GatherDataFromPath(path);

    m_rom_size = size;

    if ((m_rom_size > 0x40) && GatherLynxHeader(buffer))
    {
        m_rom_size -= 0x40;
        buffer += 0x40;
        m_type = MEDIA_LYNX;
    }
    else if ((m_rom_size > 10) && GatherBS93Header(buffer))
    {
        m_rom_size -= 10;
        buffer += 10;
        m_rom_size = MIN(m_rom_size, m_homebrew_size);
        m_type = MEDIA_HOMEBREW;
    }
    else
    {
        Debug("WARNING: Unable to gather ROM header");
        m_type = MEDIA_LYNX;
        DefaultLynxHeader();
    }

    Log("ROM Size: %d KB, %d bytes (0x%0X)", m_rom_size / 1024, m_rom_size, m_rom_size);

    m_rom = new u8[m_rom_size];
    memcpy(m_rom, buffer, m_rom_size);

    m_crc = CalculateCRC32(0, m_rom, m_rom_size);
    Log("ROM CRC32: %08X", m_crc);

    GatherInfoFromDB();

    if (m_type == MEDIA_LYNX)
        SetupBanks();

    m_ready = true;

    Debug("ROM loaded from buffer. Size: %d bytes", m_rom_size);

    if (!m_ready)
        HardReset();

    return m_ready;
}

GLYNX_Bios_State Media::LoadBios(const char* path)
{
    using namespace std;

    m_is_bios_loaded = false;
    m_is_bios_valid = false;

    if (!IsValidFile(path))
    {
        Error("There was a problem opening the file %s", path);
        return BIOS_LOAD_FILE_ERROR;
    }

    ifstream file;
    open_ifstream_utf8(file, path, ios::in | ios::binary | ios::ate);

    if (!file.is_open())
    {
        Error("There was a problem opening the file %s", path);
        return BIOS_LOAD_FILE_ERROR;
    }

    int size = static_cast<int>(file.tellg());

    if (size != 0x200)
    {
        Error("Incorrect BIOS size %d: %s", size, path);
        file.close();
        return BIOS_LOAD_INVALID_SIZE;
    }

    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(m_bios), size);
    file.close();

    m_is_bios_loaded = true;
    Log("BIOS loaded (%d bytes): %s", size, path);

    m_bios[0x1F8] = 0x00;   // Register 0xFFF8 is RAM
    m_bios[0x1F9] = 0x80;   // Register 0xFFF9 is MAPCTL

    u32 crc = CalculateCRC32(0, m_bios, size);

    m_is_bios_valid = (crc == GLYNX_DB_BIOS_CRC);

    if (m_is_bios_valid)
    {
        Log("BIOS CRC is valid: %08X: %s", crc, path);
        return BIOS_LOAD_OK;
    }
    else
    {
        Log("WARNING: Incorrect BIOS CRC %08X: %s", crc, path);
        return BIOS_LOAD_INVALID_CRC;
    }
}

bool Media::LoadFromZipFile(const u8* buffer, int size)
{
    Debug("Loading from ZIP file... Size: %d", size);

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

        if ((extension == "lnx") || (extension == "lyx") || (extension == "o"))
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

void Media::GatherInfoFromDB()
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
                Log("WARNING: ROM size mismatch. Database: %d bytes, ROM: %d bytes", k_game_database[i].file_size, m_rom_size);
                Debug("Forcing ROM size to database value");
                m_rom_size = k_game_database[i].file_size;
            }

            if (k_game_database[i].bank0_page_size != 0)
            {
                Debug("Forcing bank0 page size to database value: %d", k_game_database[i].bank0_page_size);
                m_bank_page_size[0] = k_game_database[i].bank0_page_size;
            }

            Debug("Forcing bank1 page size to database value: %d", k_game_database[i].bank1_page_size);
            m_bank_page_size[1] = k_game_database[i].bank1_page_size;

            if (k_game_database[i].flags & GLYNX_DB_FLAG_ROTATE_LEFT)
            {
                Debug("Forcing rotation to database value: Rotate LEFT");
                m_rotation = GLYNX_ROTATION_LEFT;
            }
            else if (k_game_database[i].flags & GLYNX_DB_FLAG_ROTATE_RIGHT)
            {
                Debug("Forcing rotation to database value: Rotate RIGHT");
                m_rotation = GLYNX_ROTATION_RIGHT;
            }

            if (k_game_database[i].flags & GLYNX_DB_FLAG_AUDIN)
            {
                Debug("Forcing AUDIN to database value: TRUE");
                m_audin = true;
            }

            if (k_game_database[i].flags & GLYNX_DB_FLAG_EEPROM_93C46)
            {
                Debug("Forcing EEPROM to database value: 93C46");
                m_eeprom = GLYNX_EEPROM_93C46;
            }

            if (k_game_database[i].console_type != GLYNX_CONSOLE_AUTO)
            {
                Debug("Forcing console type to database value: %s", k_game_database[i].console_type == GLYNX_CONSOLE_MODEL_I ? "Lynx I" : "Lynx II");
                m_console_type = k_game_database[i].console_type;
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

bool Media::GatherLynxHeader(const u8* buffer)
{
    const u8* p = buffer;
    GLYNX_Cartridge_Header header;

    memset(&header, 0, sizeof(header));

    if (p[0] != 'L' || p[1] != 'Y' || p[2] != 'N' || p[3] != 'X')
    {
        Error("Invalid LYNX header magic: %c%c%c%c", p[0], p[1], p[2], p[3]);
        return false;
    }

    Debug("LYNX Header found");

    memcpy(header.magic, p, 4);
    p += 4;

    header.bank0_page_size = read_u16_le(p);
    p += 2;
    header.bank1_page_size = read_u16_le(p);
    p += 2;

    header.version = read_u16_le(p);
    p += 2;

    if (header.version != 1)
    {
        Error("Invalid LYNX header version: %d", header.version);
        return false;
    }

    memcpy(header.name, p, 32);
    header.name[31] = 0;
    p += 32;

    memcpy(header.manufacturer, p, 16);
    header.manufacturer[15] = 0;
    p += 16;

    header.rotation = *p;
    p++;

    header.audin = (*p & 0x01) != 0;
    p++;

    header.eeprom = *p;

    m_bank_page_size[0] = header.bank0_page_size;
    m_bank_page_size[1] = header.bank1_page_size;
    m_rotation = ReadHeaderRotation(header.rotation);
    m_audin = (header.audin == 1);
    m_eeprom = ReadHeaderEEPROM(header.eeprom);
    strncpy(m_header_name, header.name, 31);
    m_header_name[31] = 0;
    strncpy(m_header_manufacturer, header.manufacturer, 15);
    m_header_manufacturer[15] = 0;

    return true;
}

bool Media::GatherBS93Header(const u8* buffer)
{
    const u8* p = buffer;
    GLYNX_BS93_Header header;

    memset(&header, 0, sizeof(header));

    memcpy(header.magic, p, 2);
    p += 2;

    if (header.magic[0] != 0x80 || header.magic[1] != 0x08)
    {
        Log("WARNING: Invalid BS93 header magic: %c%c", p[0], p[1]);
    }

    header.boot_address = read_u16_be(p);
    p += 2;

    header.size = read_u16_be(p);
    p += 2;

    memcpy(header.bs93, p, 4);
    p += 4;

    if (header.bs93[0] != 'B' || header.bs93[1] != 'S' 
        || header.bs93[2] != '9' || header.bs93[3] != '3')
    {
        Error("Invalid BS93 header string: %c%c%c%c", p[0], p[1], p[2], p[3]);
        return false;
    }

    Debug("BS93 Header found");

    m_homebrew_boot_address = header.boot_address;
    m_homebrew_size = header.size - 10;

    return true;
}

void Media::DefaultLynxHeader()
{
    Debug("Using default header values");

    m_bank_page_size[0] = (m_rom_size + 255) >> 8;
    m_bank_page_size[1] = 0;
    m_rotation = GLYNX_ROTATION_AUTO;
    m_audin = false;
    m_audin_value = false;
    m_eeprom = GLYNX_EEPROM_NONE;
}

int Media::DetectEpyxHeaderless()
{
    if (m_bank_data[0] == NULL || m_bank_size[0] < (u32)EPYX_HEADER_OLD)
        return 0;

    int headerless = EPYX_HEADER_OLD;

    for (int i = 0; i < EPYX_HEADER_OLD; i++)
    {
        u8 data = m_bank_data[0][i & m_bank_mask[0]];

        if (data != 0x00)
        {
            if (i < EPYX_HEADER_NEW)
            {
                // Less than 410 zeros -> not headerless
                headerless = 0;
                break;
            }
            else
            {
                // At least 410 zeros -> new EPYX type
                headerless = EPYX_HEADER_NEW;
                break;
            }
        }
    }

    if (headerless > 0)
    {
        Log("EPYX headerless cart detected (%d zeros)", headerless);
        m_type = MEDIA_EPYX_HEADERLESS;
    }

    return headerless;
}

void Media::SetupBanks()
{
    // Calculate page size and number of pages for each bank
    for (int bank = 0; bank < 2; bank++)
    {
        u16 page_size = m_bank_page_size[bank];

        if (bank == 1 && page_size == 0)
        {
            Debug("Using shadow RAM for bank1");

            // Shadow RAM: 64K, page size 256
            m_bank_data[1] = m_shadow_ram;
            m_bank_size[1] = SHADOW_RAM_SIZE;
            m_address_shift_bits[1] = 8;
            m_page_offset_mask[1] = 0xFF;
            m_bank_mask[1] = 0xFFFF;
            m_bank1_is_ram = true;
            continue;
        }

        if (page_size == 0)
        {
            Debug("Unknown page size for bank %d", bank);

            InitPointer(m_bank_data[bank]);
            m_bank_size[bank] = 0;
            m_address_shift_bits[bank] = 0;
            m_page_offset_mask[bank] = 0;
            m_bank_mask[bank] = 0;
            continue;
        }

        u32 total_size = page_size * 256;
        m_bank_size[bank] = total_size;

        Debug("Bank %d: Page size: %d, Total size: %d bytes", bank, page_size, total_size);

        if (bank == 0)
        {
            m_bank_data[0] = m_rom;
        }
        else
        {
            m_bank_data[1] = (m_rom_size > m_bank_size[0]) ? (m_rom + m_bank_size[0]) : NULL;
        }

        u32 shift = 0;
        u32 ps = page_size;
        while (ps >>= 1)
            shift++;

        m_address_shift_bits[bank] = shift;
        m_page_offset_mask[bank] = (1u << shift) - 1;
        m_bank_mask[bank] = total_size - 1;

        Debug("Bank %d: Address shift bits: %d, Page offset mask: 0x%X, Bank mask: 0x%X", bank, m_address_shift_bits[bank], m_page_offset_mask[bank], m_bank_mask[bank]);
    }

    // For AUDIN carts, setup alternative banks (Bank0A, Bank1A)
    if (m_audin)
    {
        // Calculate ROM offset for Bank0A
        // Use m_bank_page_size[1] to get actual ROM Bank1 size
        // When Bank1 is shadow RAM (page_size == 0), there is no Bank1 data in ROM
        u32 bank1_rom_size = m_bank_page_size[1] * 256;
        u32 offset = m_bank_size[0] + bank1_rom_size;

        // Bank0A starts after Bank1 ROM data
        if (m_rom_size > offset)
        {
            m_bank_data_a[0] = m_rom + offset;
            Debug("Bank0A: Offset: 0x%X", offset);
        }

        // Bank1A starts after Bank0A (same size as Bank0)
        offset += m_bank_size[0];
        if (m_rom_size > offset)
        {
            m_bank_data_a[1] = m_rom + offset;
            Debug("Bank1A: Offset: 0x%X", offset);
        }
    }

    m_epyx_headerless = DetectEpyxHeaderless();
}

void Media::GatherDataFromPath(const char* path)
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

GLYNX_Rotation Media::ReadHeaderRotation(u8 rotation)
{
    switch (rotation)
    {
        case 0:
            Debug("Header rotation: No rotation");
            return GLYNX_ROTATION_AUTO;
        case 1:
            Debug("Header rotation: Rotate right");
            return GLYNX_ROTATION_RIGHT;
        case 2:
            Debug("Header rotation: Rotate left");
            return GLYNX_ROTATION_LEFT;
        default:
            Debug("Invalid rotation value in header: %d", rotation);
            return GLYNX_ROTATION_AUTO;
    }
}

GLYNX_EEPROM Media::ReadHeaderEEPROM(u8 eeprom)
{
    switch (eeprom)
    {
        case 0:
            Debug("Header EEPROM: No EEPROM");
            return GLYNX_EEPROM_NONE;
        case 1:
            Debug("Header EEPROM: 93C46");
            return GLYNX_EEPROM_93C46;
        case 2:
            Debug("Header EEPROM: 93C56");
            return GLYNX_EEPROM_93C56;
        case 3:
            Debug("Header EEPROM: 93C66");
            return GLYNX_EEPROM_93C66;
        case 4:
            Debug("Header EEPROM: 93C76");
            return GLYNX_EEPROM_93C76;
        case 5:
            Debug("Header EEPROM: 93C86");
            return GLYNX_EEPROM_93C86;
        case 0x40:
            Debug("Header EEPROM: SD");
            return GLYNX_EEPROM_SD;
        case 0x80:
            Debug("Header EEPROM: 8-bit");
            return GLYNX_EEPROM_8BIT;
        default:
            Debug("Invalid EEPROM value in header: %d", eeprom);
            return GLYNX_EEPROM_NONE;
    }
}

bool Media::IsValidFile(const char* path)
{
    using namespace std;

    if (!IsValidPointer(path))
    {
        Error("Invalid path %s", path);
        return false;
    }

    ifstream file;
    open_ifstream_utf8(file, path, ios::in | ios::binary | ios::ate);

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

void Media::SaveState(std::ostream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
    if (m_eeprom != GLYNX_EEPROM_NONE)
        m_eeprom_instance->SaveState(stream);
}

void Media::LoadState(std::istream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
    if (m_eeprom != GLYNX_EEPROM_NONE)
        m_eeprom_instance->LoadState(stream);
}

void Media::Serialize(StateSerializer& s)
{
    G_SERIALIZE(s, m_address_shift);
    G_SERIALIZE(s, m_page_offset);
    G_SERIALIZE(s, m_shift_register_strobe);
    G_SERIALIZE(s, m_shift_register_bit);
    G_SERIALIZE(s, m_bank1_dirty);
    G_SERIALIZE(s, m_bank1_write_enable);
    G_SERIALIZE(s, m_audin_value);

    if (m_bank1_is_ram && m_bank1_dirty && m_bank_data[1] != NULL && m_bank_size[1] > 0)
    {
        G_SERIALIZE_ARRAY(s, m_bank_data[1], m_bank_size[1]);
    }
}
