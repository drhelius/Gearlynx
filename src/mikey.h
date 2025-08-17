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

#ifndef MIKEY_H
#define MIKEY_H

#include <iostream>
#include <fstream>
#include "common.h"
#include "mikey_defines.h"

class Cartridge;
class Memory;
class M6502;

class Mikey
{
public:
    Mikey(Cartridge* cartridge, M6502* m6502);
    ~Mikey();
    void Init(Memory* memory);
    void Reset();
    void Clock(u32 cycles);
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    u8 ReadTimer(u8 timer_index, u8 reg);
    void WriteTimer(u8 timer_index, u8 reg, u8 value);
    u8 ReadAudio(u8 channel, u8 reg);
    void WriteAudio(u8 channel, u8 reg, u8 value);
    u8 ReadAudioExtra(u16 address);
    void WriteAudioExtra(u16 address, u8 value);

private:
    Cartridge* m_cartridge;
    Memory* m_memory;
    M6502* m_m6502;
    GLYNX_Mikey_Timer m_timers[8];
    GLYNX_Mikey_Color m_colors[16];
    GLYNX_Mikey_Audio m_audio[4];
    u8 m_ATTEN_A;
    u8 m_ATTEN_B;
    u8 m_ATTEN_C;
    u8 m_ATTEN_D;
    u8 m_MPAN;
    u8 m_MSTEREO;
    u8 m_INTRST;
    u8 m_INTSET;
    u8 m_SYSCTL1;
    u8 m_IODIR;
    u8 m_IODAT;
    u8 m_SERCTL;
    u8 m_SERDAT;
    u8 m_SDONEACK;
    u8 m_CPUSLEEP;
    u8 m_DISPCTL;
    u8 m_PBKUP;
    u16_union m_DISPADR;
};

#include "mikey_inline.h"

#endif /* MIKEY_H */
