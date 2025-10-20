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

#include <cstring>
#include <istream>
#include <ostream>
#include "mikey.h"
#include "memory.h"
#include "no_bios.h"
#include "state_serializer.h"

Mikey::Mikey(Suzy* suzy, Media* media, M6502* m6502)
{
    m_suzy = suzy;
    m_media = media;
    m_m6502 = m6502;
    InitPointer(m_memory);
    InitPointer(m_frame_buffer);
    InitPointer(m_ram);
    m_pixel_format = GLYNX_PIXEL_RGBA8888;
    Reset();
}

Mikey::~Mikey()
{
}

void Mikey::Init(Memory* memory, GLYNX_Pixel_Format pixel_format)
{
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
    ResetUART();

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
        m_state.audio[i].internal_mix = true;
    }
}

void Mikey::ResetUART()
{
    m_state.uart.tx_int_en = false;
    m_state.uart.rx_int_en = false;
    m_state.uart.par_en = false;
    m_state.uart.tx_open = false;
    m_state.uart.tx_brk = false;
    m_state.uart.par_even = false;
    m_state.uart.tx_ready = true;
    m_state.uart.rx_ready = false;
    m_state.uart.tx_empty = true;
    m_state.uart.par_err = false;
    m_state.uart.ovr_err = false;
    m_state.uart.fram_err = false;
    m_state.uart.rx_break = false;
    m_state.uart.par_bit = false;
    m_state.uart.tx_active = false;
    m_state.uart.tx_hold_valid = false;
    m_state.uart.tx_parbit = false;
    m_state.uart.tx_hold_data = 0;
    m_state.uart.tx_data = 0;
    m_state.uart.rx_data = 0;
    m_state.uart.tx_bit_index = 0;
    m_state.uart.prescaler = 0;
    m_state.uart.tx_empty_bits = 0;
    m_state.uart.tx_ready_bits = 0;
    m_state.uart.tx_started_from_chain = false;
}

void Mikey::ResetPalette()
{
    for (int address = 0xFDA0; address < 0xFDC0; address++)
        WriteColor(address, 0xFF);
}

void Mikey::HorizontalBlank()
{
    u8 timer_2_counter = m_state.timers[2].counter;
    u8 timer_2_backup = m_state.timers[2].backup;
    int line = 101 - timer_2_counter;

    if (line >= 0 && line < 102)
    {
        //DebugMikey("===> Rendering line %d: DISPADR %04X. Timer 2 counter: %d. Cycles: %d", m_state.render_line, m_state.dispadr_latch, timer_2_counter, m_debug_cycles);
        LineDMA(line);
    }
    // else
    // {
    //     DebugMikey("===> Skiping VBLANK line %d: DISPADR %04X. Timer 2 counter: %d. Cycles: %d", m_state.render_line, m_state.dispadr_latch, timer_2_counter, m_debug_cycles);
    // }

    // Typically end of hcount 104
    if (timer_2_counter == timer_2_backup)
    {
        DebugMikey("===> Rest signal goes low");
        m_state.rest = false;
    }
    // Typically end of hcount 103, start of 3rd vblank line
    else if (timer_2_counter == (timer_2_backup - 1))
    {
        DebugMikey("===> Latching DISPADR %04X", m_state.DISPADR.value);
        m_state.dispadr_latch = m_state.DISPADR.value & 0xFFFC;
    }
    // Typically end of hcount 101
    else if (timer_2_counter == (timer_2_backup - 3))
    {
        DebugMikey("===> Rest signal goes high");
        m_state.rest = true;
    }

    // At the end of the last vblank line
    if (timer_2_counter == (timer_2_backup - 2))
    {
        m_state.render_line = 0;
    }
    else
        m_state.render_line++;
}

void Mikey::VerticalBlank()
{
    DebugMikey("===> VBLANK. DISPADR %04X. Cycles: %d", m_state.dispadr_latch, m_debug_cycles);
    m_state.frame_ready = true;
    //m_state.render_line = 0;
    m_debug_cycles = 0;

    GLYNX_Rotation rotation = m_media->GetRotation();
    if (rotation != NO_ROTATION)
        RotateFrameBuffer(rotation);
}

void Mikey::LineDMA(int line)
{
    if (IS_SET_BIT(m_state.DISPCTL, 0))
    {
        if (m_pixel_format == GLYNX_PIXEL_RGB565)
            LineDMATemplate<2>(line);
        else if (m_pixel_format == GLYNX_PIXEL_RGBA8888)
            LineDMATemplate<4>(line);
    }
    else
    {
        if (m_pixel_format == GLYNX_PIXEL_RGB565)
            LineDMABlankTemplate<2>(line);
        else if (m_pixel_format == GLYNX_PIXEL_RGBA8888)
            LineDMABlankTemplate<4>(line);
    }
}

template <int bytes_per_pixel>
void Mikey::LineDMATemplate(int line)
{
    assert(line >= 0 && line < GLYNX_SCREEN_HEIGHT);

    u8* ram = m_memory->GetRAM();
    u16 line_offset = (u16)(m_state.dispadr_latch + (line * (GLYNX_SCREEN_WIDTH / 2)));
    u8* src_line_ptr = ram + line_offset;
    u8* dst_line_ptr = m_frame_buffer + (line * GLYNX_SCREEN_WIDTH * bytes_per_pixel);
    u16* palette = m_host_palette;
    u8* src = src_line_ptr;

    // RGB565
    if (bytes_per_pixel == 2)
    {
        u16* dst16 = (u16*)(dst_line_ptr);

        for (int x = 0; x < GLYNX_SCREEN_WIDTH; x += 2)
        {
            u8 byte = *src++;
            u8 color0 = byte >> 4;
            u8 color1 = byte & 0x0F;
            u16 idx0 = palette[color0] & 0x0FFF;
            u16 idx1 = palette[color1] & 0x0FFF;

            dst16[0] = m_rgb565_palette[idx0];
            dst16[1] = m_rgb565_palette[idx1];
            dst16 += 2;
        }
    }
    // RGBA8888
    else
    {
        u32* dst32 = (u32*)(dst_line_ptr);

        for (int x = 0; x < GLYNX_SCREEN_WIDTH; x += 2)
        {
            u8 byte = *src++;
            u8 color0 = byte >> 4;
            u8 color1 = byte & 0x0F;
            u16 idx0 = palette[color0] & 0x0FFF;
            u16 idx1 = palette[color1] & 0x0FFF;

            dst32[0] = m_rgba8888_palette[idx0];
            dst32[1] = m_rgba8888_palette[idx1];
            dst32 += 2;
        }
    }
}

template <int bytes_per_pixel>
void Mikey::LineDMABlankTemplate(int line)
{
    assert(line >= 0 && line < GLYNX_SCREEN_HEIGHT);

    u8* dst_line_ptr = m_frame_buffer + (line * GLYNX_SCREEN_WIDTH * bytes_per_pixel);
    size_t line_size = GLYNX_SCREEN_WIDTH * bytes_per_pixel;
    memset(dst_line_ptr, 0, line_size);
}

void Mikey::RotateFrameBuffer(GLYNX_Rotation rotation)
{
    if (rotation == NO_ROTATION)
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
                const int dst_index = (rotation == ROTATE_LEFT)
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
            const int dst_index = (rotation == ROTATE_LEFT)
                                  ? (width - 1 - x) * height + y
                                  : x * height + (height - 1 - y);
            dst[dst_index] = src[src_index];
        }
    }

    memcpy(m_frame_buffer, m_rotated_frame_buffer, pixel_count * sizeof(u32));
}

void Mikey::RenderNoBiosScreen(u8* frame_buffer)
{
    int byte_count = GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT * (m_pixel_format == GLYNX_PIXEL_RGB565 ? 2 : 4);
    u8* no_bios_image = (m_pixel_format == GLYNX_PIXEL_RGB565) ? (u8*)k_no_bios_rgb565 : (u8*)k_no_bios_rgba8888;
    memcpy(frame_buffer, no_bios_image, byte_count);
}

void Mikey::SaveState(std::ostream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void Mikey::LoadState(std::istream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void Mikey::Serialize(StateSerializer& s)
{
    for (int i = 0; i < 8; i++)
    {
        G_SERIALIZE(s, m_state.timers[i].backup);
        G_SERIALIZE(s, m_state.timers[i].control_a);
        G_SERIALIZE(s, m_state.timers[i].control_b);
        G_SERIALIZE(s, m_state.timers[i].counter);

        G_SERIALIZE(s, m_state.timers[i].internal_cycles);
        G_SERIALIZE(s, m_state.timers[i].internal_period_cycles);
        G_SERIALIZE(s, m_state.timers[i].internal_pending_ticks);
    }

    for (int i = 0; i < 16; i++)
    {
        G_SERIALIZE(s, m_state.colors[i].green);
        G_SERIALIZE(s, m_state.colors[i].bluered);
    }

    for (int i = 0; i < 4; i++)
    {
        G_SERIALIZE(s, m_state.audio[i].volume);
        G_SERIALIZE(s, m_state.audio[i].feedback);
        G_SERIALIZE(s, m_state.audio[i].output);
        G_SERIALIZE(s, m_state.audio[i].lfsr_low);
        G_SERIALIZE(s, m_state.audio[i].backup);
        G_SERIALIZE(s, m_state.audio[i].control);
        G_SERIALIZE(s, m_state.audio[i].counter);
        G_SERIALIZE(s, m_state.audio[i].other);

        G_SERIALIZE(s, m_state.audio[i].internal_cycles);
        G_SERIALIZE(s, m_state.audio[i].internal_period_cycles);
        G_SERIALIZE(s, m_state.audio[i].internal_pending_ticks);
        G_SERIALIZE(s, m_state.audio[i].internal_lfsr);
        G_SERIALIZE(s, m_state.audio[i].internal_taps_mask);
        G_SERIALIZE(s, m_state.audio[i].internal_mix);
    }

    G_SERIALIZE(s, m_state.uart.tx_int_en);
    G_SERIALIZE(s, m_state.uart.rx_int_en);
    G_SERIALIZE(s, m_state.uart.par_en);
    G_SERIALIZE(s, m_state.uart.tx_open);
    G_SERIALIZE(s, m_state.uart.tx_brk);
    G_SERIALIZE(s, m_state.uart.par_even);
    G_SERIALIZE(s, m_state.uart.tx_ready);
    G_SERIALIZE(s, m_state.uart.rx_ready);
    G_SERIALIZE(s, m_state.uart.tx_empty);
    G_SERIALIZE(s, m_state.uart.par_err);
    G_SERIALIZE(s, m_state.uart.ovr_err);
    G_SERIALIZE(s, m_state.uart.fram_err);
    G_SERIALIZE(s, m_state.uart.rx_break);
    G_SERIALIZE(s, m_state.uart.par_bit);
    G_SERIALIZE(s, m_state.uart.tx_active);
    G_SERIALIZE(s, m_state.uart.tx_hold_valid);
    G_SERIALIZE(s, m_state.uart.tx_parbit);
    G_SERIALIZE(s, m_state.uart.tx_hold_data);
    G_SERIALIZE(s, m_state.uart.tx_data);
    G_SERIALIZE(s, m_state.uart.rx_data);
    G_SERIALIZE(s, m_state.uart.tx_bit_index);
    G_SERIALIZE(s, m_state.uart.prescaler);
    G_SERIALIZE(s, m_state.uart.tx_empty_bits);
    G_SERIALIZE(s, m_state.uart.tx_ready_bits);
    G_SERIALIZE(s, m_state.uart.tx_started_from_chain);
    G_SERIALIZE(s, m_state.uart.rxq_head);
    G_SERIALIZE(s, m_state.uart.rxq_count);
    G_SERIALIZE_ARRAY(s, m_state.uart.rxq_data, 2);
    G_SERIALIZE_ARRAY(s, m_state.uart.rxq_flags, 2);

    G_SERIALIZE(s, m_state.ATTEN_A);
    G_SERIALIZE(s, m_state.ATTEN_B);
    G_SERIALIZE(s, m_state.ATTEN_C);
    G_SERIALIZE(s, m_state.ATTEN_D);
    G_SERIALIZE(s, m_state.MPAN);
    G_SERIALIZE(s, m_state.MSTEREO);
    G_SERIALIZE(s, m_state.SYSCTL1);
    G_SERIALIZE(s, m_state.IODIR);
    G_SERIALIZE(s, m_state.IODAT);
    G_SERIALIZE(s, m_state.SERCTL);
    G_SERIALIZE(s, m_state.SERDAT);
    G_SERIALIZE(s, m_state.SDONEACK);
    G_SERIALIZE(s, m_state.CPUSLEEP);
    G_SERIALIZE(s, m_state.DISPCTL);
    G_SERIALIZE(s, m_state.PBKUP);
    G_SERIALIZE(s, m_state.DISPADR.value);
    G_SERIALIZE(s, m_state.irq_pending);
    G_SERIALIZE(s, m_state.irq_mask);
    G_SERIALIZE(s, m_state.frame_ready);
    G_SERIALIZE(s, m_state.render_line);
    G_SERIALIZE(s, m_state.dispadr_latch);
    G_SERIALIZE(s, m_state.rest);

    G_SERIALIZE_ARRAY(s, m_host_palette, 16);
}