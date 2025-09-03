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

#ifndef SUZY_H
#define SUZY_H

#include <iostream>
#include <fstream>
#include "common.h"
#include "suzy_defines.h"

class Cartridge;
class Memory;
class M6502;
class Mikey;

class Suzy
{
public:
    struct Suzy_State
    {
        u16_union TMPADR;
        u16_union TILTACUM;
        u16_union HOFF;
        u16_union VOFF;
        u16_union VIDBAS;
        u16_union COLLBAS;
        u16_union VIDADR;
        u16_union COLLADR;
        u16_union SCBNEXT;
        u16_union SPRDLINE;
        u16_union HPOSSTRT;
        u16_union VPOSSTRT;
        u16_union SPRHSIZ;
        u16_union SPRVSIZ;
        u16_union STRETCH;
        u16_union TILT;
        u16_union SPRDOFF;
        u16_union SPRVPOS;
        u16_union COLLOFF;
        u16_union VSIZACUM;
        u16_union HSIZOFF;
        u16_union VSIZOFF;
        u16_union SCBADR;
        u16_union PROCADR;
        u8 MATHD, MATHC, MATHB, MATHA, MATHP, MATHN, MATHH, MATHG, MATHF, MATHE, MATHM, MATHL, MATHK, MATHJ;
        u8 SPRCTL0, SPRCTL1, SPRCOLL, SPRINIT, SUZYBUSEN, SPRGO, SPRSYS;
        u8 JOYSTICK, SWITCHES;
        u8 pen_map[16];
    };

public:
    Suzy(Cartridge* cartridge, M6502* m6502);
    ~Suzy();
    void Init(Mikey* mikey, Memory* memory);
    void Reset();
    void Clock(u32 cycles);
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    Suzy_State* GetState();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void SpritesGo();
    void DrawSprite();
    void DrawSpriteLineLiteral(u16 data_begin, u16 data_end, s32 x, s32 y, s32 dx, int bpp, int type, u16 hsiz, u32 haccum_init);
    void DrawSpriteLinePacked(u16 data_begin, u16 data_end, s32 x, s32 y, s32 dx, int bpp, int type, u16 hsiz, u32 haccum_init);
    void DrawPixel(s32 x, s32 y, u8 pen, int type);
    bool IsPixelTransparent(u8 pen, int type);
    u8 RamRead(u16 address);
    u16 RamReadWord(u16 address);
    void RamWrite(u16 address, u8 value);
    void ShiftRegisterReset(u16 address);
    u32 ShiftRegisterGetBits(int n, u16 stop_addr);
    void ComputeQuadLUT();

private:
    struct QuadPos
    {
        bool left;
        bool up;
    };

private:
    Cartridge* m_cartridge;
    Mikey* m_mikey;
    Memory* m_memory;
    M6502* m_m6502;
    Suzy_State m_state;
    u8* m_ram;
    QuadPos m_quad_lut[4][4][4] = {};

    u16 m_shift_register_address;
    u8 m_shift_register_current;
    int m_shift_register_bit;
};

#include "suzy_inline.h"

#endif /* SUZY_H */
