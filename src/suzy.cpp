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
#include "suzy.h"
#include "cartridge.h"
#include "memory.h"
#include "m6502.h"
#include "mikey.h"

Suzy::Suzy(Cartridge* cartridge, M6502* m6502)
{
    m_cartridge = cartridge;
    m_m6502 = m6502;
    InitPointer(m_mikey);
    InitPointer(m_memory);
    InitPointer(m_frame_buffer);
    m_pixel_format = GLYNX_PIXEL_RGBA8888;
    Reset();
}

Suzy::~Suzy()
{
}

void Suzy::Init(Mikey* mikey, Memory* memory, GLYNX_Pixel_Format pixel_format)
{
    m_mikey = mikey;
    m_memory = memory;
    m_pixel_format = pixel_format;
    InitPalettes();
    Reset();
}

void Suzy::Reset()
{
    m_frame_ready = false;
    m_render_line = 0;
    memset(&m_state, 0, sizeof(Suzy_State));
}

void Suzy::InitPalettes()
{
    for (int i = 0; i < 4096; ++i)
    {
        u8 green = ((i >> 8) & 0x0F) * 255 / 15;
        u8 blue = ((i >> 4) & 0x0F) * 255 / 15;
        u8 red = (i & 0x0F) * 255 / 15;
        m_rgba888_palette[i][0] = red;
        m_rgba888_palette[i][1] = green;
        m_rgba888_palette[i][2] = blue;
        m_rgba888_palette[i][3] = 255;

        green  = ((i >> 8) & 0x0F) * 63 / 15;
        blue   = ((i >> 4) & 0x0F) * 31 / 15;
        red    = (i & 0x0F) * 31 / 15;
        u16 rgb565 = (red << 11) | (green << 5) | blue;
        m_rgb565_palette[i][0] = rgb565 & 0xFF;
        m_rgb565_palette[i][1] = (rgb565 >> 8) & 0xFF;
    }
}

void Suzy::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Suzy::LoadState(std::istream& stream)
{
    UNUSED(stream);
}