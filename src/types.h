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

struct GLYNX_Runtime_Info
{
    int screen_width;
    int screen_height;
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
    GLYNX_PIXEL_RGB555,
    GLYNX_PIXEL_RGBA8888,
    GLYNX_PIXEL_BGR565,
    GLYNX_PIXEL_BGR555,
    GLYNX_PIXEL_BGRA8888
};

enum GLYNX_Keys
{
    GLYNX_KEY_A,
    GLYNX_KEY_B,
    GLYNX_KEY_START,
    GLYNX_KEY_OPTION1,
    GLYNX_KEY_OPTION2,
    GLYNX_KEY_UP,
    GLYNX_KEY_RIGHT,
    GLYNX_KEY_DOWN,
    GLYNX_KEY_LEFT
};

struct GLYNX_Cartridge_Header
{
    u8  magic[4];
    u16 size_bank0;
    u16 size_bank1;
    u16 version;
    u8  name[32];
    u8  manufacturer[16];
    u8  rotation;
    u8  audin;
    u8  eeprom;
    u8  spare[3];
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
    char name[64];
    char bytes[25];
    char segment[5];
    u8 opcodes[7];
    int size;
    bool jump;
    u16 jump_address;
    u8 jump_bank;
    bool subroutine;
    int irq;
};

#endif /* TYPES_H */