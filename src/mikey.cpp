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

#include <istream>
#include <ostream>
#include "mikey.h"
#include "memory.h"
#include "suzy.h"

Mikey::Mikey(Media* media, M6502* m6502)
{
    m_media = media;
    m_m6502 = m6502;
    InitPointer(m_suzy);
    InitPointer(m_memory);
    InitPointer(m_frame_buffer);
    InitPointer(m_ram);
    m_pixel_format = GLYNX_PIXEL_RGBA8888;
    Reset();
}

Mikey::~Mikey()
{
}

void Mikey::Init(Suzy* suzy, Memory* memory, GLYNX_Pixel_Format pixel_format)
{
    m_suzy = suzy;
    m_memory = memory;
    m_pixel_format = pixel_format;
    m_ram = m_memory->GetRAM();
    InitPalettes();
    Reset();
}

void Mikey::Reset()
{
    memset(&m_state, 0, sizeof(Mikey_State));
    memset(m_host_palette, 0, sizeof(m_host_palette));

    ResetPalette();
    ResetTimers();
    ResetAudio();

    m_debug_cycles = 0;
}

void Mikey::InitPalettes()
{
    for (int i = 0; i < 4096; ++i)
    {
        u8 green = ((i >> 8) & 0x0F) * 255 / 15;
        u8 blue = ((i >> 4) & 0x0F) * 255 / 15;
        u8 red = (i & 0x0F) * 255 / 15;

        #ifdef GLYNX_LITTLE_ENDIAN
        m_rgba8888_palette[i] = (u32)red | ((u32)green << 8) | ((u32)blue << 16) | ((u32)255 << 24);
        #else
        m_rgba8888_palette[i] = ((u32)255) | ((u32)blue << 8) | ((u32)green << 16) | ((u32)red << 24);
        #endif

        green  = ((i >> 8) & 0x0F) * 63 / 15;
        blue   = ((i >> 4) & 0x0F) * 31 / 15;
        red    = (i & 0x0F) * 31 / 15;
        u16 rgb565 = (red << 11) | (green << 5) | blue;

        #ifdef GLYNX_LITTLE_ENDIAN
        m_rgb565_palette[i] = rgb565;
        #else
        m_rgb565_palette[i] = (rgb565 >> 8) | ((rgb565 & 0xFF) << 8);
        #endif
    }
}

void Mikey::ResetTimers()
{
    for (int i = 0; i < 8; i++)
    {
        m_state.timers[i].backup = 0;
        m_state.timers[i].counter = 0;
        m_state.timers[i].control_a = 0;
        m_state.timers[i].control_b = 0;

        m_state.timers[i].internal_cycles = 0;
        m_state.timers[i].internal_period_cycles = k_mikey_timer_period_cycles[0];
        m_state.timers[i].internal_pending_ticks = 0;
    }
}

void Mikey::ResetAudio()
{
    for (int i = 0; i < 4; i++)
    {
        m_state.audio[i].volume = 0;
        m_state.audio[i].feedback = 0;
        m_state.audio[i].output = 0;
        m_state.audio[i].lfsr_low = 0;
        m_state.audio[i].backup = 0;
        m_state.audio[i].control = 0;
        m_state.audio[i].counter = 0;
        m_state.audio[i].other = 0;

        m_state.audio[i].internal_cycles = 0;
        m_state.audio[i].internal_period_cycles = k_mikey_timer_period_cycles[0];
        m_state.audio[i].internal_pending_ticks = 0;
        m_state.audio[i].internal_lfsr = 0;
        m_state.audio[i].internal_taps_mask = 0;
        m_state.audio[i].internal_mix = false;
    }
}

void Mikey::ResetPalette()
{
    for (int address = 0xFDA0; address < 0xFDC0; address++)
        WriteColor(address, 0xFF);
}

void Mikey::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Mikey::LoadState(std::istream& stream)
{
    UNUSED(stream);
}