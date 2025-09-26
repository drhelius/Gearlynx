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

class Media;
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
        bool frame_ready;
        u32 render_line;
        u16 dispadr_latch;
        bool rest;
    };

public:
    Mikey(Media* media, M6502* m6502);
    ~Mikey();
    void Init(Suzy* suzy, Memory* memory, GLYNX_Pixel_Format pixel_format);
    void Reset();
    bool Clock(u32 cycles);
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    Mikey_State* GetState();
    void SetBuffer(u8* frame_buffer);
    u8* GetBuffer();
    u32* GetRGBA8888Palette();
    u16* GetRGB565Palette();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void InitPalettes();
    void ResetTimers();
    u8 ReadColor(u16 address);
    void WriteColor(u16 address, u8 value);
    u8 ReadTimer(u16 address);
    void WriteTimer(u16 address, u8 value);
    u8 ReadAudio(u16 address);
    void WriteAudio(u16 address, u8 value);
    u8 ReadAudioExtra(u16 address);
    void WriteAudioExtra(u16 address, u8 value);
    void UpdateTimers(u32 cycles);
    void UpdateIRQs();
    void HorizontalBlank();
    void VerticalBlank();
    void LineDMA(int line);
    template <int bytes_per_pixel>
    void LineDMATemplate(int line);
    template <int bytes_per_pixel>
    void LineDMABlankTemplate(int line);

private:
    Media* m_media;
    Suzy* m_suzy;
    Memory* m_memory;
    M6502* m_m6502;
    Mikey_State m_state;
    u8* m_ram;
    u16 m_host_palette[16] = {};
    u8* m_frame_buffer;
    GLYNX_Pixel_Format m_pixel_format;
    u32 m_rgba8888_palette[4096] = {};
    u16 m_rgb565_palette[4096] = {};

    u32 m_debug_cycles;
};

static const u32 k_mikey_timerX_period_cycles[8] = { 16, 32, 64, 128, 256, 512, 1024, 0 };
static const u32 k_mikey_timer4_period_cycles[8] = { 128, 256, 512, 1024, 2048, 4096, 8192, 0 };
static const int k_mikey_timer_forward_links[8] = { 2, 3, 4, 5, -1, 7, -1, -1 };
static const int k_mikey_timer_backward_links[8] = { -1, -1, 0, 1, 2, 3, -1, 5 };

#include "mikey_inline.h"

#endif /* MIKEY_H */
