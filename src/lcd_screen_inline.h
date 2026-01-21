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

#ifndef LCD_SCREEN_INLINE_H
#define LCD_SCREEN_INLINE_H

#include "lcd_screen.h"
#include "mikey.h"
#include "memory.h"
#include "bus.h"

INLINE void LcdScreen::Update(u32 cycles)
{
    if (m_in_vblank)
        return;

    m_current_cycle += cycles;

    if (m_dma_burst_count >= k_dma_bursts_per_line && m_pixel_count >= GLYNX_SCREEN_WIDTH)
        return;

    while (m_pixel_count < GLYNX_SCREEN_WIDTH && m_pixel_next_at <= m_current_cycle)
    {
        DrawPixel();
        m_pixel_next_at += k_pixel_spacing_cycles;
    }

    while (m_dma_burst_count < k_dma_bursts_per_line && m_dma_next_at <= m_current_cycle)
    {
        DoDMA();
        m_dma_burst_count++;
        m_dma_next_at += k_dma_spacing_cycles;
    }
}

INLINE void LcdScreen::ResetLine(u8 line)
{
    m_current_line = line;
    m_current_cycle = 0;
    m_dma_next_at = m_rendering_offset + 8;
    m_dma_burst_count = 0;
    m_dma_buffer_half = 16;
    m_pixel_next_at = m_rendering_offset;
    m_pixel_count = 0;
    m_pixel_buffer_read_pos = 0;
    m_line_dst_offset = m_current_line * GLYNX_SCREEN_WIDTH;
}

INLINE void LcdScreen::ClearLine(u8 line)
{
    u32 offset = line * GLYNX_SCREEN_WIDTH;
    memset(&m_screen_buffer[offset], 0, GLYNX_SCREEN_WIDTH * sizeof(u16));
}

INLINE void LcdScreen::FirstDMA()
{
    m_dma_current_src_addr = m_mikey->GetState()->dispadr_latch;
    m_dma_buffer_half = 0;
    DoDMA();
}

INLINE void LcdScreen::ConfigureLineTiming()
{
    Mikey::Mikey_State* state = m_mikey->GetState();
    u8 t0_backup = state->timers[0].backup;
    u32 t0_period = state->timers[0].internal_period_cycles;

    m_line_cycles = (t0_backup + 1) * t0_period;
    m_rendering_offset = MAX(0, (int)m_line_cycles - 1920);
}

INLINE void LcdScreen::UpdatePalette(int index, u16 color)
{
    assert(index < 16 && index >= 0);
    m_current_palette[index] = color;
}

INLINE void LcdScreen::SetBuffer(u8* frame_buffer)
{
    m_frame_buffer = frame_buffer;
}

INLINE u8* LcdScreen::GetBuffer()
{
    return m_frame_buffer;
}

INLINE u32* LcdScreen::GetRGBA8888Palette()
{
    return m_rgba8888_palette;
}

INLINE u16* LcdScreen::GetRGB565Palette()
{
    return m_rgb565_palette;
}

INLINE GLYNX_Pixel_Format LcdScreen::GetPixelFormat()
{
    return m_pixel_format;
}

INLINE void LcdScreen::SetVBlank(bool vblank)
{
    m_in_vblank = vblank;
}

INLINE void LcdScreen::DoDMA()
{
    u8* dst = &m_dma_buffer[m_dma_buffer_half];
    bool enabled = IS_SET_BIT(m_mikey->GetState()->DISPCTL, 0);

    if (likely(enabled))
    {
        u8 src_byte;
        for (int i = 0; i < 8; i++)
        {
            src_byte = m_ram[(m_dma_current_src_addr + i) & 0xFFFF];
            dst[i * 2] = src_byte >> 4;
            dst[i * 2 + 1] = src_byte & 0x0F;
        }

        m_bus->InjectCycles(k_dma_burst_cycles);
    }
    else
    {
        memset(dst, 0, 16);
        m_bus->InjectCycles(k_dma_refresh_only_cycles);
    }

    m_dma_current_src_addr += 8;
    m_dma_buffer_half ^= 16;
}

INLINE void LcdScreen::DrawPixel()
{
    u8 pen = m_dma_buffer[m_pixel_buffer_read_pos];
    m_pixel_buffer_read_pos = (m_pixel_buffer_read_pos + 1) & 0x1F;

    u16 color = m_current_palette[pen];
    m_screen_buffer[m_line_dst_offset + m_pixel_count] = color;
    m_pixel_count++;
}

#endif /* LCD_SCREEN_INLINE_H */
