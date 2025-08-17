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

class Suzy
{
public:
    Suzy(Cartridge* cartridge, M6502* m6502);
    ~Suzy();
    void Init(Memory* memory);
    void Reset();
    void Clock(u32 cycles);
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    Cartridge* m_cartridge;
    Memory* m_memory;
    M6502* m_m6502;
    u16_union m_TMPADR;
    u16_union m_TILTACUM;
    u16_union m_HOFF;
    u16_union m_VOFF;
    u16_union m_VIDBAS;
    u16_union m_COLLBAS;
    u16_union m_VIDADR;
    u16_union m_COLLADR;
    u16_union m_SCBNEXT;
    u16_union m_SPRDLINE;
    u16_union m_HPOSSTRT;
    u16_union m_VPOSSTRT;
    u16_union m_SPRHSIZ;
    u16_union m_SPRVSIZ;
    u16_union m_STRETCH;
    u16_union m_TILT;
    u16_union m_SPRDOFF;
    u16_union m_SPRVPOS;
    u16_union m_COLLOFF;
    u16_union m_VSIZACUM;
    u16_union m_HSIZOFF;
    u16_union m_VSIZOFF;
    u16_union m_SCBADR;
    u16_union m_PROCADR;
    u8 m_MATHD;
    u8 m_MATHC;
    u8 m_MATHB;
    u8 m_MATHA;
    u8 m_MATHP;
    u8 m_MATHN;
    u8 m_MATHH;
    u8 m_MATHG;
    u8 m_MATHF;
    u8 m_MATHE;
    u8 m_MATHM;
    u8 m_MATHL;
    u8 m_MATHK;
    u8 m_MATHJ;
    u8 m_SPRCTL0;
    u8 m_SPRCTL1;
    u8 m_SPRCOLL;
    u8 m_SPRINIT;
    u8 m_SUZYBUSEN;
    u8 m_SPRGO;
    u8 m_SPRSYS;
    u8 m_JOYSTICK;
    u8 m_SWITCHES;
};

#include "suzy_inline.h"

#endif /* SUZY_H */
