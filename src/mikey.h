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
class Suzy;

class Mikey
{
public:
    struct Mikey_State
    {
        GLYNX_Mikey_Timer timers[8];
        GLYNX_Mikey_Color colors[16];
        GLYNX_Mikey_Audio audio[4];
        u8 ATTEN_A;
        u8 ATTEN_B;
        u8 ATTEN_C;
        u8 ATTEN_D;
        u8 MPAN;
        u8 MSTEREO;
        u8 SYSCTL1;
        u8 IODIR;
        u8 IODAT;
        u8 SERCTL;
        u8 SERDAT;
        u8 SDONEACK;
        u8 CPUSLEEP;
        u8 DISPCTL;
        u8 PBKUP;
        u16_union DISPADR;
        u8 irq_pending;
        u8 irq_mask;
    };

public:
    Mikey(Cartridge* cartridge, M6502* m6502);
    ~Mikey();
    void Init(Suzy* suzy, Memory* memory);
    void Reset();
    void Clock(u32 cycles);
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    Mikey_State* GetState();
    u16* GetPalette();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void InitTimers();
    u8 ReadTimer(u8 timer_index, u8 reg);
    void WriteTimer(u8 timer_index, u8 reg, u8 value);
    u8 ReadAudio(u8 channel, u8 reg);
    void WriteAudio(u8 channel, u8 reg, u8 value);
    u8 ReadAudioExtra(u16 address);
    void WriteAudioExtra(u16 address, u8 value);
    void UpdateTimers(u32 cycles);
    void DecrementTimerCounter(u8 timer_index);
    void UpdateIRQs();

private:
    Cartridge* m_cartridge;
    Suzy* m_suzy;
    Memory* m_memory;
    M6502* m_m6502;
    Mikey_State m_state;
    u16 m_palette[16] = {};
};

static const u32 k_mikey_timer_period_cycles[8] = { 4, 8, 16, 32, 64, 128, 512, 0 };

#include "mikey_inline.h"

#endif /* MIKEY_H */
