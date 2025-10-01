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

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

union u16_union
{
    u16 value;
    struct
    {
#ifdef GLYNX_LITTLE_ENDIAN
        u8 low;
        u8 high;
#else
        u8 high;
        u8 low;
#endif
    };
};

struct GLYNX_Runtime_Info
{
    int screen_width;
    int screen_height;
    float frame_time;
};

enum GLYNX_Bios_State
{
    BIOS_LOAD_OK = 0,
    BIOS_LOAD_FILE_ERROR,
    BIOS_LOAD_INVALID_SIZE,
    BIOS_LOAD_INVALID_CRC
};

struct GLYNX_Color
{
    u8 red;
    u8 green;
    u8 blue;
};

enum GLYNX_Pixel_Format
{
    GLYNX_PIXEL_RGB565,
    GLYNX_PIXEL_RGBA8888,
};

enum GLYNX_Keys
{
    GLYNX_KEY_A         = 0x0001,
    GLYNX_KEY_B         = 0x0002,
    GLYNX_KEY_OPTION2   = 0x0004,
    GLYNX_KEY_OPTION1   = 0x0008,
    GLYNX_KEY_LEFT      = 0x0010,
    GLYNX_KEY_RIGHT     = 0x0020,
    GLYNX_KEY_UP        = 0x0040,
    GLYNX_KEY_DOWN      = 0x0080,
    GLYNX_KEY_PAUSE     = 0x0100,
};

struct GLYNX_Cartridge_Header
{
    u8 magic[4];
    u16 bank0_page_size;
    u16 bank1_page_size;
    u16 version;
    char name[32];
    char manufacturer[16];
    u8 rotation;
    u8 audin;
    u8 eeprom;
    u8 reserved[3];
};

struct GLYNX_BS93_Header
{
    u8 magic[2];
    u16 boot_address;
    u16 size;
    u8 bs93[4];
};

struct GLYNX_SaveState_Header
{
    u32 magic;
    u32 version;
    u32 size;
    s64 timestamp;
    char rom_name[128];
    u32 rom_crc;
    u32 screenshot_size;
    u16 screenshot_width;
    u16 screenshot_height;
    char emu_build[32];
};

struct GLYNX_SaveState_Header_Libretro
{
    u32 magic;
    u32 version;
};

struct GLYNX_SaveState_Screenshot
{
    u32 width;
    u32 height;
    u32 size;
    u8* data;
};

struct GLYNX_Disassembler_Record
{
    u32 address;
    bool rom;
    char name[64];
    char bytes[10];
    char segment[5];
    u8 opcodes[3];
    int size;
    bool jump;
    u16 jump_address;
    u8 jump_bank;
    bool subroutine;
    int irq;
};

struct GLYNX_Mikey_Timer
{
    u8 backup;
    u8 control_a;
    u8 control_b;
    u8 counter;

    u32 internal_cycles;
    u32 internal_period_cycles;
    u32 internal_pending_ticks;
};

struct GLYNX_Mikey_Audio
{
    u8 volume;
    u8 feedback;
    s8 output;
    u8 lfsr_low;
    u8 backup;
    u8 control;
    u8 counter;
    u8 other;

    u32 internal_cycles;
    u32 internal_period_cycles;
    u32 internal_pending_ticks;
    u16 internal_lfsr;
    u16 internal_taps_mask;
    bool mix;
};

struct GLYNX_Mikey_Color
{
    u8 green;
    u8 bluered;
};

#endif /* TYPES_H */
