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
#include "audio.h"
#include "suzy.h"
#include "media.h"
#include "m6502.h"
#include "bit_ops.h"
#include "bus.h"
#include "lcd_screen.h"

INLINE bool Mikey::Clock(u32 cycles)
{
    UpdateTimers(cycles);
    UpdateVideo(cycles);
    UpdateAudio(cycles);
    UpdateIRQs();

    bool ret = m_state.frame_ready;

    if (m_state.frame_ready)
    {
        m_state.frame_ready = false;
        DebugMikey("*************** FRAME READY ****************");
    }

    return ret;
}

template<bool debug>
INLINE u8 Mikey::Read(u16 address)
{
    if (!debug)
    {
        m_bus->InjectCycles(k_bus_cycles_mikey_read);
    }

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
            DebugMikey("Reading INTRST: %02X", m_state.irq_pending);
            return m_state.irq_pending;
        case MIKEY_INTSET:        // 0xFD81
            DebugMikey("Reading INTSET: %02X", m_state.irq_pending);
            return m_state.irq_pending;
        case MIKEY_MAGRDY0:       // 0xFD84
            DebugMikey("Reading MAGRDY0 (unused): 00");
            return 0x00;
        case MIKEY_MAGRDY1:       // 0xFD85
            DebugMikey("Reading MAGRDY1 (unused): 00");
            return 0x00;
        case MIKEY_AUDIN:         // 0xFD86
            DebugMikey("Reading AUDIN (unused): 80");
            return 0x80;
        case MIKEY_SYSCTL1:       // 0xFD87
            DebugMikey("Reading write-only SYSCTL1: FF");
            return 0xFF;
        case MIKEY_MIKEYHREV:     // 0xFD88
            return 0x01;
        case MIKEY_MIKEYSREV:     // 0xFD89
            DebugMikey("Reading write-only MIKEYSREV: FF");
            return 0xFF;
        case MIKEY_IODIR:         // 0xFD8A
            DebugMikey("Reading write-only IODIR: FF");
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

            //DebugMikey("Reading IODAT: %02X", ret);

            return ret;
        }
        case MIKEY_SERCTL:        // 0xFD8C
        {
            u8 status = 0;
            status |= (m_state.uart.tx_ready ? 0x80 : 0x00);
            status |= (m_state.uart.rx_ready ? 0x40 : 0x00);
            status |= (m_state.uart.tx_empty ? 0x20 : 0x00);
            status |= (m_state.uart.par_err ? 0x10 : 0x00);
            status |= (m_state.uart.ovr_err ? 0x08 : 0x00);
            status |= (m_state.uart.fram_err ? 0x04 : 0x00);
            status |= (m_state.uart.rx_break ? 0x02 : 0x00);
            status |= (m_state.uart.par_bit  ? 0x01 : 0x00);
            DebugMikey("Reading SERCTL: %02X", status);
            return status;
        }
        case MIKEY_SERDAT:        // 0xFD8D
        {
            u8 ret = m_state.uart.rx_data;
            DebugMikey("Reading SERDAT (RX): %02X", ret);

            if (!debug)
            {
                if (m_state.uart.rxq_count > 0)
                {
                    m_state.uart.rxq_head ^= 1;
                    m_state.uart.rxq_count--;
                }

                UartRxReflectHead();
                UartRelevelIRQ();
            }
            return ret;
        }
        case MIKEY_SDONEACK:      // 0xFD90
            DebugMikey("Reading write-only SDONEACK: FF");
            return 0xFF;
        case MIKEY_CPUSLEEP:      // 0xFD91
            DebugMikey("Reading write-only CPUSLEEP: FF");
            return 0xFF;
        case MIKEY_DISPCTL:       // 0xFD92
            DebugMikey("Reading write-only DISPCTL: FF");
            return 0xFF;
        case MIKEY_PBKUP:         // 0xFD93
            DebugMikey("Reading write-only PBKUP: FF");
            return 0xFF;
        case MIKEY_DISPADRL:      // 0xFD94
            DebugMikey("Reading write-only DISPADRL: FF");
            return 0xFF;
        case MIKEY_DISPADRH:      // 0xFD95
            DebugMikey("Reading write-only DISPADRH: FF");
            return 0xFF;
        case MIKEY_MTEST0:        // 0xFD9C
            DebugMikey("Reading MTEST0 (unused): FF");
            return 0xFF;
        case MIKEY_MTEST1:        // 0xFD9D
            DebugMikey("Reading MTEST1 (unused): FF");
            return 0xFF;
        case MIKEY_MTEST2:        // 0xFD9E
            DebugMikey("Reading MTEST2 (unused): FF");
            return 0xFF;
        default:
            //assert(false && "Unhandled Mikey Read Address");
            DebugMikey("Register READ called with unknown address: %04X", address);
            return 0xFF;
        }
    }

    assert(false && "Unhandled Mikey Read Address");
    return 0xFF;
}

template<bool debug>
INLINE void Mikey::Write(u16 address, u8 value)
{
    if (!debug)
    {
        m_bus->InjectCycles(k_bus_cycles_mikey_write);
    }

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
        {
            DebugMikey("Clearing IRQs: %02X (was %02X)", value, m_state.irq_pending);
            m_state.irq_pending &= ~value;
            UartRelevelIRQ();
            break;
        }
        case MIKEY_INTSET:        // 0xFD81
            DebugMikey("Setting IRQs: %02X (was %02X)", value, m_state.irq_pending);
            m_state.irq_pending |= value;
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
            m_media->ShiftRegisterStrobe(IS_SET_BIT(value, 0));
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
            m_media->ShiftRegisterBit(IS_SET_BIT(value, 1));
            m_state.IODAT = value;
            break;
        case MIKEY_SERCTL:        // 0xFD8C
        {
            DebugMikey("Setting SERCTL to %02X (was %02X)", value, m_state.SERCTL);
            m_state.SERCTL = value;

            m_state.uart.tx_int_en = IS_SET_BIT(value, 7);
            m_state.uart.rx_int_en = IS_SET_BIT(value, 6);
            m_state.uart.par_en = IS_SET_BIT(value, 4);
            m_state.uart.tx_open = IS_SET_BIT(value, 2);
            m_state.uart.tx_brk = IS_SET_BIT(value, 1);
            m_state.uart.par_even = IS_SET_BIT(value, 0);

            if (IS_SET_BIT(value, 3)) // RESETERR
            {
                m_state.uart.par_err  = false;
                m_state.uart.fram_err = false;
                m_state.uart.ovr_err  = false;
                m_state.uart.rx_break = false;
            }

            if (m_state.uart.tx_brk)
            {
                m_state.uart.tx_empty = false;
                m_state.uart.tx_ready = false;
                m_state.uart.tx_active = true;
            }
            else
            {
                if (!m_state.uart.tx_active && !m_state.uart.tx_hold_valid)
                {
                    m_state.uart.tx_empty = true;
                    m_state.uart.tx_ready = true;
                }
            }

            if (m_state.uart.tx_int_en || m_state.uart.rx_int_en)
                m_state.irq_mask = SET_BIT(m_state.irq_mask, 4);
            else
                m_state.irq_mask = UNSET_BIT(m_state.irq_mask, 4);

            UartRelevelIRQ();
            break;
        }
        case MIKEY_SERDAT:        // 0xFD8D
        {
            DebugMikey("Setting SERDAT (TX) to %02X", value);

            if (!m_state.uart.tx_active && !m_state.uart.tx_brk)
            {
                UartBeginFrame(value);
                m_state.uart.tx_ready_bits = 2;
                m_state.uart.tx_started_from_chain = false;
            }
            else
            {
                m_state.uart.tx_hold_data = value;
                m_state.uart.tx_hold_valid = true;
                m_state.uart.tx_ready = false;
                m_state.uart.tx_empty = false;

                if (m_state.uart.tx_brk)
                    m_state.uart.tx_suppress_eof_loopback = true;
            }

            UartRelevelIRQ();
            break;
        }
        case MIKEY_SDONEACK:      // 0xFD90
            DebugMikey("Setting SDONEACK to %02X (was %02X)", value, m_state.SDONEACK);
            m_state.SDONEACK = value;
            break;
        case MIKEY_CPUSLEEP:      // 0xFD91
            DebugMikey("Setting CPUSLEEP to %02X (was %02X)", value, m_state.CPUSLEEP);
            if ((value == 0) && m_suzy->IsBlitterBusy())
                m_m6502->Halt(true);
            m_state.CPUSLEEP = value;
            break;
        case MIKEY_DISPCTL:       // 0xFD92
            DebugMikey("Setting DISPCTL to %02X (was %02X)", value, m_state.DISPCTL);
            m_state.DISPCTL = value;
            break;
        case MIKEY_PBKUP:         // 0xFD93
            DebugMikey("Setting PBKUP to %02X (was %02X)", value, m_state.PBKUP);
            m_state.PBKUP = value;
            m_lcd_screen->ConfigureLineTiming();
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

INLINE LcdScreen* Mikey::GetLcdScreen()
{
    return m_lcd_screen;
}

inline u8 Mikey::ReadColor(u16 address)
{
    assert(address >= MIKEY_GREEN0 && address <= MIKEY_BLUEREDF);

    int color_index = address & 0xF;

    if (address < MIKEY_BLUERED0)
        return m_state.colors[color_index].green;
    else
        return m_state.colors[color_index].bluered;
}

inline void Mikey::WriteColor(u16 address, u8 value)
{
    assert(address >= MIKEY_GREEN0 && address <= MIKEY_BLUEREDF);

    int color_index = address & 0xF;

    if (address < MIKEY_BLUERED0)
        m_state.colors[color_index].green = value;
    else
        m_state.colors[color_index].bluered = value;

    m_lcd_screen->UpdatePalette(color_index, ((m_state.colors[color_index].green & 0x0F) << 8) | (m_state.colors[color_index].bluered & 0xFF));
}


inline u8 Mikey::ReadTimer(u16 address)
{
    assert(address >= MIKEY_TIM0BKUP && address <= MIKEY_TIM7CTLB);

    m_bus->InjectCycles(k_bus_cycles_timer);

    int reg = address & 3;
    int i = (address >> 2) & 7;
    GLYNX_Mikey_Timer* t = &m_state.timers[i];

    switch (reg)
    {
    case 0:
        DebugMikey("Reading Timer %d Backup: %02X", i, t->backup);
        return t->backup;
    case 1:
        DebugMikey("Reading Timer %d Control A: %02X", i, t->control_a);
        return t->control_a;
    case 2:
        DebugMikey("Reading Timer %d Counter: %02X", i, t->counter);
        return t->counter;
    case 3:
        DebugMikey("Reading Timer %d Control B: %02X", i, t->control_b);
        return t->control_b;
    default:
        return 0xFF;
    }
}

inline void Mikey::WriteTimer(u16 address, u8 value)
{
    assert(address >= MIKEY_TIM0BKUP && address <= MIKEY_TIM7CTLB);

    m_bus->InjectCycles(k_bus_cycles_timer);

    int reg = address & 3;
    int i = (address >> 2) & 7;
    GLYNX_Mikey_Timer* t = &m_state.timers[i];

    switch (reg)
    {
    case 0:
        DebugMikey("Setting Timer %d Backup to %02X (was %02X)", i, value, t->backup);
        t->backup = value;
        if (i == 0) // HCOUNT timer
            m_lcd_screen->ConfigureLineTiming();
        break;
    case 1:
    {
        DebugMikey("Setting Timer %d Control A to %02X (was %02X)", i, value, t->control_a);

        u8 old_control_a = t->control_a;
        u8 old_prescaler = old_control_a & 0x07;
        u8 new_prescaler = value & 0x07;

        t->control_a = value;

        t->internal_period_cycles = k_mikey_timer_period_cycles[new_prescaler];

        // Re-sync ONLY when clock source changes or when enabling counting from disabled
        bool prescaler_changed = (old_prescaler != new_prescaler);
        bool enable_count_rising = IS_NOT_SET_BIT(old_control_a, 3) && IS_SET_BIT(value, 3);

        if (prescaler_changed || enable_count_rising)
        {
            if (enable_count_rising)
                t->internal_cycles = (t->internal_period_cycles / 2);
            else
                t->internal_cycles = 0;

            t->internal_pending_ticks = 0;

            if (i == 0) // HCOUNT timer
                m_lcd_screen->ConfigureLineTiming();
        }

        // Timer 4 (UART) does NOT use CTRLA[7] for its IRQ; it's masked by SERCTL
        if (i != 4)
        {
            if (IS_SET_BIT(value, 7))
                m_state.irq_mask = SET_BIT(m_state.irq_mask, i);
            else
                m_state.irq_mask = UNSET_BIT(m_state.irq_mask, i);
        }

        // RESET TIMER DONE is level-triggered
        if (IS_SET_BIT(value, 6))
            t->control_b = UNSET_BIT(t->control_b, 3) | 0xF0;

        break;
    }
    case 2:
        DebugMikey("Setting Timer %d Counter to %02X (was %02X)", i, value, m_state.timers[i].counter);
        t->counter = value;
        t->internal_cycles = (t->internal_period_cycles / 2);
        break;
    case 3:
        DebugMikey("Setting Timer %d Control B to %02X (was %02X)", i, value, m_state.timers[i].control_b);
        if (IS_NOT_SET_BIT(t->control_b, 1) && IS_SET_BIT(value, 1))
            BorrowInTimer(i, t);
        t->control_b = value & 0xF8;
        break;
    default:
        break;
    }
}

inline u8 Mikey::ReadAudio(u16 address)
{
    assert(address >= MIKEY_AUD0VOL && address <= MIKEY_AUD3MISC);

    m_bus->InjectCycles(k_bus_cycles_audio);

    int reg = address & 7;
    int i = ((address - MIKEY_AUD0VOL) >> 3) & 3;
    GLYNX_Mikey_Audio* c = &m_state.audio[i];

    switch (reg)
    {
    case 0:
        return c->volume;
    case 1:
        return c->feedback;
    case 2:
        return c->output;
    case 3:
        return c->lfsr_low;
    case 4:
        return c->backup;
    case 5:
        return c->control;
    case 6:
        return c->counter;
    case 7:
        return c->other;
    default:
        return 0xFF;
    }
}

inline void Mikey::WriteAudio(u16 address, u8 value)
{
    assert(address >= MIKEY_AUD0VOL && address <= MIKEY_AUD3MISC);

    m_bus->InjectCycles(k_bus_cycles_audio);

#ifndef GLYNX_DISABLE_VGMRECORDER
    if (m_audio->IsVgmRecording())
        m_audio->GetVgmRecorder()->WriteMikey(address, value);
#endif

    int reg = address & 7;
    int i = ((address - MIKEY_AUD0VOL) >> 3) & 3;
    GLYNX_Mikey_Audio* c = &m_state.audio[i];

    switch (reg)
    {
    case 0:
        c->volume = value;
        break;
    case 1:
        c->feedback = value;
        RebuildTapsMask(c);
        break;
    case 2:
        c->output = value;
        break;
    case 3:
        c->lfsr_low = value;
        RebuildLFSR(c);
        break;
    case 4:
        c->backup = value;
        CalculateCutoff(i);
        break;
    case 5:
    {
        u8 old_control = c->control;
        u8 old_prescaler = old_control & 0x07;
        u8 new_prescaler = value & 0x07;

        c->control = value;
        c->internal_period_cycles = k_mikey_timer_period_cycles[new_prescaler];

        bool prescaler_changed = (old_prescaler != new_prescaler);
        bool enable_count_rising = IS_NOT_SET_BIT(old_control, 3) && IS_SET_BIT(value, 3);

        if (prescaler_changed || enable_count_rising)
        {
            if (enable_count_rising)
                c->internal_cycles = (c->internal_period_cycles / 2);
            else
                c->internal_cycles = 0;
            c->internal_pending_ticks = 0;
            CalculateCutoff(i);
        }

        if (IS_SET_BIT(value, 6))
            c->other = UNSET_BIT(c->other, 3);

        if (IS_NOT_SET_BIT(c->control, 3))
            c->internal_mix = true;

        RebuildTapsMask(c);
        break;
    }
    case 6:
        c->counter = value;
        c->internal_cycles = (c->internal_period_cycles / 2);
        break;
    case 7:
        if (IS_NOT_SET_BIT(c->other, 1) && IS_SET_BIT(value, 1))
            BorrowInChannel(i, c);
        c->other = value & 0xF8;
        RebuildLFSR(c);
        break;
    default:
        break;
    }
}

inline u8 Mikey::ReadAudioExtra(u16 address)
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

inline void Mikey::WriteAudioExtra(u16 address, u8 value)
{
    assert(address >= MIKEY_ATTEN_A && address <= MIKEY_MSTEREO);

#ifndef GLYNX_DISABLE_VGMRECORDER
    if (m_audio->IsVgmRecording())
        m_audio->GetVgmRecorder()->WriteMikey(address, value);
#endif

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

        // Is not enabled?
        if (IS_NOT_SET_BIT(t->control_a, 3))
            continue;

        // Clear transient status bits for this update
        t->control_b = UNSET_BIT(t->control_b, 0); // Borrow Out
        t->control_b = UNSET_BIT(t->control_b, 1); // Borrow In

        // Reset Timer Done is level-triggered
        if (IS_SET_BIT(t->control_a, 6))
            t->control_b = UNSET_BIT(t->control_b, 3);

        // One-shot already done?
        if (IS_NOT_SET_BIT(t->control_a, 4) && IS_SET_BIT(t->control_b, 3))
            continue;

        int tick = 0;

        // Linked mode: consume pending ticks queued by the previous timer
        if (t->internal_period_cycles == 0)
        {
            tick = t->internal_pending_ticks;
            t->internal_pending_ticks = 0;
        }
        // Prescaled/free-running mode
        else
        {
            t->internal_cycles += cycles;

            if (t->internal_cycles >= t->internal_period_cycles)
            {
                tick = t->internal_cycles / t->internal_period_cycles;
                t->internal_cycles -= tick * t->internal_period_cycles;
            }
        }

        // Any clocks this update? Reflect it on BORROW-IN
        if (tick > 0)
            t->control_b = SET_BIT(t->control_b, 1);

        while (tick-- > 0)
        {
            if (!BorrowInTimer(i, t))
                break;
        }
    }
}

INLINE bool Mikey::BorrowInTimer(int i, GLYNX_Mikey_Timer* t)
{
    if (t->counter > 0)
    {
        t->counter--;
        if (t->internal_period_cycles != 0)
            t->control_b = UNSET_BIT(t->control_b, 2); // reset Last clock
    }
    else
    {
        t->control_b = SET_BIT(t->control_b, 0); // Borrow Out
        if (t->internal_period_cycles != 0)
            t->control_b = SET_BIT(t->control_b, 2); // Last clock
        t->control_b = SET_BIT(t->control_b, 3); // Timer Done

        int link = k_mikey_timer_forward_links[i];

        // Propagate link tick to next timer
        if (link >= 0)
        {
            if (link < 8)
            {
                m_state.timers[link].internal_pending_ticks++;
                m_state.timers[link].control_b = SET_BIT(m_state.timers[link].control_b, 1);
            }
            else
            {
                m_state.audio[0].internal_pending_ticks++;
                m_state.audio[0].other = SET_BIT(m_state.audio[0].other, 1);
            }
        }

        bool one_shot = IS_NOT_SET_BIT(t->control_a, 4);

        if (!one_shot)
            t->counter = t->backup;

        // IRQ on borrow attempt (except timer 4 / UART baud)
        if (IS_SET_BIT(t->control_a, 7) && (i != 4))
            m_state.irq_pending = SET_BIT(m_state.irq_pending, i);

        if (likely(i == 0))
            HorizontalBlank();
        else if (i == 4)
            UartClock();

        // In one-shot, after DONE we must not consume more clocks
        if (one_shot && IS_SET_BIT(t->control_b, 3))
            return false;
    }

    return true;
}

INLINE void Mikey::UpdateAudio(u32 cycles)
{
    for (int i = 0; i < 4; i++)
    {
        GLYNX_Mikey_Audio* c = &m_state.audio[i];

        // Is not enabled?
        if (IS_NOT_SET_BIT(c->control, 3))
            continue;

        // Clear transient status bits for this update
        c->other = UNSET_BIT(c->other, 0); // Borrow Out
        c->other = UNSET_BIT(c->other, 1); // Borrow In

        // Reset Timer Done is level-triggered
        if (IS_SET_BIT(c->control, 6))
            c->other = UNSET_BIT(c->other, 3);

        // One-shot already done?
        if (IS_NOT_SET_BIT(c->control, 4) && IS_SET_BIT(c->other, 3))
            continue;

        int tick = 0;

        // Linked mode: consume pending ticks queued by the previous timer
        if (c->internal_period_cycles == 0)
        {
            tick = c->internal_pending_ticks;
            c->internal_pending_ticks = 0;
        }
        // Prescaled/free-running mode
        else
        {
            c->internal_cycles += cycles;

            if (c->internal_cycles >= c->internal_period_cycles)
            {
                tick = c->internal_cycles / c->internal_period_cycles;
                c->internal_cycles -= tick * c->internal_period_cycles;
            }
        }

        // Any clocks this update? Reflect it on BORROW-IN
        if (tick > 0)
            c->other = SET_BIT(c->other, 1);

        while (tick-- > 0)
        {
            if (!BorrowInChannel(i, c))
                break;
        }
    }
}

INLINE bool Mikey::BorrowInChannel(int i, GLYNX_Mikey_Audio* c)
{
    if (c->counter > 0)
    {
        c->counter--;
        if (c->internal_period_cycles != 0)
            c->other = UNSET_BIT(c->other, 2); // reset Last clock
    }
    else
    {
        c->other = SET_BIT(c->other, 0); // Borrow Out
        if (c->internal_period_cycles != 0)
            c->other = SET_BIT(c->other, 2); // Last clock
        c->other = SET_BIT(c->other, 3); // Timer done

        int link = k_mikey_audio_forward_links[i];

        // Propagate link tick to next timer
        if (link >= 0)
        {
            m_state.audio[link].internal_pending_ticks++;
            m_state.audio[link].other = SET_BIT(m_state.audio[link].other, 1);
        }
        else // audio ch 3 links to timer 1
        {
            m_state.timers[1].internal_pending_ticks++;
            m_state.timers[1].control_b = SET_BIT(m_state.timers[1].control_b, 1);
        }

        const bool one_shot = IS_NOT_SET_BIT(c->control, 4);

        if (!one_shot)
            c->counter = c->backup;

        AdvanceLFSR(i);

        // In one-shot, after DONE we must not consume more clocks
        if (one_shot && IS_SET_BIT(c->other, 3))
            return false;
    }

    return true;
}

inline void Mikey::AdvanceLFSR(u8 channel)
{
    GLYNX_Mikey_Audio* c = &m_state.audio[channel];

    s8 vol = (s8)c->volume;
    u16 x = (u16)(c->internal_lfsr & c->internal_taps_mask);
    u8 xorbit = parity16(x);
    u8 data_in = (u8)(xorbit ^ 1u);

    c->internal_lfsr = (u16)(((c->internal_lfsr << 1) & 0x0FFE) | (u16)data_in);

    if (IS_SET_BIT(c->control, 5))
    {
        int acc = (int)c->output;
        int delta = data_in ? (int)vol : -(int)vol;
        acc += delta;
        acc = CLAMP(acc, -128, 127);
        c->output = (s8)acc;
    }
    else
    {
        int v = data_in ? (int)vol : -(int)vol;
        v = CLAMP(v, -128, 127);
        c->output = (s8)v;
    }

    c->lfsr_low = (u8)(c->internal_lfsr & 0x00FF);
    c->other = (u8)((c->other & 0x0F) | ((c->internal_lfsr >> 4) & 0xF0));
}

INLINE void Mikey::RebuildTapsMask(GLYNX_Mikey_Audio* channel)
{
    u8 feedback = channel->feedback;
    u8 control = channel->control;
    u16 mask = (u16)(feedback & 0x3F);
    mask |= ((u16)(feedback & 0xC0)) << 4;
    mask |= (u16)(control & 0x80);
    channel->internal_taps_mask = mask;
}

INLINE void Mikey::RebuildLFSR(GLYNX_Mikey_Audio* channel)
{
    u16 lfsr = (u16)(channel->lfsr_low);
    lfsr |= ((u16)(channel->other & 0xF0)) << 4;
    channel->internal_lfsr = lfsr;
}

inline void Mikey::CalculateCutoff(u8 channel)
{
    GLYNX_Mikey_Audio* c = &m_state.audio[channel];

    if (c->internal_period_cycles != 0)
    {
        u32 cycles = (c->backup + 1) * c->internal_period_cycles;
        c->internal_mix = (cycles >= 32);
    }
    else
    {
        int link = k_mikey_audio_backward_links[channel];
        if (link >= 0)
        {
            u32 cycles = (m_state.audio[link].backup + 1) * m_state.audio[link].internal_period_cycles;
            cycles *= (c->backup + 1);
            c->internal_mix = (cycles >= 32);
        }
        else // channel 0 links to timer 7
        {
            u32 cycles = (m_state.timers[7].backup + 1) * m_state.timers[7].internal_period_cycles;
            cycles *= (c->backup + 1);
            c->internal_mix = (cycles >= 32);
        }
    }
}

INLINE void Mikey::UpdateIRQs()
{
    u8 effective_irqs = m_state.irq_pending & m_state.irq_mask;
    m_m6502->AssertIRQ(effective_irqs != 0, effective_irqs);
}

INLINE void Mikey::UartRelevelIRQ()
{
    bool tx_level = (m_state.uart.tx_int_en && m_state.uart.tx_ready);
    bool rx_level = (m_state.uart.rx_int_en && m_state.uart.rx_ready);

    if (tx_level || rx_level)
        m_state.irq_pending = SET_BIT(m_state.irq_pending, 4);

    UpdateIRQs();
}

inline void Mikey::UartRxReflectHead()
{
    if (m_state.uart.rxq_count > 0)
    {
        u8 h = m_state.uart.rxq_head & 1;
        u8 flags = m_state.uart.rxq_flags[h];
        m_state.uart.rx_data  = m_state.uart.rxq_data[h];
        m_state.uart.par_bit  = IS_SET_BIT(flags, 0);
        m_state.uart.par_err  = IS_SET_BIT(flags, 1);
        m_state.uart.fram_err = IS_SET_BIT(flags, 2);
        m_state.uart.rx_break = IS_SET_BIT(flags, 3);
        m_state.uart.rx_ready = true;
    }
    else
    {
        m_state.uart.rx_ready = false;
        m_state.uart.par_err  = false;
        m_state.uart.fram_err = false;
        m_state.uart.rx_break = false;
        m_state.uart.par_bit  = false;
    }
}

inline void Mikey::UartRxPush(u8 data, bool parbit, bool parerr, bool framerr, bool rxbreak)
{
    if (m_state.uart.rxq_count < 2)
    {
        u8 tail = (m_state.uart.rxq_head + m_state.uart.rxq_count) & 1;
        u8 flags = (parbit ? 0x01 : 0) | (parerr ? 0x02 : 0) | (framerr ? 0x04 : 0) | (rxbreak ? 0x08 : 0);
        m_state.uart.rxq_data[tail] = data;
        m_state.uart.rxq_flags[tail] = flags;
        m_state.uart.rxq_count++;
    }
    else
        m_state.uart.ovr_err = true;

    UartRxReflectHead();
}

inline void Mikey::UartBeginFrame(u8 data)
{
    m_state.uart.tx_data = data;
    m_state.uart.tx_bit_index = 0;

    if (m_state.uart.par_en)
    {
        bool odd = (parity8(data) != 0);
        bool want_even = m_state.uart.par_even;
        m_state.uart.tx_parbit = (want_even ? odd : !odd);
    }
    else
        m_state.uart.tx_parbit = m_state.uart.par_even ? 1 : 0;

    m_state.uart.tx_active = true;
    m_state.uart.tx_empty = false;
    m_state.uart.tx_ready = false;
    m_state.uart.tx_empty_bits = 0;
    m_state.uart.tx_started_from_chain = false;
}

inline void Mikey::UartClock()
{
    m_state.uart.prescaler = (m_state.uart.prescaler + 1) & 7;
    if (m_state.uart.prescaler != 0)
        return;

    // If break is asserted, keep line busy and do not advance a normal frame
    if (m_state.uart.tx_brk)
    {
        m_state.uart.tx_active = true;
        m_state.uart.tx_empty = false;
        return;
    }

    if (!m_state.uart.tx_active)
    {
        if (m_state.uart.tx_empty_bits > 0)
        {
            m_state.uart.tx_empty_bits--;
            if (m_state.uart.tx_empty_bits == 0)
            {
                m_state.uart.tx_empty = true;
                UartRelevelIRQ();
            }
        }
        return; // nothing to shift this tick
    }

    if (!m_state.uart.tx_ready && m_state.uart.tx_ready_bits > 0)
    {
        m_state.uart.tx_ready_bits--;
        if (m_state.uart.tx_ready_bits == 0)
        {
            m_state.uart.tx_ready = true;
            UartRelevelIRQ();
        }
    }

    // Skip synthesizing the TX line level per bit for now

    m_state.uart.tx_bit_index++;

    if (m_state.uart.tx_bit_index >= 11)
    {
        // Frame complete on TX side
        m_state.uart.tx_active = false;

        // Loopback RX: enqueue into 2-deep RX queue
        u8 new_data = m_state.uart.tx_data;
        bool new_parbit = (m_state.uart.tx_parbit != 0);
        bool new_parerr = false;
        bool new_fram = false;   // not synthesized here
        bool new_break = false;

        if (m_state.uart.par_en)
        {
            bool odd = (parity8(new_data) != 0);
            bool expected_parbit = m_state.uart.par_even ? odd : !odd;
            new_parerr = (new_parbit != expected_parbit);
        }
        else
        {
            new_parerr = (new_parbit != m_state.uart.par_even);
        }

        if (m_state.uart.tx_suppress_eof_loopback)
            m_state.uart.tx_suppress_eof_loopback = false;
        else
            UartRxPush(new_data, new_parbit, new_parerr, new_fram, new_break);

        // If there is a holding byte queued, start it now
        if (m_state.uart.tx_hold_valid)
        {
            u8 next = m_state.uart.tx_hold_data;
            m_state.uart.tx_hold_valid = false;
            UartBeginFrame(next);
            m_state.uart.tx_ready = true;
            m_state.uart.tx_ready_bits = 0;
            m_state.uart.tx_empty_bits = 0;
            m_state.uart.tx_started_from_chain = true;
        }
        else
        {
            m_state.uart.tx_ready = true;
            m_state.uart.tx_empty_bits = (m_state.uart.tx_started_from_chain ? 0 : 2);
            m_state.uart.tx_started_from_chain = false;
            m_state.uart.tx_empty = (m_state.uart.tx_empty_bits == 0);
        }

        UartRelevelIRQ();
    }
}

INLINE void Mikey::UpdateVideo(u32 cycles)
{
    m_lcd_screen->Update(cycles);

    m_state.refresh_cycle_counter += cycles;

    while (m_state.refresh_cycle_counter >= k_mikey_refresh_period_cycles)
    {
        m_state.refresh_cycle_counter -= k_mikey_refresh_period_cycles;
        m_bus->InjectCycles(k_mikey_refresh_inject_cycles);
    }
}

#endif /* MIKEY_INLINE_H */
