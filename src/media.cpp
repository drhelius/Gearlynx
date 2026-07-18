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
#include "game_drive.h"
#include "miniz.h"
#include "crc.h"
#include "game_db.h"
#include "state_serializer.h"

Media::Media()
{
    InitPointer(m_rom);
    for (int i = 0; i < CART_BANK_COUNT; i++)
    {
        InitPointer(m_cart_bank_data[i]);
        InitPointer(m_cart_bank_ram[i]);
    }
    InitPointer(m_persistent_ram);
    InitPointer(m_nvram);
    InitPointer(m_eeprom_instance);
    InitPointer(m_game_drive_instance);
    InitPointer(m_decrypt_buffer_a);
    InitPointer(m_decrypt_buffer_b);
    InitPointer(m_decrypt_buffer_tmp);
    InitPointer(m_decrypt_buffer_sub);
    m_is_bios_loaded = false;
    m_is_bios_valid = false;
    m_forced_rotation = GLYNX_ROTATION_AUTO;
    m_forced_console_type = GLYNX_CONSOLE_AUTO;
    m_forced_eeprom = GLYNX_EEPROM_NONE;
    m_eeprom_forced = false;
    HardReset();
}

Media::~Media()
{
    ReleaseCartBankRAM();
    SafeDeleteArray(m_rom);
    SafeDeleteArray(m_nvram);
    SafeDelete(m_eeprom_instance);
    SafeDelete(m_game_drive_instance);
    SafeDeleteArray(m_decrypt_buffer_a);
    SafeDeleteArray(m_decrypt_buffer_b);
    SafeDeleteArray(m_decrypt_buffer_tmp);
    SafeDeleteArray(m_decrypt_buffer_sub);
}

void Media::Init()
{
    m_eeprom_instance = new EEPROM();
    m_game_drive_instance = new GameDrive();
    m_nvram = new u8[NVRAM_SIZE];
    memset(m_nvram, 0, NVRAM_SIZE);
    m_decrypt_buffer_a = new u8[EPYX_DECRYPT_BLOCK_SIZE];
    m_decrypt_buffer_b = new u8[EPYX_DECRYPT_BLOCK_SIZE];
    m_decrypt_buffer_tmp = new u8[EPYX_DECRYPT_BLOCK_SIZE];
    m_decrypt_buffer_sub = new u8[EPYX_DECRYPT_BLOCK_SIZE];
    HardReset();
}

void Media::Reset()
{
    m_address_shift = 0;
    m_page_offset = 0;
    m_shift_register_strobe = false;
    m_shift_register_bit = false;
    memset(m_nvram, 0, NVRAM_SIZE);
    ApplyEEPROMConfiguration();
}

void Media::HardReset()
{
    if (m_game_drive_instance)
        m_game_drive_instance->Configure(false, NULL);

    ReleaseCartBankRAM();
    SafeDeleteArray(m_rom);
    m_rom_size = 0;
    m_ready = false;
    m_is_in_game_database = false;
    m_file_path[0] = 0;
    m_file_directory[0] = 0;
    m_file_name[0] = 0;
    m_file_extension[0] = 0;
    m_header_name[0] = 0;
    m_header_manufacturer[0] = 0;
    ClearCartBanks();
    m_bank_page_size[0] = 0;
    m_bank_page_size[1] = 0;
    m_address_shift = 0;
    m_page_offset = 0;
    memset(m_lnx2_bank, 0, sizeof(m_lnx2_bank));
    m_required_rom_size = 0;
    m_shift_register_strobe = false;
    m_shift_register_bit = false;
    m_nvram_enabled = false;
    m_is_lnx2 = false;
    m_missing_header = false;
    m_save_memory_dirty = false;
    m_rotation = GLYNX_ROTATION_AUTO;
    m_console_type = GLYNX_CONSOLE_AUTO;
    m_eeprom = GLYNX_EEPROM_NONE;
    m_active_eeprom = GLYNX_EEPROM_NONE;
    m_type = MEDIA_LYNX;
    m_audin = false;
    m_audin_value = false;
    m_homebrew_boot_address = 0;
    m_homebrew_size = 0;
    m_epyx_headerless = 0;
    m_crc = 0;
}

void Media::ReleaseCartBankRAM()
{
    for (int i = 0; i < CART_BANK_COUNT; i++)
    {
        SafeDeleteArray(m_cart_bank_ram[i]);
    }

    SafeDeleteArray(m_persistent_ram);
    m_persistent_ram_size = 0;
}

void Media::ClearCartBanks()
{
    for (int i = 0; i < CART_BANK_COUNT; i++)
    {
        InitPointer(m_cart_bank_data[i]);
        InitPointer(m_cart_bank_ram[i]);
        m_cart_bank_size[i] = 0;
        m_cart_bank_mask[i] = 0;
        m_cart_bank_block_size[i] = 0;
        m_cart_bank_block_count[i] = 0;
        m_cart_bank_type[i] = GLYNX_CART_BANK_UNUSED;
        m_address_shift_bits[i] = 0;
        m_page_offset_mask[i] = 0;
    }
}

void Media::SetupCartBank(int bank, GLYNX_Cartridge_Bank_Type type, u32 block_size, u32 block_count, u8* data)
{
    if (bank < 0 || bank >= CART_BANK_COUNT || type == GLYNX_CART_BANK_UNUSED || block_size == 0 || block_count == 0)
        return;

    u32 total_size = block_size * block_count;
    m_cart_bank_type[bank] = type;
    m_cart_bank_size[bank] = total_size;
    m_cart_bank_mask[bank] = total_size - 1;
    m_cart_bank_block_size[bank] = block_size;
    m_cart_bank_block_count[bank] = block_count;

    u32 shift = 0;
    u32 ps = block_size;
    while (ps >>= 1)
        shift++;

    m_address_shift_bits[bank] = shift;
    m_page_offset_mask[bank] = (1u << shift) - 1;

    m_cart_bank_data[bank] = data;

    Debug("%s: Type: %d, Block size: %d, Blocks: %d, Total size: %d bytes", GetCartBankName(bank), type, block_size, block_count, total_size);
    Debug("%s: Address shift bits: %d, Page offset mask: 0x%X, Bank mask: 0x%X", GetCartBankName(bank), m_address_shift_bits[bank], m_page_offset_mask[bank], m_cart_bank_mask[bank]);
}

u32 Media::SetupClassicBanks()
{
    u32 required_size = 0;

    u16 bank0_page_size = (u16)m_bank_page_size[0];
    if (bank0_page_size > 0)
    {
        u32 size = bank0_page_size * 256;
        SetupCartBank(CART_BANK_0, GLYNX_CART_BANK_ROM, bank0_page_size, 256, (m_rom_size >= size) ? m_rom : NULL);
        required_size = MAX(required_size, size);
    }
    else
    {
        Debug("Unknown page size for bank0");
    }

    u16 bank1_page_size = (u16)m_bank_page_size[1];
    if (bank1_page_size > 0)
    {
        u32 offset = m_cart_bank_size[CART_BANK_0];
        u32 size = bank1_page_size * 256;
        SetupCartBank(CART_BANK_1, GLYNX_CART_BANK_ROM, bank1_page_size, 256, (m_rom_size >= offset + size) ? (m_rom + offset) : NULL);
        required_size = MAX(required_size, offset + size);
    }
    else if (m_nvram_enabled)
    {
        SetupCartBank(CART_BANK_1, GLYNX_CART_BANK_RAM_PERSISTENT, 32, NVRAM_SIZE / 32, m_nvram);
        m_cart_bank_data[CART_BANK_1] = m_nvram;
        SafeDeleteArray(m_cart_bank_ram[CART_BANK_1]);
        required_size = MAX(required_size, m_cart_bank_size[CART_BANK_0] + m_cart_bank_size[CART_BANK_1]);
        Debug("Using 8KB NVRAM for bank1");
    }
    else
    {
        Debug("Bank1 not available (no ROM data, no NVRAM)");
    }

    if (m_audin)
    {
        u32 bank1_rom_size = bank1_page_size * 256;
        u32 offset = m_cart_bank_size[CART_BANK_0] + bank1_rom_size;

        if (m_cart_bank_size[CART_BANK_0] > 0 && m_rom_size > offset)
        {
            u32 size = m_cart_bank_size[CART_BANK_0];
            SetupCartBank(CART_BANK_0_A, GLYNX_CART_BANK_ROM, m_cart_bank_block_size[CART_BANK_0], 256, (m_rom_size >= offset + size) ? (m_rom + offset) : NULL);
            required_size = MAX(required_size, offset + m_cart_bank_size[CART_BANK_0_A]);
        }

        offset += m_cart_bank_size[CART_BANK_0];
        if (bank1_page_size > 0 && m_rom_size > offset)
        {
            u32 size = bank1_page_size * 256;
            SetupCartBank(CART_BANK_1_A, GLYNX_CART_BANK_ROM, bank1_page_size, 256, (m_rom_size >= offset + size) ? (m_rom + offset) : NULL);
            required_size = MAX(required_size, offset + m_cart_bank_size[CART_BANK_1_A]);
        }
    }

    return required_size;
}

u32 Media::SetupLynx2Banks()
{
    u32 offset = 0;
    u32 persistent_offset = 0;

    for (int i = 0; i < CART_BANK_COUNT; i++)
    {
        u8 descriptor = m_lnx2_bank[i];
        GLYNX_Cartridge_Bank_Type type = (GLYNX_Cartridge_Bank_Type)(descriptor >> 6);

        if (type != GLYNX_CART_BANK_RAM_PERSISTENT)
            continue;

        u32 block_count = 2u << ((descriptor >> 3) & 0x07);
        u32 block_size = 16u << (descriptor & 0x07);
        m_persistent_ram_size += block_size * block_count;
    }

    if (m_persistent_ram_size > 0)
    {
        m_persistent_ram = new u8[m_persistent_ram_size];
        memset(m_persistent_ram, 0xFF, m_persistent_ram_size);
    }

    for (int i = 0; i < CART_BANK_COUNT; i++)
    {
        u8 descriptor = m_lnx2_bank[i];
        GLYNX_Cartridge_Bank_Type type = (GLYNX_Cartridge_Bank_Type)(descriptor >> 6);

        if (type == GLYNX_CART_BANK_UNUSED)
            continue;

        u32 block_count = 2u << ((descriptor >> 3) & 0x07);
        u32 block_size = 16u << (descriptor & 0x07);
        u32 size = block_size * block_count;
        u8* rom_data = (m_rom_size >= offset + size) ? (m_rom + offset) : NULL;
        u8* data = rom_data;

        if (type == GLYNX_CART_BANK_RAM)
        {
            m_cart_bank_ram[i] = new u8[size];
            if (rom_data != NULL)
                memcpy(m_cart_bank_ram[i], rom_data, size);
            else
                memset(m_cart_bank_ram[i], 0xFF, size);
            data = m_cart_bank_ram[i];
        }
        else if (type == GLYNX_CART_BANK_RAM_PERSISTENT)
        {
            data = m_persistent_ram + persistent_offset;
            if (rom_data != NULL)
                memcpy(data, rom_data, size);
            persistent_offset += size;
        }

        SetupCartBank(i, type, block_size, block_count, data);
        offset += size;
    }

    return offset;
}

void Media::SetupBanks()
{
    ReleaseCartBankRAM();
    ClearCartBanks();

    if (m_is_lnx2)
        m_required_rom_size = SetupLynx2Banks();
    else
        m_required_rom_size = SetupClassicBanks();
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

    if (file.is_open())
    {
        int size = (int)(file.tellg());
        if (size > 0)
        {
            char* buffer = new char[size];
            file.seekg(0, ios::beg);

            if (file.read(buffer, size))
            {
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
            }
            else
            {
                Error("There was a problem reading the file %s...", path);
                m_ready = false;
            }

            SafeDeleteArray(buffer);
        }
        else
        {
            Error("Invalid file size %d for file %s...", size, path);
            m_ready = false;
        }

        file.close();
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

    if ((m_rom_size > 0x40) && GatherLynx2Header(buffer))
    {
        m_rom_size -= 0x40;
        buffer += 0x40;
        m_type = MEDIA_LYNX;
    }
    else if ((m_rom_size > 0x40) && GatherLynxHeader(buffer))
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

    u32 loaded_rom_size = m_rom_size;
    m_rom = new u8[m_rom_size];
    memcpy(m_rom, buffer, m_rom_size);

    m_crc = CalculateCRC32(0, m_rom, m_rom_size);
    Log("ROM CRC32: %08X", m_crc);

    GatherInfoFromDB();

    if (m_rom_size > loaded_rom_size)
    {
        Debug("ROM buffer too small (%d bytes) for database size (%d bytes), padding with 0xFF", loaded_rom_size, m_rom_size);
        u8* padded = new u8[m_rom_size];
        memcpy(padded, m_rom, loaded_rom_size);
        memset(padded + loaded_rom_size, 0xFF, m_rom_size - loaded_rom_size);
        SafeDeleteArray(m_rom);
        m_rom = padded;
    }

    if (m_type == MEDIA_LYNX)
    {
        SetupBanks();

        u32 required_size = m_required_rom_size;

        if (required_size > m_rom_size)
        {
            Debug("ROM buffer too small (%d bytes) for banks (%d bytes), padding with 0xFF", m_rom_size, required_size);
            u8* padded = new u8[required_size];
            memcpy(padded, m_rom, m_rom_size);
            memset(padded + m_rom_size, 0xFF, required_size - m_rom_size);
            SafeDeleteArray(m_rom);
            m_rom = padded;
            m_rom_size = required_size;
            SetupBanks();
        }
    }

    m_epyx_headerless = DetectEpyxHeaderless();
    ApplyEEPROMConfiguration();

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
    if (!file.good() || file.gcount() != size)
    {
        Error("Failed to load BIOS data: %s", path);
        file.close();
        return BIOS_LOAD_FILE_ERROR;
    }
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

    mz_zip_reader_end(&zip_archive);
    return false;
}

void Media::GatherInfoFromDB()
{
    int i = 0;
    m_is_in_game_database = false;

    while(!m_is_in_game_database && (k_game_database[i].title != 0))
    {
        u32 db_crc = k_game_database[i].crc;

        if (db_crc == m_crc)
        {
            m_is_in_game_database = true;
            Log("ROM found in database: %s. CRC: %08X", k_game_database[i].title, m_crc);

            if (m_is_lnx2)
            {
                Debug("Skipping database media overrides for LNX2 ROM");
            }
            else if (m_rom_size == k_game_database[i].file_size)
            {
                Debug("ROM size matches database: %d bytes", m_rom_size);
            }
            else
            {
                Log("WARNING: ROM size mismatch. Database: %d bytes, ROM: %d bytes", k_game_database[i].file_size, m_rom_size);
                Debug("Forcing ROM size to database value");
                m_rom_size = k_game_database[i].file_size;
            }

            if (!m_is_lnx2 && k_game_database[i].bank0_page_size != 0)
            {
                Debug("Forcing bank0 page size to database value: %d", k_game_database[i].bank0_page_size);
                m_bank_page_size[0] = k_game_database[i].bank0_page_size;
            }

            if (!m_is_lnx2)
            {
                Debug("Forcing bank1 page size to database value: %d", k_game_database[i].bank1_page_size);
                m_bank_page_size[1] = k_game_database[i].bank1_page_size;
            }

            if (!m_is_lnx2 && (k_game_database[i].flags & GLYNX_DB_FLAG_ROTATE_LEFT))
            {
                Debug("Forcing rotation to database value: Rotate LEFT");
                m_rotation = GLYNX_ROTATION_LEFT;
            }
            else if (!m_is_lnx2 && (k_game_database[i].flags & GLYNX_DB_FLAG_ROTATE_RIGHT))
            {
                Debug("Forcing rotation to database value: Rotate RIGHT");
                m_rotation = GLYNX_ROTATION_RIGHT;
            }

            if (!m_is_lnx2 && (k_game_database[i].flags & GLYNX_DB_FLAG_AUDIN))
            {
                Debug("Forcing AUDIN to database value: TRUE");
                m_audin = true;
            }

            if (!m_is_lnx2 && (k_game_database[i].flags & GLYNX_DB_FLAG_EEPROM_93C46))
            {
                Debug("Forcing EEPROM to database value: 93C46");
                m_eeprom = GLYNX_EEPROM_93C46;
            }

            if (!m_is_lnx2 && (k_game_database[i].flags & GLYNX_DB_FLAG_NVRAM_8KB))
            {
                Debug("Enabling 8KB NVRAM in bank1");
                m_nvram_enabled = true;
            }

            if (!m_is_lnx2 && k_game_database[i].console_type != GLYNX_CONSOLE_AUTO)
            {
                Debug("Forcing console type to database value: %s", k_game_database[i].console_type == GLYNX_CONSOLE_MODEL_I ? "Lynx I" : "Lynx II");
                m_console_type = k_game_database[i].console_type;
            }
        }
        else
            i++;
    }

    if (!m_is_in_game_database)
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
        Log("Invalid LYNX header magic: %c%c%c%c", p[0], p[1], p[2], p[3]);
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
        Log("Invalid LYNX header version: %d", header.version);
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

bool Media::GatherLynx2Header(const u8* buffer)
{
    const u8* p = buffer;
    GLYNX_Cartridge_Header_LNX2 header;

    memset(&header, 0, sizeof(header));

    if (p[0] != 'L' || p[1] != 'N' || p[2] != 'X' || p[3] != '2')
        return false;

    Debug("LNX2 Header found");

    memcpy(header.magic, p, 4);
    p += 4;

    for (int i = 0; i < CART_BANK_COUNT; i++)
    {
        header.bank[i] = *p;
        m_lnx2_bank[i] = header.bank[i];
        p++;
    }

    header.version = read_u16_le(p);
    p += 2;

    if (header.version != 1)
    {
        Log("Invalid LNX2 header version: %d", header.version);
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

    header.flags = *p;
    p++;

    header.eeprom = *p;

    m_is_lnx2 = true;
    m_rotation = ReadHeaderRotation(header.rotation);
    m_audin = ((m_lnx2_bank[CART_BANK_0_A] >> 6) != GLYNX_CART_BANK_UNUSED) || ((m_lnx2_bank[CART_BANK_1_A] >> 6) != GLYNX_CART_BANK_UNUSED);
    m_eeprom = ReadHeaderEEPROM(header.eeprom);
    if (header.flags & 0x01)
        m_console_type = GLYNX_CONSOLE_MODEL_II;

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
        Debug("WARNING: Invalid BS93 header magic: %c%c", header.magic[0], header.magic[1]);
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
        Log("Invalid BS93 header string: %c%c%c%c", header.bs93[0], header.bs93[1], header.bs93[2], header.bs93[3]);
        return false;
    }

    Debug("BS93 Header found");

    if (header.size <= 10)
    {
        Log("Invalid BS93 header size: %d", header.size);
        return false;
    }

    m_homebrew_boot_address = header.boot_address;
    m_homebrew_size = header.size - 10;

    return true;
}

void Media::DefaultLynxHeader()
{
    Log("Using default header values");

    m_missing_header = true;
    m_bank_page_size[0] = (m_rom_size + 255) >> 8;
    m_bank_page_size[1] = 0;
    m_rotation = GLYNX_ROTATION_AUTO;
    m_audin = false;
    m_audin_value = false;
    m_eeprom = GLYNX_EEPROM_NONE;
}

int Media::DetectEpyxHeaderless()
{
    if (m_cart_bank_data[CART_BANK_0] == NULL || m_cart_bank_size[CART_BANK_0] < (u32)EPYX_HEADER_OLD)
        return 0;

    int headerless = EPYX_HEADER_OLD;

    for (int i = 0; i < EPYX_HEADER_OLD; i++)
    {
        u8 data = m_cart_bank_data[CART_BANK_0][i & m_cart_bank_mask[CART_BANK_0]];

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
        m_homebrew_boot_address = 0x200;
    }

    return headerless;
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
    u8 base_type = eeprom & 0x07;
    u8 flags = eeprom & (GLYNX_EEPROM_SD | GLYNX_EEPROM_8BIT);
    u8 unknown_bits = eeprom & ~(0x07 | GLYNX_EEPROM_SD | GLYNX_EEPROM_8BIT);

    if (eeprom == 0)
    {
        Debug("Header EEPROM: No EEPROM");
        return GLYNX_EEPROM_NONE;
    }

    if (unknown_bits != 0)
    {
        Debug("Invalid EEPROM value in header: %d", eeprom);
        return GLYNX_EEPROM_NONE;
    }

    if (base_type == GLYNX_EEPROM_NONE)
    {
        if (eeprom & GLYNX_EEPROM_8BIT)
            base_type = GLYNX_EEPROM_93C46;
        else if (eeprom == GLYNX_EEPROM_SD)
        {
            Debug("Header EEPROM: SD");
            return GLYNX_EEPROM_SD;
        }
        else
        {
            Debug("Invalid EEPROM value in header: %d", eeprom);
            return GLYNX_EEPROM_NONE;
        }
    }

    switch (base_type)
    {
        case GLYNX_EEPROM_93C46:
            Debug("Header EEPROM: 93C46%s%s", (flags & GLYNX_EEPROM_SD) ? " SD" : "", (flags & GLYNX_EEPROM_8BIT) ? " 8-bit" : "");
            break;
        case GLYNX_EEPROM_93C56:
            Debug("Header EEPROM: 93C56%s%s", (flags & GLYNX_EEPROM_SD) ? " SD" : "", (flags & GLYNX_EEPROM_8BIT) ? " 8-bit" : "");
            break;
        case GLYNX_EEPROM_93C66:
            Debug("Header EEPROM: 93C66%s%s", (flags & GLYNX_EEPROM_SD) ? " SD" : "", (flags & GLYNX_EEPROM_8BIT) ? " 8-bit" : "");
            break;
        case GLYNX_EEPROM_93C76:
            Debug("Header EEPROM: 93C76%s%s", (flags & GLYNX_EEPROM_SD) ? " SD" : "", (flags & GLYNX_EEPROM_8BIT) ? " 8-bit" : "");
            break;
        case GLYNX_EEPROM_93C86:
            Debug("Header EEPROM: 93C86%s%s", (flags & GLYNX_EEPROM_SD) ? " SD" : "", (flags & GLYNX_EEPROM_8BIT) ? " 8-bit" : "");
            break;
        default:
            Debug("Invalid EEPROM value in header: %d", eeprom);
            return GLYNX_EEPROM_NONE;
    }

    return (GLYNX_EEPROM)(base_type | flags);
}

void Media::ApplyEEPROMConfiguration()
{
    GLYNX_EEPROM previous_eeprom = m_active_eeprom;
    m_active_eeprom = m_eeprom_forced ? m_forced_eeprom : m_eeprom;
    m_eeprom_instance->Reset(m_active_eeprom);

    if ((previous_eeprom & GLYNX_EEPROM_SD) != (m_active_eeprom & GLYNX_EEPROM_SD))
        m_game_drive_instance->Configure((m_active_eeprom & GLYNX_EEPROM_SD) != 0, m_file_directory);
    else
        m_game_drive_instance->Reset(false);
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
    Serialize(serializer, GLYNX_SAVESTATE_VERSION);
    if (m_active_eeprom != GLYNX_EEPROM_NONE)
        m_eeprom_instance->SaveState(stream);
    if (m_game_drive_instance->IsAvailable())
        m_game_drive_instance->SaveState(stream);
}

void Media::LoadState(std::istream& stream, int version)
{
    StateSerializer serializer(stream);
    Serialize(serializer, version);
    if (m_active_eeprom != GLYNX_EEPROM_NONE)
        m_eeprom_instance->LoadState(stream);
    if (m_game_drive_instance->IsAvailable())
    {
        if (version >= 17)
            m_game_drive_instance->LoadState(stream);
        else
            m_game_drive_instance->Reset(false);
    }
    if (m_persistent_ram_size > 0)
        m_save_memory_dirty = true;
}

void Media::Serialize(StateSerializer& s, int version)
{
    G_SERIALIZE(s, m_address_shift);
    G_SERIALIZE(s, m_page_offset);
    G_SERIALIZE(s, m_shift_register_strobe);
    G_SERIALIZE(s, m_shift_register_bit);
    G_SERIALIZE(s, m_audin_value);

    if (version >= 15)
    {
        for (int i = 0; i < CART_BANK_COUNT; i++)
        {
            if (IsCartBankWritable(i) && m_cart_bank_data[i] != NULL && m_cart_bank_size[i] > 0)
                G_SERIALIZE_ARRAY(s, m_cart_bank_data[i], m_cart_bank_size[i]);
        }
    }
    else if (IsCartBankWritable(CART_BANK_1) && m_cart_bank_data[CART_BANK_1] != NULL && m_cart_bank_size[CART_BANK_1] > 0)
    {
        G_SERIALIZE_ARRAY(s, m_cart_bank_data[CART_BANK_1], m_cart_bank_size[CART_BANK_1]);
    }
}

// EPYX decryption based on Wookie's work:
// http://atariage.com/forums/topic/129030-lynx-encryption/

static const u8 k_epyx_public_mod[EPYX_DECRYPT_BLOCK_SIZE] =
{
    0x35, 0xB5, 0xA3, 0x94, 0x28, 0x06, 0xD8, 0xA2, 0x26, 0x95,
    0xD7, 0x71, 0xB2, 0x3C, 0xFD, 0x56, 0x1C, 0x4A, 0x19, 0xB6,
    0xA3, 0xB0, 0x26, 0x00, 0x36, 0x5A, 0x30, 0x6E, 0x3C, 0x4D,
    0x63, 0x38, 0x1B, 0xD4, 0x1C, 0x13, 0x64, 0x89, 0x36, 0x4C,
    0xF2, 0xBA, 0x2A, 0x58, 0xF4, 0xFE, 0xE1, 0xFD, 0xAC, 0x7E,
    0x79
};

void Media::DecryptDoubleValue(u8* result, int length)
{
    int x = 0;
    for (int i = length - 1; i >= 0; i--)
    {
        x += 2 * result[i];
        result[i] = (u8)(x & 0xFF);
        x >>= 8;
    }
}

int Media::DecryptMinusEquals(u8* result, const u8* value, int length)
{
    int x = 0;

    for (int i = length - 1; i >= 0; i--)
    {
        x += result[i] - value[i];
        m_decrypt_buffer_sub[i] = (u8)(x & 0xFF);
        x >>= 8;
    }

    if (x >= 0)
    {
        memcpy(result, m_decrypt_buffer_sub, length);
        return 1;
    }

    return 0;
}

void Media::DecryptPlusEquals(u8* result, const u8* value, int length)
{
    int carry = 0;
    for (int i = length - 1; i >= 0; i--)
    {
        int tmp = result[i] + value[i] + carry;
        carry = (tmp >= 256) ? 1 : 0;
        result[i] = (u8)tmp;
    }
}

void Media::DecryptMontgomery(u8* L, const u8* M, const u8* N, const u8* modulus, int length)
{
    memset(L, 0, length);

    for (int i = 0; i < length; i++)
    {
        u8 tmp = N[i];

        for (int j = 0; j < 8; j++)
        {
            DecryptDoubleValue(L, length);

            u8 increment = (tmp & 0x80) >> 7;
            tmp <<= 1;

            if (increment != 0)
            {
                DecryptPlusEquals(L, M, length);
                int carry = DecryptMinusEquals(L, modulus, length);
                if (carry != 0)
                    DecryptMinusEquals(L, modulus, length);
            }
            else
            {
                DecryptMinusEquals(L, modulus, length);
            }
        }
    }
}

int Media::DecryptBlock(int accumulator, u8* result, const u8* encrypted, int length)
{
    memset(m_decrypt_buffer_a, 0, length);
    memset(m_decrypt_buffer_b, 0, length);
    memset(m_decrypt_buffer_tmp, 0, length);

    // Copy encrypted block in reverse order
    for (int i = length - 1; i >= 0; i--)
        m_decrypt_buffer_b[i] = encrypted[length - 1 - i];

    // Montgomery multiplication: A = B^2 mod modulus
    DecryptMontgomery(m_decrypt_buffer_a, m_decrypt_buffer_b, m_decrypt_buffer_b, k_epyx_public_mod, length);

    // TMP = B^2
    memcpy(m_decrypt_buffer_tmp, m_decrypt_buffer_a, length);

    // Montgomery multiplication: A = B^3 mod modulus
    DecryptMontgomery(m_decrypt_buffer_a, m_decrypt_buffer_b, m_decrypt_buffer_tmp, k_epyx_public_mod, length);

    // Accumulate and output decrypted bytes
    for (int i = length - 1; i > 0; i--)
    {
        accumulator += m_decrypt_buffer_a[i];
        accumulator &= 0xFF;
        *result = (u8)accumulator;
        result++;
    }

    return accumulator;
}

int Media::DecryptFrame(u8* result, const u8* encrypted, int length)
{
    int accumulator = 0;
    int blocks = 256 - encrypted[0];

    const u8* eptr = encrypted + 1;
    u8* rptr = result;

    for (int i = 0; i < blocks; i++)
    {
        accumulator = DecryptBlock(accumulator, rptr, eptr, length);
        rptr += (length - 1);
        eptr += length;
    }

    return blocks;
}

int Media::DecryptEpyxLoader(u8* output, int max_size)
{
    if (m_type != MEDIA_EPYX_HEADERLESS || m_epyx_headerless == 0)
        return 0;

    if (m_cart_bank_data[CART_BANK_0] == NULL || m_cart_bank_size[CART_BANK_0] == 0)
        return 0;

    Debug("Decrypting EPYX loader...");

    // Read encrypted data from cart starting after header zeros
    u8 encrypted[256];
    u8 decrypted[256];
    int read_offset = m_epyx_headerless;

    // First byte is block count (inverted)
    encrypted[0] = m_cart_bank_data[CART_BANK_0][read_offset & m_cart_bank_mask[CART_BANK_0]];
    int block_count = 256 - encrypted[0];

    Debug("EPYX loader: %d encrypted blocks", block_count);

    if (block_count <= 0 || block_count > 5)
    {
        Error("Invalid EPYX block count: %d", block_count);
        return 0;
    }

    // Read encrypted blocks (51 bytes per block)
    int encrypted_size = 1 + (EPYX_DECRYPT_BLOCK_SIZE * block_count);
    for (int i = 1; i < encrypted_size; i++)
    {
        encrypted[i] = m_cart_bank_data[CART_BANK_0][(read_offset + i) & m_cart_bank_mask[CART_BANK_0]];
    }

    // Decrypt the frame
    DecryptFrame(decrypted, encrypted, EPYX_DECRYPT_BLOCK_SIZE);

    // Calculate decrypted size (50 bytes per block, since we skip one byte per block)
    int decrypted_size = (EPYX_DECRYPT_BLOCK_SIZE - 1) * block_count;

    if (decrypted_size > max_size)
    {
        Error("EPYX decrypted size %d exceeds buffer %d", decrypted_size, max_size);
        decrypted_size = max_size;
    }

    memcpy(output, decrypted, decrypted_size);

    Debug("EPYX loader decrypted: %d bytes", decrypted_size);

    return decrypted_size;
}

void Media::ClearSaveMemoryDirty()
{
    if (m_eeprom_instance && m_eeprom_instance->IsAvailable())
        m_eeprom_instance->ClearDirty();
    m_save_memory_dirty = false;
}

bool Media::SaveRam(std::ostream& file)
{
    Debug("Saving RAM to stream");

    s32 size = GetSaveMemorySize();
    u8* data = GetSaveMemoryPointer();

    if (size > 0 && data != NULL)
    {
        file.write(reinterpret_cast<const char*>(data), size);
        if (!file.good())
        {
            Error("Failed to save RAM to stream");
            return false;
        }

        return true;
    }

    return false;
}

bool Media::LoadRam(std::istream& file, s32 file_size)
{
    Debug("Loading RAM from stream");

    s32 size = GetSaveMemorySize();
    u8* data = GetSaveMemoryPointer();

    if (size <= 0 || data == NULL)
    {
        Log("No save memory available");
        return false;
    }

    if (file_size != size)
    {
        Log("Invalid RAM size: %d (expected %d)", file_size, size);
        return false;
    }

    file.read(reinterpret_cast<char*>(data), size);
    if (!file.good())
    {
        Error("Failed to load RAM from stream");
        return false;
    }

    return true;
}
