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

#ifndef MEDIA_H
#define MEDIA_H

#include "common.h"

#define EPYX_HEADER_OLD 512
#define EPYX_HEADER_NEW 410
#define EPYX_DECRYPT_BLOCK_SIZE 51
#define NVRAM_SIZE (8 * 1024) // 8KB

class StateSerializer;
class EEPROM;

class Media
{
public:
    enum GLYNX_Media_Type
    {
        MEDIA_LYNX = 0,
        MEDIA_HOMEBREW = 1,
        MEDIA_EPYX_HEADERLESS = 2
    };

public:
    Media();
    ~Media();
    void Init();
    void Reset();
    void HardReset();
    u8* GetROM();
    u8* GetBIOS();
    bool IsReady();
    bool IsBiosLoaded();
    bool IsBiosValid();
    int GetROMSize();
    u32 GetCRC();
    void ForceRotation(GLYNX_Rotation rotation);
    GLYNX_Rotation GetRotation();
    void ForceConsoleType(GLYNX_Console_Type type);
    GLYNX_Console_Type GetConsoleType();
    GLYNX_EEPROM GetEEPROM();
    GLYNX_Media_Type GetType();
    bool GetAudin();
    u16 GetHomebrewBootAddress();
    int GetEpyxHeaderless();
    int DecryptEpyxLoader(u8* output, int max_size);
    const char* GetFilePath();
    const char* GetFileDirectory();
    const char* GetFileName();
    const char* GetFileExtension();
    const char* GetHeaderName();
    const char* GetHeaderManufacturer();
    u16 GetHeaderBank0PageSize();
    u16 GetHeaderBank1PageSize();
    bool LoadFromFile(const char* path);
    bool LoadFromBuffer(const u8* buffer, int size, const char* path);
    GLYNX_Bios_State LoadBios(const char* path);
    u8 ReadBank0();
    u8 ReadBank1();
    u8 ReadBank0A();
    u8 ReadBank1A();
    u8 PeekBank0();
    u8 PeekBank1();
    u8 PeekBank0A();
    u8 PeekBank1A();
    void WriteBank0(u8 value);
    void WriteBank1(u8 value);
    void WriteBank0A(u8 value);
    void WriteBank1A(u8 value);
    void ShiftRegisterStrobe(bool strobe);
    void ShiftRegisterBit(bool bit);
    void AdvanceCounter();
    u32 GetBankAddress(int bank);
    void SetAudinValue(bool value);
    bool GetAudinValue();
    u16 GetCounterValue();
    u32 GetAddressShift();
    bool GetShiftRegisterStrobe();
    bool GetShiftRegisterBit();
    u32 GetBankSize(int bank);
    u32 GetBankMask(int bank);
    u32 GetAddressShiftBits(int bank);
    u32 GetPageOffsetMask(int bank);
    u8* GetBankData(int bank);
    u8* GetBankDataA(int bank);
    u8* GetBank1Data();
    u32 GetBank1Size();
    bool IsBank1RAM();
    EEPROM* GetEEPROMInstance();
    u8* GetSaveMemoryPointer();
    s32 GetSaveMemorySize();
    bool IsSaveMemoryDirty();
    u8* GetNVRAM();
    bool IsNVRAMEnabled();
    void SaveRam(std::ostream& file);
    bool LoadRam(std::istream& file, s32 file_size);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void Serialize(StateSerializer& s);
    bool LoadFromZipFile(const u8* buffer, int size);
    void GatherInfoFromDB();
    bool GatherLynxHeader(const u8* buffer);
    bool GatherBS93Header(const u8* buffer);
    void DefaultLynxHeader();
    int DetectEpyxHeaderless();
    void SetupBanks();
    void GatherDataFromPath(const char* path);
    GLYNX_Rotation ReadHeaderRotation(u8 rotation);
    GLYNX_EEPROM ReadHeaderEEPROM(u8 eeprom);
    bool IsValidFile(const char* path);
    void DecryptDoubleValue(u8* result, int length);
    int DecryptMinusEquals(u8* result, const u8* value, int length);
    void DecryptPlusEquals(u8* result, const u8* value, int length);
    void DecryptMontgomery(u8* L, const u8* M, const u8* N, const u8* modulus, int length);
    int DecryptBlock(int accumulator, u8* result, const u8* encrypted, int length);
    int DecryptFrame(u8* result, const u8* encrypted, int length);
    void ClearSaveMemoryDirty();

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
    char m_header_name[32];
    char m_header_manufacturer[16];
    u8* m_bank_data[2];
    u8* m_bank_data_a[2];
    u32 m_bank_size[2];
    u32 m_bank_mask[2];
    u32 m_bank_page_size[2];
    u32 m_address_shift;
    u32 m_address_shift_bits[2];
    u32 m_page_offset;
    u32 m_page_offset_mask[2];
    bool m_shift_register_strobe;
    bool m_shift_register_bit;
    bool m_bank1_is_ram;
    bool m_nvram_enabled;
    u8* m_nvram;
    GLYNX_Rotation m_rotation;
    GLYNX_Rotation m_forced_rotation;
    GLYNX_Console_Type m_console_type;
    GLYNX_Console_Type m_forced_console_type;
    GLYNX_EEPROM m_eeprom;
    EEPROM* m_eeprom_instance;
    GLYNX_Media_Type m_type;
    bool m_audin;
    bool m_audin_value;
    u16 m_homebrew_boot_address;
    u16 m_homebrew_size;
    int m_epyx_headerless;
    u32 m_crc;
    u8* m_decrypt_buffer_a;
    u8* m_decrypt_buffer_b;
    u8* m_decrypt_buffer_tmp;
    u8* m_decrypt_buffer_sub;
};

#include "media_inline.h"

#endif /* MEDIA_H */