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

#ifndef MIKEY_INLINE_H
#define MIKEY_INLINE_H

#include "mikey.h"
#include "suzy.h"
#include "cartridge.h"
#include "m6502.h"

INLINE bool Mikey::Clock(u32 cycles)
{
    m_debug_cycles += cycles;
    UpdateTimers(cycles);
    UpdateIRQs();

    bool ret = m_state.frame_ready;

    if (m_state.frame_ready)
    {
        m_state.frame_ready = false;
        DebugMikey("*************** FRAME READY ****************");
    }

    return ret;
}

INLINE u8 Mikey::Read(u16 address)
{
    if (address < 0xFD20)
        return ReadTimer(address);
    else if (address < 0xFD40)
        return ReadAudio(address);
    else if (address <= 0xFD50)
        return ReadAudioExtra(address);
    else if (address >= 0xFDA0 && address < 0xFDC0)
        return ReadColor(address);
    else
    {
        switch (address)
        {
        case MIKEY_INTRST:        // 0xFD80
            return m_state.irq_pending;
        case MIKEY_INTSET:        // 0xFD81
            return m_state.irq_pending;
        case MIKEY_MAGRDY0:       // 0xFD84
            DebugMikey("Reading MAGRDY0 (unused)");
            return 0x00;
        case MIKEY_MAGRDY1:       // 0xFD85
            DebugMikey("Reading MAGRDY1 (unused)");
            return 0x00;
        case MIKEY_AUDIN:         // 0xFD86
            DebugMikey("Reading AUDIN (unused)");
            return 0x80;
        case MIKEY_SYSCTL1:       // 0xFD87
            DebugMikey("Reading write-only SYSCTL1: %02X", m_state.SYSCTL1);
            return 0xFF;
        case MIKEY_MIKEYHREV:     // 0xFD88
            return 0x01;
        case MIKEY_MIKEYSREV:     // 0xFD89
            DebugMikey("Reading write-only MIKEYSREV");
            return 0xFF;
        case MIKEY_IODIR:         // 0xFD8A
            DebugMikey("Reading write-only IODIR: %02X", m_state.IODIR);
            return 0xFF;
        case MIKEY_IODAT:         // 0xFD8B
        {
            u8 ret = 0x00;
            if (IS_SET_BIT(m_state.IODIR, 0))
                ret |= IS_SET_BIT(m_state.IODAT, 0) ? 0x01 : 0x00;
            if (IS_SET_BIT(m_state.IODIR, 1))
                ret |= IS_SET_BIT(m_state.IODAT, 1) ? 0x02 : 0x00;
            if (IS_SET_BIT(m_state.IODIR, 2))
                ret |= IS_SET_BIT(m_state.IODAT, 2) ? 0x04 : 0x00;
            if (IS_SET_BIT(m_state.IODIR, 3))
                ret |= (IS_SET_BIT(m_state.IODAT, 3) && m_state.rest) ? 0x08 : 0x00;
            if (IS_SET_BIT(m_state.IODIR, 4))
                ret |= IS_SET_BIT(m_state.IODAT, 4) ? 0x10 : 0x00;

            ret |= 0x04;

            return ret;
        }
        case MIKEY_SERCTL:        // 0xFD8C
            return m_state.SERCTL;
        case MIKEY_SERDAT:        // 0xFD8D
            return m_state.SERDAT;
        case MIKEY_SDONEACK:      // 0xFD90
            DebugMikey("Reading write-only SDONEACK: %02X", m_state.SDONEACK);
            return 0xFF;
        case MIKEY_CPUSLEEP:      // 0xFD91
            DebugMikey("Reading write-only CPUSLEEP: %02X", m_state.CPUSLEEP);
            return 0xFF;
        case MIKEY_DISPCTL:       // 0xFD92
            DebugMikey("Reading write-only DISPCTL: %02X", m_state.DISPCTL);
            return 0xFF;
        case MIKEY_PBKUP:         // 0xFD93
            DebugMikey("Reading write-only PBKUP: %02X", m_state.PBKUP);
            return 0xFF;
        case MIKEY_DISPADRL:      // 0xFD94
            DebugMikey("Reading write-only DISPADRL: %02X", m_state.DISPADR.low);
            return 0xFF;
        case MIKEY_DISPADRH:      // 0xFD95
            DebugMikey("Reading write-only DISPADRH: %02X", m_state.DISPADR.high);
            return 0xFF;
        case MIKEY_MTEST0:        // 0xFD9C
            DebugMikey("Reading MTEST0 (unused)");
            return 0xFF;
        case MIKEY_MTEST1:        // 0xFD9D
            DebugMikey("Reading MTEST1 (unused)");
            return 0xFF;
        case MIKEY_MTEST2:        // 0xFD9E
            DebugMikey("Reading MTEST2 (unused)");
            return 0xFF;
        default:
            assert(false && "Unhandled Mikey Read Address");
            DebugMikey("Register READ called with unknown address: %04X", address);
            return 0xFF;
        }
    }

    assert(false && "Unhandled Mikey Read Address");
    return 0xFF;
}

INLINE void Mikey::Write(u16 address, u8 value)
{
    if (address < 0xFD20)
        WriteTimer(address, value);
    else if (address < 0xFD40)
        WriteAudio(address, value);
    else if (address <= 0xFD50)
        WriteAudioExtra(address, value);
    else if (address >= 0xFDA0 && address < 0xFDC0)
        WriteColor(address, value);
    else
    {
        switch (address)
        {
        case MIKEY_INTRST:        // 0xFD80
            DebugMikey("Clearing IRQs: %02X (was %02X)", m_state.irq_pending & ~value, m_state.irq_pending);
            m_state.irq_pending &= ~value;
            if (m_state.irq_pending != 0)
                UpdateIRQs();
            break;
        case MIKEY_INTSET:        // 0xFD81
            DebugMikey("Setting IRQs: %02X (was %02X)", m_state.irq_pending | value, m_state.irq_pending);
            m_state.irq_pending |= value;
            if (m_state.irq_pending != 0)
                UpdateIRQs();
            break;
        case MIKEY_MAGRDY0:       // 0xFD84
            DebugMikey("Writing MAGRDY0 (unused): %02X", value);
            break;
        case MIKEY_MAGRDY1:       // 0xFD85
            DebugMikey("Writing MAGRDY1 (unused): %02X", value);
            break;
        case MIKEY_AUDIN:         // 0xFD86
            DebugMikey("Writing AUDIN (unused): %02X", value);
            break;
        case MIKEY_SYSCTL1:       // 0xFD87
            DebugMikey("Setting SYSCTL1 to %02X (was %02X)", value, m_state.SYSCTL1);
            m_cartridge->ShiftRegisterStrobe(value & 0x01);
            m_state.SYSCTL1 = value;
            break;
        case MIKEY_MIKEYHREV:     // 0xFD88
            DebugMikey("Writing to read-only MIKEYHREV: %02X", value);
            break;
        case MIKEY_MIKEYSREV:     // 0xFD89
            DebugMikey("Writing MIKEYSREV (unused): %02X", value);
            break;
        case MIKEY_IODIR:         // 0xFD8A
            DebugMikey("Setting IODIR to %02X (was %02X)", value, m_state.IODIR);
            m_state.IODIR = value;
            break;
        case MIKEY_IODAT:         // 0xFD8B
            DebugMikey("Setting IODAT to %02X (was %02X)", value, m_state.IODAT);
            m_cartridge->ShiftRegisterBit(value & 0x02);
            m_state.IODAT = value;
            break;
        case MIKEY_SERCTL:        // 0xFD8C
            m_state.SERCTL = value;
            break;
        case MIKEY_SERDAT:        // 0xFD8D
            m_state.SERDAT = value;
            break;
        case MIKEY_SDONEACK:      // 0xFD90
            DebugMikey("Setting SDONEACK to %02X (was %02X)", value, m_state.SDONEACK);
            m_state.SDONEACK = value;
            break;
        case MIKEY_CPUSLEEP:      // 0xFD91
            DebugMikey("Setting CPUSLEEP to %02X (was %02X)", value, m_state.CPUSLEEP);
            m_state.CPUSLEEP = value;
            break;
        case MIKEY_DISPCTL:       // 0xFD92
            DebugMikey("Setting DISPCTL to %02X (was %02X)", value, m_state.DISPCTL);
            m_state.DISPCTL = value;
            break;
        case MIKEY_PBKUP:         // 0xFD93
            DebugMikey("Setting PBKUP to %02X (was %02X)", value, m_state.PBKUP);
            m_state.PBKUP = value;
            break;
        case MIKEY_DISPADRL:      // 0xFD94
            DebugMikey("Setting DISPADR low to %02X (was %02X)", value, m_state.DISPADR.low);
            m_state.DISPADR.low = value;
            break;
        case MIKEY_DISPADRH:      // 0xFD95
            DebugMikey("Setting DISPADR high to %02X (was %02X)", value, m_state.DISPADR.high);
            m_state.DISPADR.high = value;
            break;
        case MIKEY_MTEST0:        // 0xFD9C
            DebugMikey("Writing MTEST0 (unused): %02X", value);
            break;
        case MIKEY_MTEST1:        // 0xFD9D
            DebugMikey("Writing MTEST1 (unused): %02X", value);
            break;
        case MIKEY_MTEST2:        // 0xFD9E
            DebugMikey("Writing MTEST2 (unused): %02X", value);
            break;
        default:
            //assert(false && "Unhandled Mikey Write Address");
            DebugMikey("Register WRITE called with unknown address: %04X, value: %02X", address, value);
            break;
        }
    }
}

INLINE Mikey::Mikey_State* Mikey::GetState()
{
    return &m_state;
}

INLINE void Mikey::SetBuffer(u8* frame_buffer)
{
    m_frame_buffer = frame_buffer;
}

INLINE u8* Mikey::GetBuffer()
{
    return m_frame_buffer;
}

INLINE u32* Mikey::GetRGBA8888Palette()
{
    return m_rgba8888_palette;
}

INLINE u16* Mikey::GetRGB565Palette()
{
    return m_rgb565_palette;
}

INLINE u8 Mikey::ReadColor(u16 address)
{
    assert(address >= MIKEY_GREEN0 && address <= MIKEY_BLUEREDF);

    int color_index = address & 0xF;

    if (address < MIKEY_BLUERED0)
        return m_state.colors[color_index].green;
    else
        return m_state.colors[color_index].bluered;
}

INLINE void Mikey::WriteColor(u16 address, u8 value)
{
    assert(address >= MIKEY_GREEN0 && address <= MIKEY_BLUEREDF);

    int color_index = address & 0xF;

    if (address < MIKEY_BLUERED0)
        m_state.colors[color_index].green = value;
    else
        m_state.colors[color_index].bluered = value;

    m_host_palette[color_index] = ((m_state.colors[color_index].green & 0x0F) << 8) | (m_state.colors[color_index].bluered & 0xFF);
}


INLINE u8 Mikey::ReadTimer(u16 address)
{
    assert(address >= MIKEY_TIM0BKUP && address <= MIKEY_TIM7CTLB);

    int reg = address & 3;
    int i = (address >> 2) & 7;

    switch (reg)
    {
    case 0:
        return m_state.timers[i].backup;
    case 1:
        return m_state.timers[i].control_a;
    case 2:
        return m_state.timers[i].counter;
    case 3:
        return m_state.timers[i].control_b;
    default:
        return 0xFF;
    }
}

INLINE void Mikey::WriteTimer(u16 address, u8 value)
{
    assert(address >= MIKEY_TIM0BKUP && address <= MIKEY_TIM7CTLB);

    int reg = address & 3;
    int i = (address >> 2) & 7;
    GLYNX_Mikey_Timer* t = &m_state.timers[i];

    switch (reg)
    {
    case 0:
        DebugMikey("Setting Timer %d Backup to %02X (was %02X)", i, value, t->backup);
        t->backup = value;
        break;
    case 1:
        DebugMikey("Setting Timer %d Control A to %02X (was %02X)", i, value, t->control_a);
        t->control_a = value;

        if (i == 4)
            t->internal_period_cycles = k_mikey_timer4_period_cycles[value & 0x07];
        else
            t->internal_period_cycles = k_mikey_timerX_period_cycles[value & 0x07];

        if (IS_SET_BIT(value, 6) || IS_SET_BIT(value, 3))
            t->internal_cycles = 0;

        if (IS_SET_BIT(value, 7))
            m_state.irq_mask = SET_BIT(m_state.irq_mask, i);
        else
            m_state.irq_mask = UNSET_BIT(m_state.irq_mask, i);

        if (IS_SET_BIT(value, 6))
            t->control_b = UNSET_BIT(t->control_b, 3);

        break;
    case 2:
        DebugMikey("Setting Timer %d Counter to %02X (was %02X)", i, value, m_state.timers[i].counter);
        t->counter = value;
        break;
    case 3:
        DebugMikey("Setting Timer %d Control B to %02X (was %02X)", i, value, m_state.timers[i].control_b);
        t->control_b = value;
        break;
    default:
        break;
    }
}

INLINE u8 Mikey::ReadAudio(u16 address)
{
    assert(address >= MIKEY_AUD0VOL && address <= MIKEY_AUD3MISC);

    int reg = address & 7;
    int channel = (address >> 7) & 3;

    switch (reg)
    {
    case 0:
        return m_state.audio[channel].volume;
    case 1:
        return m_state.audio[channel].shift_feedback;
    case 2:
        return m_state.audio[channel].output_value;
    case 3:
        return m_state.audio[channel].left_shift;
    case 4:
        return m_state.audio[channel].timer_backup;
    case 5:
        return m_state.audio[channel].control;
    case 6:
        return m_state.audio[channel].count;
    case 7:
        return m_state.audio[channel].misc;
    default:
        return 0xFF;
    }
}

INLINE void Mikey::WriteAudio(u16 address, u8 value)
{
    assert(address >= MIKEY_AUD0VOL && address <= MIKEY_AUD3MISC);

    int reg = address & 7;
    int channel = (address >> 7) & 3;

    switch (reg)
    {
    case 0:
        m_state.audio[channel].volume = value;
        break;
    case 1:
        m_state.audio[channel].shift_feedback = value;
        break;
    case 2:
        m_state.audio[channel].output_value = value;
        break;
    case 3:
        m_state.audio[channel].left_shift = value;
        break;
    case 4:
        m_state.audio[channel].timer_backup = value;
        break;
    case 5:
        m_state.audio[channel].control = value;
        break;
    case 6:
        m_state.audio[channel].count = value;
        break;
    case 7:
        m_state.audio[channel].misc = value;
        break;
    default:
        break;
    }
}

INLINE u8 Mikey::ReadAudioExtra(u16 address)
{
    assert(address >= MIKEY_ATTEN_A && address <= MIKEY_MSTEREO);

    switch (address)
    {
    case MIKEY_ATTEN_A:       // 0xFD40
        return m_state.ATTEN_A;
    case MIKEY_ATTEN_B:       // 0xFD41
        return m_state.ATTEN_B;
    case MIKEY_ATTEN_C:       // 0xFD42
        return m_state.ATTEN_C;
    case MIKEY_ATTEN_D:       // 0xFD43
        return m_state.ATTEN_D;
    case MIKEY_MPAN:          // 0xFD44
        return m_state.MPAN;
    case MIKEY_MSTEREO:       // 0xFD50
        return m_state.MSTEREO;
    default:
        DebugMikey("Audio Extra READ called with unknown address: %04X", address);
        return 0xFF;
    }
}

INLINE void Mikey::WriteAudioExtra(u16 address, u8 value)
{
    assert(address >= MIKEY_ATTEN_A && address <= MIKEY_MSTEREO);

    switch (address)
    {
    case MIKEY_ATTEN_A:       // 0xFD40
        m_state.ATTEN_A = value;
        break;
    case MIKEY_ATTEN_B:       // 0xFD41
        m_state.ATTEN_B = value;
        break;
    case MIKEY_ATTEN_C:       // 0xFD42
        m_state.ATTEN_C = value;
        break;
    case MIKEY_ATTEN_D:       // 0xFD43
        m_state.ATTEN_D = value;
        break;
    case MIKEY_MPAN:          // 0xFD44
        m_state.MPAN = value;
        break;
    case MIKEY_MSTEREO:       // 0xFD50
        m_state.MSTEREO = value;
        break;
    default:
        DebugMikey("Audio Extra WRITE called with unknown address: %04X, value: %02X", address, value);
        break;
    }
}

INLINE void Mikey::UpdateTimers(u32 cycles)
{
    for (int i = 0; i < 8; i++)
    {
        GLYNX_Mikey_Timer* t = &m_state.timers[i];

        // if timer is disabled, skip it
        if (IS_NOT_SET_BIT(t->control_a, 3))
            continue;

        // If reset timer-done is enabled, clear timer-done bit
        if (IS_SET_BIT(t->control_a, 6))
            t->control_b = UNSET_BIT(t->control_b, 3);

        // clear borrow out
        t->control_b = UNSET_BIT(t->control_b, 0);

        int link = k_mikey_timer_forward_links[i];

        // clear borrow in to linked timer
        if (link >= 0)
            m_state.timers[link].control_b = UNSET_BIT(m_state.timers[link].control_b, 1); 

        // if reload is disabled and timer is done, skip it
        // if (IS_NOT_SET_BIT(t->control_a, 4) && IS_SET_BIT(t->control_b, 3))
        //     continue;

        int tick = 0;

        // if linked timer in linked mode
        if (t->internal_period_cycles == 0)
        {
            if (IS_SET_BIT(t->control_b, 1))
                tick = 1;
        }
        // normal timer mode
        else
        {
            t->internal_cycles += cycles;

            while (t->internal_cycles >= t->internal_period_cycles)
            {
                t->internal_cycles -= t->internal_period_cycles;
                tick++;
            }
        }

        for (int j = 0; j < tick; j++)
        {
            if (t->counter > 0)
            {
                t->counter--;
            }
            else
            {
                // timer done
                t->control_b = SET_BIT(t->control_b, 3);

                // borrow out
                t->control_b = SET_BIT(t->control_b, 0);

                if (link >= 0)
                {
                    // borrow in to linked timer
                    m_state.timers[link].control_b = SET_BIT(m_state.timers[link].control_b, 1);
                }

                // if reload is set
                if (IS_SET_BIT(t->control_a, 4))
                {
                    t->counter = t->backup;
                }

                // irq if enabled and not timer 4
                if (IS_SET_BIT(t->control_a, 7) && (i != 4))
                    m_state.irq_pending = SET_BIT(m_state.irq_pending, i);

                if (i == 0)
                {
                    HorizontalBlank();
                }
                else if (i == 2)
                {
                    VerticalBlank();
                }
            }
        }
    }
}

INLINE void Mikey::UpdateIRQs()
{
    m_m6502->AssertIRQ((m_state.irq_pending & m_state.irq_mask) != 0);
}

INLINE void Mikey::HorizontalBlank()
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

INLINE void Mikey::VerticalBlank()
{
    Debug("===> VBLANK. DISPADR %04X. Cycles: %d", m_state.dispadr_latch, m_debug_cycles);
    m_state.frame_ready = true;
    //m_state.render_line = 0;
    m_debug_cycles = 0;
}

INLINE void Mikey::LineDMA(int line)
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
inline void Mikey::LineDMATemplate(int line)
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
inline void Mikey::LineDMABlankTemplate(int line)
{
    assert(line >= 0 && line < GLYNX_SCREEN_HEIGHT);

    u8* dst_line_ptr = m_frame_buffer + (line * GLYNX_SCREEN_WIDTH * bytes_per_pixel);
    size_t line_size = GLYNX_SCREEN_WIDTH * bytes_per_pixel;
    memset(dst_line_ptr, 0, line_size);
}

#endif /* MIKEY_INLINE_H */
