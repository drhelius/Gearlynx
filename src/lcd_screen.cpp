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

#include "lcd_screen.h"
#include "mikey.h"
#include "memory.h"
#include "bus.h"
#include "no_bios.h"

LcdScreen::LcdScreen(Mikey* mikey, Memory* memory, Bus* bus)
{
    m_mikey = mikey;
    m_memory = memory;
    m_bus = bus;
    m_ram = m_memory->GetRAM();
    InitPointer(m_frame_buffer);
    m_pixel_format = GLYNX_PIXEL_RGBA8888;
    Reset();
}

LcdScreen::~LcdScreen()
{
}

void LcdScreen::Init(GLYNX_Pixel_Format pixel_format)
{
    Reset();
    m_pixel_format = pixel_format;
    InitPalettes();
}

void LcdScreen::Reset()
{
    memset(m_screen_buffer, 0, sizeof(m_screen_buffer));
    memset(&m_state, 0, sizeof(m_state));
}

void LcdScreen::InitPalettes()
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

        m_rgb565_palette[i] = rgb565;
    }
}

void LcdScreen::EndFrame(GLYNX_Rotation rotation)
{
    u16* src = m_screen_buffer;
    const int pixel_count = GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT;

    if (m_pixel_format == GLYNX_PIXEL_RGB565)
    {
        u16* dst = (u16*)m_frame_buffer;
        for (int i = 0; i < pixel_count; ++i)
        {
            u16 color_12bit = src[i] & 0x0FFF;
            dst[i] = m_rgb565_palette[color_12bit];
        }
    }
    else
    {
        u32* dst = (u32*)m_frame_buffer;
        for (int i = 0; i < pixel_count; ++i)
        {
            u16 color_12bit = src[i] & 0x0FFF;
            dst[i] = m_rgba8888_palette[color_12bit];
        }
    }

    RotateFrameBuffer(rotation);
}

void LcdScreen::RenderNoBiosScreen(u8* frame_buffer)
{
    int byte_count = GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT * (m_pixel_format == GLYNX_PIXEL_RGB565 ? 2 : 4);
    u8* no_bios_image = (m_pixel_format == GLYNX_PIXEL_RGB565) ? (u8*)k_no_bios_rgb565 : (u8*)k_no_bios_rgba8888;
    memcpy(frame_buffer, no_bios_image, byte_count);
}

void LcdScreen::RotateFrameBuffer(GLYNX_Rotation rotation)
{
    if (rotation == GLYNX_ROTATION_DISABLED)
        return;

    const int width = GLYNX_SCREEN_WIDTH;
    const int height = GLYNX_SCREEN_HEIGHT;
    const int pixel_count = width * height;

    if (m_pixel_format == GLYNX_PIXEL_RGB565)
    {
        const u16* src = reinterpret_cast<const u16*>(m_frame_buffer);
        u16* dst = reinterpret_cast<u16*>(m_rotated_frame_buffer);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const int src_index = y * width + x;
                const int dst_index = (rotation == GLYNX_ROTATION_LEFT)
                                      ? (width - 1 - x) * height + y
                                      : x * height + (height - 1 - y);
                dst[dst_index] = src[src_index];
            }
        }

        memcpy(m_frame_buffer, m_rotated_frame_buffer, pixel_count * sizeof(u16));
        return;
    }

    const u32* src = reinterpret_cast<const u32*>(m_frame_buffer);
    u32* dst = reinterpret_cast<u32*>(m_rotated_frame_buffer);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const int src_index = y * width + x;
            const int dst_index = (rotation == GLYNX_ROTATION_LEFT)
                                  ? (width - 1 - x) * height + y
                                  : x * height + (height - 1 - y);
            dst[dst_index] = src[src_index];
        }
    }

    memcpy(m_frame_buffer, m_rotated_frame_buffer, pixel_count * sizeof(u32));
}

void LcdScreen::SaveState(std::ostream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void LcdScreen::LoadState(std::istream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void LcdScreen::Serialize(StateSerializer& s)
{
    G_SERIALIZE_ARRAY(s, m_screen_buffer, GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT);
    G_SERIALIZE_ARRAY(s, m_state.current_palette, 16);
    G_SERIALIZE_ARRAY(s, m_state.dma_buffer, 32);

    G_SERIALIZE(s, m_state.current_cycle);
    G_SERIALIZE(s, m_state.current_line);
    G_SERIALIZE(s, m_state.rendering_offset);
    G_SERIALIZE(s, m_state.line_cycles);

    G_SERIALIZE(s, m_state.dma_next_at);
    G_SERIALIZE(s, m_state.dma_current_src_addr);
    G_SERIALIZE(s, m_state.dma_burst_count);
    G_SERIALIZE(s, m_state.dma_buffer_half);

    G_SERIALIZE(s, m_state.pixel_next_at);
    G_SERIALIZE(s, m_state.pixel_count);
    G_SERIALIZE(s, m_state.pixel_buffer_read_pos);
    G_SERIALIZE(s, m_state.line_dst_offset);
    G_SERIALIZE(s, m_state.in_vblank);
}
