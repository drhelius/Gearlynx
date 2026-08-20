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
#include "no_power.h"

template<typename Pixel, GLYNX_Rotation rotation>
static void ConvertAndRotateFrameBuffer(const u16* src, Pixel* dst,
    const Pixel* palette)
{
    const int width = GLYNX_SCREEN_WIDTH;
    const int height = GLYNX_SCREEN_HEIGHT;
    const int pixel_count = width * height;

    if (rotation == GLYNX_ROTATION_180)
    {
        for (int i = 0; i < pixel_count; ++i)
            dst[pixel_count - 1 - i] = palette[src[i] & 0x0FFF];
        return;
    }

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const int src_index = y * width + x;
            const int dst_index = (rotation == GLYNX_ROTATION_LEFT)
                                  ? (width - 1 - x) * height + y
                                  : x * height + (height - 1 - y);
            dst[dst_index] = palette[src[src_index] & 0x0FFF];
        }
    }
}

template<typename Pixel>
static void ConvertRotatedFrameBuffer(const u16* src, Pixel* dst,
    const Pixel* palette, GLYNX_Rotation rotation)
{
    switch (rotation)
    {
        case GLYNX_ROTATION_LEFT:
            ConvertAndRotateFrameBuffer<Pixel, GLYNX_ROTATION_LEFT>(src, dst, palette);
            break;
        case GLYNX_ROTATION_180:
            ConvertAndRotateFrameBuffer<Pixel, GLYNX_ROTATION_180>(src, dst, palette);
            break;
        case GLYNX_ROTATION_RIGHT:
        default:
            ConvertAndRotateFrameBuffer<Pixel, GLYNX_ROTATION_RIGHT>(src, dst, palette);
            break;
    }
}

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

        if (rotation == GLYNX_ROTATION_DISABLED)
        {
            for (int i = 0; i < pixel_count; ++i)
            {
                u16 color_12bit = src[i] & 0x0FFF;
                dst[i] = m_rgb565_palette[color_12bit];
            }
            return;
        }

        ConvertRotatedFrameBuffer(src, dst, m_rgb565_palette,
            rotation);
    }
    else
    {
        u32* dst = (u32*)m_frame_buffer;

        if (rotation == GLYNX_ROTATION_DISABLED)
        {
            for (int i = 0; i < pixel_count; ++i)
            {
                u16 color_12bit = src[i] & 0x0FFF;
                dst[i] = m_rgba8888_palette[color_12bit];
            }
            return;
        }

        ConvertRotatedFrameBuffer(src, dst, m_rgba8888_palette,
            rotation);
    }
}

void LcdScreen::RenderNoBiosScreen(u8* frame_buffer)
{
    int byte_count = GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT * (m_pixel_format == GLYNX_PIXEL_RGB565 ? 2 : 4);
    u8* no_bios_image = (m_pixel_format == GLYNX_PIXEL_RGB565) ? (u8*)k_no_bios_rgb565 : (u8*)k_no_bios_rgba8888;
    memcpy(frame_buffer, no_bios_image, byte_count);
}

void LcdScreen::RenderNoPowerScreen(u8* frame_buffer)
{
    int byte_count = GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT * (m_pixel_format == GLYNX_PIXEL_RGB565 ? 2 : 4);
    u8* no_power_image = (m_pixel_format == GLYNX_PIXEL_RGB565) ? (u8*)k_no_power_rgb565 : (u8*)k_no_power_rgba8888;
    memcpy(frame_buffer, no_power_image, byte_count);
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

    if (m_state.line_dst_offset >= GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT)
        m_state.line_dst_offset = 0;
    if (m_state.pixel_count > GLYNX_SCREEN_WIDTH)
        m_state.pixel_count = GLYNX_SCREEN_WIDTH;
    if (m_state.pixel_buffer_read_pos > 31)
        m_state.pixel_buffer_read_pos = 0;
    if (m_state.dma_buffer_half != 0 && m_state.dma_buffer_half != 16)
        m_state.dma_buffer_half = 0;
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
