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
#include "cartridge.h"
#include "m6502.h"

INLINE void Mikey::Clock(u32 cycles)
{
    UpdateTimers(cycles);
    UpdateIRQs();
}

INLINE u8 Mikey::Read(u16 address)
{
    if (address < 0xFD20)
    {
        u8 reg = address & 0x0003;
        u8 timer_index = (address >> 2) & 0x0007;
        return ReadTimer(timer_index, reg);
    }
    else if (address < 0xFD40)
    {
        u8 reg = address & 0x0007;
        u8 channel = (address >> 7) & 0x0003;
        return ReadAudio(channel, reg);
    }
    else if (address < 0xFD80)
    {
        return ReadAudioExtra(address);
    }
    else if (address >= 0xFDA0 && address < 0xFDC0)
    {
        u8 color_index = address & 0x000F;
        if (address < MIKEY_BLUERED0)
            return m_state.colors[color_index].green;
        else
            return m_state.colors[color_index].bluered;
    }
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
            DebugMikey("READ IODATA");
            return m_state.IODAT;
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
    {
        u8 reg = address & 0x0003;
        u8 timer_index = (address >> 2) & 0x0007;
        WriteTimer(timer_index, reg, value);
    }
    else if (address < 0xFD40)
    {
        u8 reg = address & 0x0007;
        u8 channel = (address >> 7) & 0x0003;
        WriteAudio(channel, reg, value);
    }
    else if (address < 0xFD80)
    {
        WriteAudioExtra(address, value);
    }
    else if (address >= 0xFDA0 && address < 0xFDC0)
    {
        u8 color_index = address & 0x000F;
        if (address < MIKEY_BLUERED0)
            m_state.colors[color_index].green = value;
        else
            m_state.colors[color_index].bluered = value;

        m_palette[color_index] = ((m_state.colors[color_index].green & 0x0F) << 8) | (m_state.colors[color_index].bluered & 0xFF);
    }
    else
    {
        switch (address)
        {
        case MIKEY_INTRST:        // 0xFD80
            m_state.irq_pending &= ~value;
            break;
        case MIKEY_INTSET:        // 0xFD81
            m_state.irq_pending |= value;
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
            DebugMikey("Writing SYSCTL1, value: %02X", value);
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
            DebugMikey("Writing IODIR, value: %02X", value);
            m_state.IODIR = value;
            break;
        case MIKEY_IODAT:         // 0xFD8B
            DebugMikey("Writing IODAT, value: %02X", value);
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
            m_state.SDONEACK = value;
            break;
        case MIKEY_CPUSLEEP:      // 0xFD91
            DebugMikey("Writing to CPUSLEEP: %02X", value);
            m_state.CPUSLEEP = value;
            break;
        case MIKEY_DISPCTL:       // 0xFD92
            m_state.DISPCTL = value;
            break;
        case MIKEY_PBKUP:         // 0xFD93
            m_state.PBKUP = value;
            break;
        case MIKEY_DISPADRL:      // 0xFD94
            m_state.DISPADR.low = value;
            break;
        case MIKEY_DISPADRH:      // 0xFD95
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
            DebugMikey("Register WRITE called with unknown address: %04X, value: %02X", address, value);
            break;
        }
    }
}

INLINE Mikey::Mikey_State* Mikey::GetState()
{
    return &m_state;
}

INLINE u16* Mikey::GetPalette()
{
    return m_palette;
}

INLINE u8 Mikey::ReadTimer(u8 timer_index, u8 reg)
{
    assert(timer_index < 8 && reg < 4 && "Invalid timer index or register");

    switch (reg)
    {
    case 0:
        return m_state.timers[timer_index].backup;
    case 1:
        return m_state.timers[timer_index].control_a;
    case 2:
        return m_state.timers[timer_index].counter;
    case 3:
        return m_state.timers[timer_index].control_b;
    default:
        DebugMikey("Timer READ called with unknown register: %02X, index: %02X", reg, timer_index);
        return 0xFF;
    }
}

INLINE void Mikey::WriteTimer(u8 timer_index, u8 reg, u8 value)
{
    assert(timer_index < 8 && reg < 4 && "Invalid timer index or register");

    switch (reg)
    {
    case 0:
        m_state.timers[timer_index].backup = value;
        break;
    case 1:
        m_state.timers[timer_index].control_a = value;
        m_state.timers[timer_index].internal_period_cycles = k_mikey_timer_period_cycles[value & 0x07];
        if (IS_SET_BIT(value, 7))
            m_state.irq_mask = (m_state.irq_mask | (1 << timer_index));
        else
            m_state.irq_mask = (m_state.irq_mask & ~(1 << timer_index));
        if (IS_SET_BIT(value, 6))
            // clear timer done bit
            m_state.timers[timer_index].control_b = UNSET_BIT(m_state.timers[timer_index].control_b, 3); 
        break;
    case 2:
        m_state.timers[timer_index].counter = value;
        break;
    case 3:
        m_state.timers[timer_index].control_b = value;
        break;
    default:
        DebugMikey("Timer WRITE called with unknown register: %02X, index: %02X, value: %02X", reg, timer_index, value);
        break;
    }
}

INLINE u8 Mikey::ReadAudio(u8 channel, u8 reg)
{
    assert(channel < 4 && reg < 8 && "Invalid audio channel or register");

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
        DebugMikey("Audio READ called with unknown register: %02X, channel: %02X", reg, channel);
        return 0xFF;
    }
}

INLINE void Mikey::WriteAudio(u8 channel, u8 reg, u8 value)
{
    assert(channel < 4 && reg < 8 && "Invalid audio channel or register");

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
        DebugMikey("Audio WRITE called with unknown register: %02X, channel: %02X, value: %02X", reg, channel, value);
        break;
    }
}

INLINE u8 Mikey::ReadAudioExtra(u16 address)
{
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

        // independent mode
        if (t->internal_period_cycles != 0)
        {
            t->internal_cycles += cycles;

            bool borrow_in = false;

            while (t->internal_cycles >= t->internal_period_cycles)
            {
                t->internal_cycles -= t->internal_period_cycles;
                DecrementTimerCounter(i);
                borrow_in = true;
            }

            // set borrow in flag
            t->control_b = borrow_in ? SET_BIT(t->control_b, 1) : UNSET_BIT(t->control_b, 1);
        }
        // linked mode and borrow out from linked timer
        else if (IS_SET_BIT(m_state.timers[t->internal_linked_to].control_b, 0))
        {
            DecrementTimerCounter(i);
            t->control_b = SET_BIT(t->control_b, 1);
        }
        else
        {
            // clear borrow in flag
            t->control_b = UNSET_BIT(t->control_b, 1);
            // clear borrow out flag
            t->control_b = UNSET_BIT(t->control_b, 0);
        }
    }
}

INLINE void Mikey::DecrementTimerCounter(u8 timer_index)
{
    GLYNX_Mikey_Timer* t = &m_state.timers[timer_index];

    if (t->counter == 0x00)
    {
        // if reload on underflow is set
        if (IS_SET_BIT(t->control_a, 4))
            t->counter = t->backup;

        // set irq status even if irq is disabled?
        if (IS_SET_BIT(t->control_a, 7))
            m_state.irq_pending = SET_BIT(m_state.irq_pending, timer_index);

        // set timer done bit
        t->control_b = SET_BIT(t->control_b, 3);

        // set borrow out flag
        t->control_b = SET_BIT(t->control_b, 0);

        if (timer_index == 0)
        {
            Debug("Render line");
        }
        else if (timer_index == 2)
        {
            Debug("VBLANK frame done");
        }
    }
    else
    {
        // clear borrow out flag
        t->control_b = UNSET_BIT(t->control_b, 0);
        t->counter--;
    }

}

INLINE void Mikey::UpdateIRQs()
{
    m_m6502->AssertIRQ((m_state.irq_pending && m_state.irq_mask) != 0);
}

#endif /* MIKEY_INLINE_H */
