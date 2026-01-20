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

#ifndef LCD_SCREEN_H
#define LCD_SCREEN_H

#include <iostream>
#include <fstream>
#include "common.h"
#include "state_serializer.h"

class Mikey;
class Memory;
class Bus;

class LcdScreen
{
public:
    LcdScreen(Mikey* mikey, Memory* memory, Bus* bus);
    ~LcdScreen();
    void Init(GLYNX_Pixel_Format pixel_format);
    void Reset();
    void Update(u32 cycles);
    void ResetLine(u8 line);
    void ClearLine(u8 line);
    void FirstDMA();
    void ConfigureLineTiming();
    void UpdatePalette(int index, u16 color);
    void EndFrame(GLYNX_Rotation rotation);
    void SetBuffer(u8* frame_buffer);
    u8* GetBuffer();
    u32* GetRGBA8888Palette();
    u16* GetRGB565Palette();
    GLYNX_Pixel_Format GetPixelFormat();
    void RenderNoBiosScreen(u8* frame_buffer);
    void SetVBlank(bool vblank);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void InitPalettes();
    void DoDMA();
    void DrawPixel();
    void RotateFrameBuffer(GLYNX_Rotation rotation);
    void Serialize(StateSerializer& s);

private:
    Mikey* m_mikey;
    Memory* m_memory;
    Bus* m_bus;
    u8* m_ram;
    u8* m_frame_buffer;
    u16 m_screen_buffer[GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT] = {};
    u8 m_rotated_frame_buffer[GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT * 4] = {};
    u16 m_current_palette[16] = {};
    u8 m_dma_buffer[32] = {};
    u32 m_current_cycle;
    u32 m_current_line;
    u32 m_rendering_offset;
    u32 m_line_cycles;
    u32 m_dma_next_at;
    u16 m_dma_current_src_addr;
    u32 m_dma_burst_count;
    u32 m_dma_buffer_half;
    u32 m_pixel_next_at;
    u32 m_pixel_count;
    u32 m_pixel_buffer_read_pos;
    u32 m_line_dst_offset;
    bool m_in_vblank;
    GLYNX_Pixel_Format m_pixel_format;
    u32 m_rgba8888_palette[4096] = {};
    u16 m_rgb565_palette[4096] = {};
};

static const u32 k_pixel_spacing_cycles = 12;
static const u32 k_dma_burst_cycles = 28;
static const u32 k_dma_refresh_only_cycles = 10;
static const u32 k_dma_spacing_cycles = 192;
static const u32 k_dma_bursts_per_line = 10;

#include "lcd_screen_inline.h"

#endif /* LCD_SCREEN_H */
