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

void Suzy::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Suzy::LoadState(std::istream& stream)
{
    UNUSED(stream);
}
