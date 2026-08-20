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

#include <assert.h>
#include "mikey.h"
#include "audio.h"
#include "suzy.h"
#include "media.h"
#include "m6502.h"
#include "bit_ops.h"
#include "bus.h"
#include "lcd_screen.h"
#include "trace_logger.h"
#include "eeprom.h"
#include "el_cheapo_sd.h"
#include "memory.h"

INLINE bool Mikey::Clock(u32 cycles)
{
    if (cycles > m_cpu_read_cycles)
        Advance(cycles - m_cpu_read_cycles);

    m_cpu_read_cycles = 0;

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
        return ReadTimer<debug>(address);
    else if (address < 0xFD40)
        return ReadAudio<debug>(address);

    if (!debug)
        SynchronizeCPURead();

    if (address <= 0xFD50)
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
        {
            u8 ret = 0x80;
            DebugMikey("Reading AUDIN: %02X", ret);
            return ret;
        }
        case MIKEY_SYSCTL1:       // 0xFD87
            DebugMikey("Reading write-only SYSCTL1: 80");
            return 0x80;
        case MIKEY_MIKEYHREV:     // 0xFD88
            return m_is_lynx2 ? 0x02 : 0x01;
        case MIKEY_MIKEYSREV:     // 0xFD89
            DebugMikey("Reading write-only MIKEYSREV: FF");
            return 0xFF;
        case MIKEY_IODIR:         // 0xFD8A
            DebugMikey("Reading write-only IODIR: FF");
            return 0xFF;
        case MIKEY_IODAT:         // 0xFD8B
        {
            u8 ret = 0x00;

            // Bit 0: External power input
            if (IS_SET_BIT(m_state.IODIR, 0))
                ret |= IS_SET_BIT(m_state.IODAT, 0) ? 0x01 : 0x00;
            else
                ret |= 0x01;  // Input defaults to high (power connected)

            // Bit 1: Cart address data output (0 turns cart power on)
            if (IS_SET_BIT(m_state.IODIR, 1))
                ret |= IS_SET_BIT(m_state.IODAT, 1) ? 0x02 : 0x00;
            // else input reads low

            // Bit 2: No expansion (input high when ComLynx cable not present)
            if (IS_SET_BIT(m_state.IODIR, 2))
                ret |= IS_SET_BIT(m_state.IODAT, 2) ? 0x04 : 0x00;
            else
                ret |= m_comlynx_cable_connected ? 0x00 : 0x04;

            // Bit 3: Rest signal (output with REST signal gating)
            if (IS_SET_BIT(m_state.IODIR, 3))
                ret |= (IS_SET_BIT(m_state.IODAT, 3) && m_state.rest) ? 0x08 : 0x00;
            // else input reads low

            // Bit 4: AUDIN - Audio input / EEPROM data / Cart AUDIN
            // When configured as input, defaults to high (EEPROM write done / cart ready)
            // EEPROM can override this when actively sending data
            if (IS_SET_BIT(m_state.IODIR, 4))
                ret |= IS_SET_BIT(m_state.IODAT, 4) ? 0x10 : 0x00;
            else if (m_media->GetGameDriveInstance()->IsAvailable() &&
                !m_media->GetEEPROMInstance()->IsSelected())
                ret |= m_media->GetGameDriveInstance()->HasOutput() ? 0x10 : 0x00;
            else if (m_media->GetElCheapoSDInstance()->IsSelected())
                ret |= m_media->GetElCheapoSDInstance()->OutputBit() ? 0x10 : 0x00;
            else if (m_media->GetEEPROMInstance()->IsAvailable())
            {
                if (!debug)
                    m_media->GetEEPROMInstance()->ProcessBusy();
                ret |= m_media->GetEEPROMInstance()->OutputBit() ? 0x10 : 0x00;
            }
            else
                ret |= 0x10;  // Input defaults to high (ready/done signal)

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
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
                if (m_state.uart.rx_ready)
                    TraceUARTEvent(TRACE_MIKEY_UART_DATA_READ, ret,
                        m_state.uart.rxq_flags[m_state.uart.rxq_head & 1]);
#endif

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
        case 0xFD98:              // Undocumented
            DebugMikey("Reading undocumented register 0xFD98: FD");
            return 0xFD;
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
        WriteTimer<debug>(address, value);
    else if (address < 0xFD40)
        WriteAudio<debug>(address, value);
    else if (address <= 0xFD50)
        WriteAudioExtra(address, value, debug);
    else if (address >= 0xFDA0 && address < 0xFDC0)
        WriteColor(address, value, debug);
    else
    {
        switch (address)
        {
        case MIKEY_INTRST:        // 0xFD80
        {
            DebugMikey("Clearing IRQs: %02X (was %02X)", value, m_state.irq_pending);
            m_state.irq_pending &= ~value;
            UartRelevelIRQ();
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceInterruptEvent(TRACE_MIKEY_INTERRUPT_REGISTER, 0, value);
#endif
            break;
        }
        case MIKEY_INTSET:        // 0xFD81
            DebugMikey("Setting IRQs: %02X (was %02X)", value, m_state.irq_pending);
            m_state.irq_pending |= value;
            UpdateIRQs();
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceInterruptEvent(TRACE_MIKEY_INTERRUPT_REGISTER, 1, value);
#endif
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
        {
            DebugMikey("Setting SYSCTL1 to %02X (was %02X)", value, m_state.SYSCTL1);
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            m_media->ShiftRegisterStrobe(IS_SET_BIT(value, 0), !debug);
#else
            m_media->ShiftRegisterStrobe(IS_SET_BIT(value, 0));
#endif
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug && IS_SET_BIT(value, 0) && !IS_SET_BIT(m_state.SYSCTL1, 0))
                TraceCartridgeAddressEvent();
#endif
            m_state.SYSCTL1 = value;
            break;
        }
        case MIKEY_MIKEYHREV:     // 0xFD88
            DebugMikey("Writing to read-only MIKEYHREV: %02X", value);
            break;
        case MIKEY_MIKEYSREV:     // 0xFD89
            DebugMikey("Writing MIKEYSREV (unused): %02X", value);
            break;
        case MIKEY_IODIR:         // 0xFD8A
            DebugMikey("Setting IODIR to %02X (was %02X)", value, m_state.IODIR);
            m_state.IODIR = value;

            if (IS_SET_BIT(m_state.IODIR, 4))
                m_media->SetAudinValue(IS_SET_BIT(m_state.IODAT, 4));
            else
                m_media->SetAudinValue(false);

            if (m_media->GetEEPROMInstance()->IsAvailable())
                m_media->GetEEPROMInstance()->ProcessIO(m_state.IODIR, m_state.IODAT);
            else if (m_media->GetElCheapoSDInstance()->IsAvailable())
                m_media->GetElCheapoSDInstance()->ProcessIO(m_state.IODIR, m_state.IODAT);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceCartridgeIOEvent(TRACE_CARTRIDGE_AUDIN, 0, value);
#endif

            break;
        case MIKEY_IODAT:         // 0xFD8B
            DebugMikey("Setting IODAT to %02X (was %02X)", value, m_state.IODAT);
            m_media->ShiftRegisterBit(IS_SET_BIT(value, 1));
            m_state.IODAT = value;

            if (IS_SET_BIT(m_state.IODIR, 4))
                m_media->SetAudinValue(IS_SET_BIT(m_state.IODAT, 4));

            if (m_media->GetEEPROMInstance()->IsAvailable())
                m_media->GetEEPROMInstance()->ProcessIO(m_state.IODIR, m_state.IODAT);
            else if (m_media->GetElCheapoSDInstance()->IsAvailable())
                m_media->GetElCheapoSDInstance()->ProcessIO(m_state.IODIR, m_state.IODAT);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceCartridgeIOEvent(TRACE_CARTRIDGE_AUDIN, 1, value);
#endif

            break;
        case MIKEY_SERCTL:        // 0xFD8C
        {
            DebugMikey("Setting SERCTL to %02X (was %02X)", value, m_state.SERCTL);
            bool was_tx_brk = m_state.uart.tx_brk;
            bool was_break_asserted = m_state.uart.tx_open && m_state.uart.tx_brk;

            m_state.SERCTL = value;

            m_state.uart.tx_int_en = IS_SET_BIT(value, 7);
            m_state.uart.rx_int_en = IS_SET_BIT(value, 6);
            m_state.uart.par_en = IS_SET_BIT(value, 4);
            m_state.uart.tx_open = IS_SET_BIT(value, 2);
            m_state.uart.tx_brk = IS_SET_BIT(value, 1);
            m_state.uart.par_even = IS_SET_BIT(value, 0);

            bool break_asserted = m_state.uart.tx_open && m_state.uart.tx_brk;

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug && !was_break_asserted && break_asserted)
                TraceRedEyeProblemEvent(0, TRACE_REDEYE_PROBLEM_BREAK, 0);
#endif

            if (was_break_asserted != break_asserted && m_comlynx_cable_connected &&
                m_comlynx_break_callback)
            {
                m_comlynx_break_callback(break_asserted, m_comlynx_cycle,
                    m_comlynx_user_data);
            }

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
            }
            else
            {
                if (was_tx_brk && !m_state.uart.tx_active && m_state.uart.tx_hold_valid)
                    m_state.uart.tx_ready_bits = 3;
                else if (!m_state.uart.tx_active && !m_state.uart.tx_hold_valid)
                {
                    m_state.uart.tx_empty = true;
                    m_state.uart.tx_ready = true;
                }
            }

            if (m_state.uart.tx_int_en || m_state.uart.rx_int_en)
                m_state.irq_mask = SET_BIT(m_state.irq_mask, 4);
            else
                m_state.irq_mask = UNSET_BIT(m_state.irq_mask, 4);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceUARTConfigEvent(value, true);
            if (!debug && was_break_asserted != break_asserted)
                TraceUARTEvent(TRACE_MIKEY_UART_BREAK, break_asserted ? 1 : 0);
#endif

            UartRelevelIRQ();
            break;
        }
        case MIKEY_SERDAT:        // 0xFD8D
        {
            DebugMikey("Setting SERDAT (TX) to %02X", value);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceUARTEvent(TRACE_MIKEY_UART_REGISTER, value, 0, MIKEY_SERDAT & 0xFF);
#endif

            if (!m_state.uart.tx_active && !m_state.uart.tx_brk)
            {
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
                UartBeginFrame(value, false, !debug);
#else
                UartBeginFrame(value, false);
#endif
                m_state.uart.tx_start_bits = GLYNX_UART_TX_START_BITS;
                m_state.uart.tx_ready_bits = 2;
                m_state.uart.tx_started_from_chain = false;
            }
            else
            {
                m_state.uart.tx_hold_data = value;
                m_state.uart.tx_hold_valid = true;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
                m_uart_tx_hold_trace = !debug;
#endif
                m_state.uart.tx_ready = false;
                m_state.uart.tx_empty = false;

                if (m_state.uart.tx_brk && m_state.uart.tx_active)
                    m_state.uart.tx_suppress_eof_loopback = true;
            }

            UartRelevelIRQ();
            break;
        }
        case MIKEY_SDONEACK:      // 0xFD90
            DebugMikey("Setting SDONEACK to %02X (was %02X)", value, m_state.SDONEACK);
            m_state.SDONEACK = value;
            m_state.suzy_done_pending = false;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceDisplayEvent(TRACE_MIKEY_DISPLAY_REGISTER, 0x90, value);
#endif
            break;
        case MIKEY_CPUSLEEP:      // 0xFD91
            DebugMikey("Setting CPUSLEEP to %02X (was %02X)", value, m_state.CPUSLEEP);
            if ((value == 0) && m_suzy->IsBlitterBusy() && m_suzy->IsBusEnabled() &&
                !m_state.suzy_done_pending && ((m_state.irq_pending & m_state.irq_mask) == 0))
                m_m6502->Halt(true);
            m_state.CPUSLEEP = value;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceDisplayEvent(TRACE_MIKEY_DISPLAY_REGISTER, 0x91, value);
#endif
            break;
        case MIKEY_DISPCTL:       // 0xFD92
            DebugMikey("Setting DISPCTL to %02X (was %02X)", value, m_state.DISPCTL);
            m_state.DISPCTL = value;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceDisplayEvent(TRACE_MIKEY_DISPLAY_REGISTER, 0x92, value);
#endif
            break;
        case MIKEY_PBKUP:         // 0xFD93
            DebugMikey("Setting PBKUP to %02X (was %02X)", value, m_state.PBKUP);
            m_state.PBKUP = value;
            m_lcd_screen->ConfigureLineTiming();
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceDisplayEvent(TRACE_MIKEY_DISPLAY_REGISTER, 0x93, value);
#endif
            break;
        case MIKEY_DISPADRL:      // 0xFD94
            DebugMikey("Setting DISPADR low to %02X (was %02X)", value, m_state.DISPADR.low);
            m_state.DISPADR.low = value;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceDisplayEvent(TRACE_MIKEY_DISPLAY_REGISTER, 0x94, value);
#endif
            break;
        case MIKEY_DISPADRH:      // 0xFD95
            DebugMikey("Setting DISPADR high to %02X (was %02X)", value, m_state.DISPADR.high);
            m_state.DISPADR.high = value;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceDisplayEvent(TRACE_MIKEY_DISPLAY_REGISTER, 0x95, value);
#endif
            break;
        case MIKEY_MTEST0:        // 0xFD9C
            DebugMikey("Setting MTEST0 to %02X (was %02X)", value, m_state.MTEST0);
            m_state.MTEST0 = value;
            m_uart_last_bit_cycle = 0;
            RebuildTimerSourceDistances();
            m_timer_source_countdown = CalculateNextTimerSourceCycles(m_state.timer_source_phase);
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceUARTConfigEvent(m_state.SERCTL);
#endif
            break;
        case MIKEY_MTEST1:        // 0xFD9D
            DebugMikey("Writing MTEST1 (unused): %02X", value);
            break;
        case MIKEY_MTEST2:        // 0xFD9E
            DebugMikey("Writing MTEST2 (unused): %02X", value);
            break;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        case MIKEY_DBGASCII:      // 0xFDC1
        case MIKEY_DBGHEX:        // 0xFDC2
        case MIKEY_DBGSTRL:       // 0xFDC3
        case MIKEY_DBGSTRH:       // 0xFDC4
        case MIKEY_DBGOUT:        // 0xFDC0
            if (m_debug_output_enabled)
                TraceDebugMessageEvent(address, value);
            break;
#endif
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

INLINE bool Mikey::IsPoweredOn()
{
    return IS_SET_BIT(m_state.SYSCTL1, 1);
}

INLINE LcdScreen* Mikey::GetLcdScreen()
{
    return m_lcd_screen;
}

INLINE bool Mikey::IsUartTurbo() const
{
    return IS_SET_BIT(m_state.MTEST0, 4);
}

INLINE u32 Mikey::GetUartBitCycles() const
{
    if (IsUartTurbo())
        return GLYNX_UART_TURBO_BIT_CYCLES;

    u32 timer_cycles = m_state.timers[4].internal_period_cycles;

    if (timer_cycles == 0)
        timer_cycles = 16;

    return timer_cycles * ((u32)m_state.timers[4].backup + 1) * 8;
}

INLINE u32 Mikey::GetComLynxSyncCycles() const
{
    if (IsUartTurbo())
        return COMLYNX_TURBO_SYNC_CYCLES;

    return MIN(COMLYNX_MAX_SYNC_CYCLES, GetUartBitCycles() >> 1);
}

INLINE u32 Mikey::GetComLynxPromiseCycles() const
{
    if (IsUartTurbo())
        return COMLYNX_TURBO_PROMISE_CYCLES;

    return MIN(COMLYNX_MAX_PROMISE_CYCLES, GetUartBitCycles());
}

inline u8 Mikey::ReadColor(u16 address)
{
    assert(address >= MIKEY_GREEN0 && address <= MIKEY_BLUEREDF);

    int color_index = address & 0xF;

    if (address < MIKEY_BLUERED0)
        return m_state.colors[color_index].green & 0x0F;
    else
        return m_state.colors[color_index].bluered;
}

inline void Mikey::WriteColor(u16 address, u8 value, bool debug)
{
    assert(address >= MIKEY_GREEN0 && address <= MIKEY_BLUEREDF);

    int color_index = address & 0xF;

    if (address < MIKEY_BLUERED0)
        m_state.colors[color_index].green = value;
    else
        m_state.colors[color_index].bluered = value;

    u16 rgb444 = (u16)(((m_state.colors[color_index].green & 0x0F) << 8) | (m_state.colors[color_index].bluered & 0xFF));

    m_lcd_screen->UpdatePalette(color_index, rgb444);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (!debug)
        TracePaletteEvent((u8)color_index, value, rgb444);
#endif
}

INLINE u32 Mikey::GetTimerAccessCycles(int timer)
{
    u32 elapsed = m_m6502->GetInstructionTicks() + m_bus->GetCycles();
    u32 phase = (m_state.timer_source_phase + elapsed) & 0x0F;
    u32 slot = (u32)timer;
    return (slot - phase) & 0x0F;
}

template<bool debug>
inline u8 Mikey::ReadTimer(u16 address)
{
    assert(address >= MIKEY_TIM0BKUP && address <= MIKEY_TIM7CTLB);

    int reg = address & 3;
    int i = (address >> 2) & 7;

    if (!debug)
    {
        m_bus->InjectCycles(GetTimerAccessCycles(i));
        SynchronizeCPURead();
    }

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

template<bool debug>
inline void Mikey::WriteTimer(u16 address, u8 value)
{
    assert(address >= MIKEY_TIM0BKUP && address <= MIKEY_TIM7CTLB);

    int reg = address & 3;
    int i = (address >> 2) & 7;

    if (!debug)
    {
        m_bus->InjectCycles(GetTimerAccessCycles(i));
        SynchronizeCPURead();
    }

    GLYNX_Mikey_Timer* t = &m_state.timers[i];

#ifndef GLYNX_DISABLE_VGMRECORDER
    if (!debug && (i & 1) && m_audio->IsVgmRecording())
        m_audio->GetVgmRecorder()->WriteMikey(address, value);
#endif

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

        if (IS_SET_BIT(old_control_a, 3) && old_prescaler < 7)
        {
            m_timer_source_masks[old_prescaler] = UNSET_BIT(m_timer_source_masks[old_prescaler], i);
            if (m_timer_source_masks[old_prescaler] == 0)
                m_timer_active_source_mask = UNSET_BIT(m_timer_active_source_mask, old_prescaler);
        }

        t->control_a = value;

        t->internal_period_cycles = k_mikey_timer_period_cycles[new_prescaler];

        if (IS_SET_BIT(value, 3) && new_prescaler < 7)
        {
            m_timer_source_masks[new_prescaler] = SET_BIT(m_timer_source_masks[new_prescaler], i);
            m_timer_active_source_mask = SET_BIT(m_timer_active_source_mask, new_prescaler);
        }

        // Re-sync only when clock source changes or when enabling counting from disabled
        bool prescaler_changed = (old_prescaler != new_prescaler);
        bool enable_count_rising = IS_NOT_SET_BIT(old_control_a, 3) && IS_SET_BIT(value, 3);

        if (prescaler_changed || enable_count_rising)
        {
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
            t->control_b = UNSET_BIT(t->control_b, 3);

        UpdateTimerServiceMask(i);
        RebuildTimerSourceDistances();
        m_timer_source_countdown = CalculateNextTimerSourceCycles(m_state.timer_source_phase);

        break;
    }
    case 2:
        DebugMikey("Setting Timer %d Counter to %02X (was %02X)", i, value, m_state.timers[i].counter);
        t->counter = value;
        break;
    case 3:
        DebugMikey("Setting Timer %d Control B to %02X (was %02X)", i, value, m_state.timers[i].control_b);
        if (IS_NOT_SET_BIT(t->control_b, 1) && IS_SET_BIT(value, 1))
            BorrowInTimer(i, t);
        t->control_b = value & 0x08;
        UpdateTimerStatusMask(i);
        UpdateTimerServiceMask(i);
        break;
    default:
        break;
    }

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (!debug)
        TraceTimerEvent(TRACE_MIKEY_TIMER_REGISTER, i, (u8)reg, value);
    if (!debug && i == 4)
        TraceUARTConfigEvent(m_state.SERCTL);
#endif
}

template<bool debug>
inline u8 Mikey::ReadAudio(u16 address)
{
    assert(address >= MIKEY_AUD0VOL && address <= MIKEY_AUD3MISC);

    int reg = address & 7;
    int i = ((address - MIKEY_AUD0VOL) >> 3) & 3;

    if (!debug)
    {
        m_bus->InjectCycles(GetTimerAccessCycles(i + 8));
    }

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

template<bool debug>
inline void Mikey::WriteAudio(u16 address, u8 value)
{
    assert(address >= MIKEY_AUD0VOL && address <= MIKEY_AUD3MISC);

    int reg = address & 7;
    int i = ((address - MIKEY_AUD0VOL) >> 3) & 3;

    if (!debug)
    {
        m_bus->InjectCycles(GetTimerAccessCycles(i + 8));
        SynchronizeCPURead();
    }

#ifndef GLYNX_DISABLE_VGMRECORDER
    if (!debug && m_audio->IsVgmRecording())
        m_audio->GetVgmRecorder()->WriteMikey(address, value);
#endif

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

        if (IS_SET_BIT(old_control, 3) && old_prescaler < 7)
        {
            m_timer_source_masks[old_prescaler] = UNSET_BIT(m_timer_source_masks[old_prescaler], i + 8);
            if (m_timer_source_masks[old_prescaler] == 0)
                m_timer_active_source_mask = UNSET_BIT(m_timer_active_source_mask, old_prescaler);
        }

        c->control = value;
        c->internal_period_cycles = k_mikey_timer_period_cycles[new_prescaler];

        if (IS_SET_BIT(value, 3) && new_prescaler < 7)
        {
            m_timer_source_masks[new_prescaler] = SET_BIT(m_timer_source_masks[new_prescaler], i + 8);
            m_timer_active_source_mask = SET_BIT(m_timer_active_source_mask, new_prescaler);
        }

        bool prescaler_changed = (old_prescaler != new_prescaler);
        bool enable_count_rising = IS_NOT_SET_BIT(old_control, 3) && IS_SET_BIT(value, 3);

        if (prescaler_changed || enable_count_rising)
        {
            c->internal_pending_ticks = 0;
            CalculateCutoff(i);
        }

        if (IS_SET_BIT(value, 6))
            c->other = UNSET_BIT(c->other, 3);

        if (IS_NOT_SET_BIT(c->control, 3))
            c->internal_mix = true;

        RebuildTapsMask(c);
        UpdateTimerServiceMask(i + 8);
        RebuildTimerSourceDistances();
        m_timer_source_countdown = CalculateNextTimerSourceCycles(m_state.timer_source_phase);
        break;
    }
    case 6:
        c->counter = value;
        break;
    case 7:
        if (IS_NOT_SET_BIT(c->other, 1) && IS_SET_BIT(value, 1))
        {
            BorrowInChannel(i, c);
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (!debug)
                TraceAudioEvent(TRACE_MIKEY_AUDIO_CLOCK, i, 0, 0);
#endif
        }
        c->other = value & 0xF8;
        RebuildLFSR(c);
        UpdateTimerStatusMask(i + 8);
        UpdateTimerServiceMask(i + 8);
        break;
    default:
        break;
    }

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (!debug)
        TraceAudioEvent(TRACE_MIKEY_AUDIO_CHANNEL, i, (u8)reg, value);
#endif
}

inline u8 Mikey::ReadAudioExtra(u16 address)
{
    assert(address >= MIKEY_ATTEN_A && address <= MIKEY_MSTEREO);

    if (!m_is_lynx2)
        return 0xFD;

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

inline void Mikey::WriteAudioExtra(u16 address, u8 value, bool debug)
{
    assert(address >= MIKEY_ATTEN_A && address <= MIKEY_MSTEREO);

    if (!m_is_lynx2)
        return;

#ifndef GLYNX_DISABLE_VGMRECORDER
    if (!debug && m_audio->IsVgmRecording())
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

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (!debug)
        TraceAudioEvent(TRACE_MIKEY_AUDIO_MIXER, 0xFF, (u8)(address & 0xFF), value);
#endif
}

INLINE void Mikey::Advance(u32 cycles)
{
    UpdateVideo(cycles);
    UpdateUART(cycles);
    UpdateTimerHardware(cycles);
    UpdateIRQs();
}

INLINE void Mikey::UpdateUART(u32 cycles)
{
    TraceRedEyeTimeoutEvent();

    if (m_state.uart.rx_age_cycles < GLYNX_UART_RX_AGE_MAX_CYCLES)
        m_state.uart.rx_age_cycles += cycles;

    if (m_state.uart.tx_empty_cycles > 0)
    {
        if (cycles >= m_state.uart.tx_empty_cycles)
        {
            m_state.uart.tx_empty_cycles = 0;
            m_state.uart.tx_empty = true;
        }
        else
            m_state.uart.tx_empty_cycles -= cycles;
    }
}

INLINE void Mikey::SynchronizeCPURead()
{
    u32 cycles = m_m6502->GetInstructionTicks() + m_bus->GetCycles();

    while (cycles > m_cpu_read_cycles)
    {
        Advance(cycles - m_cpu_read_cycles);
        m_cpu_read_cycles = cycles;
        cycles = m_m6502->GetInstructionTicks() + m_bus->GetCycles();
    }
}

INLINE void Mikey::UpdateTimerHardware(u32 cycles)
{
    m_video_line_remainder = 0;

    while (cycles > 0)
    {
        bool turbo_sync = unlikely(m_comlynx_cable_connected &&
            m_comlynx_turbo_sync_callback && IsUartTurbo());

        if (turbo_sync && (m_comlynx_cycle & (COMLYNX_TURBO_SYNC_CYCLES - 1)) == 0)
        {
            m_comlynx_turbo_sync_callback(m_comlynx_cycle, m_comlynx_turbo_user_data);
            turbo_sync = unlikely(m_comlynx_cable_connected &&
                m_comlynx_turbo_sync_callback && IsUartTurbo());
        }

        u32 phase = m_state.timer_source_phase;
        u32 distance = cycles + 1;
        u32 source_cycles = m_timer_source_countdown;
        u32 service_cycles = GetNextTimerServiceCycles(phase);

        if (source_cycles > 0 && source_cycles < distance)
            distance = source_cycles;
        if (service_cycles < distance)
            distance = service_cycles;

        if (turbo_sync)
        {
            u32 sync_cycles = (u32)((0 - m_comlynx_cycle) & (COMLYNX_TURBO_SYNC_CYCLES - 1));
            if (sync_cycles == 0)
                sync_cycles = COMLYNX_TURBO_SYNC_CYCLES;
            if (sync_cycles < distance)
                distance = sync_cycles;
        }

        if (distance > cycles)
        {
            ExpireTimerStatus(phase, cycles);
            if (m_timer_source_countdown > 0)
                m_timer_source_countdown -= cycles;
            m_comlynx_cycle += cycles;
            m_state.timer_source_phase = (phase + cycles) & 1023;
            break;
        }

        bool source_event = source_cycles > 0 && source_cycles == distance;

        if (distance > 1)
            ExpireTimerStatus(phase, distance - 1);

        if (m_timer_source_countdown > 0)
            m_timer_source_countdown -= distance;

        m_comlynx_cycle += distance;
        phase = (phase + distance) & 1023;
        m_state.timer_source_phase = phase;
        cycles -= distance;

        u32 slot = phase & 0x0F;

        if (source_event && slot == 1)
        {
            if (IS_SET_BIT(m_state.MTEST0, 4))
                UartClock<true>();

            if (m_timer_source_masks[0] != 0)
                ClockTimerDomain(0, cycles);
            if (m_timer_source_masks[1] != 0 && (phase & 31) == 17)
                ClockTimerDomain(1, cycles);
            if (m_timer_source_masks[6] != 0 && phase == 897)
                ClockTimerDomain(6, cycles);
        }
        else if (source_event && slot == 8)
        {
            if (m_timer_source_masks[2] != 0 && (phase & 63) == 56)
                ClockTimerDomain(2, cycles);
            if (m_timer_source_masks[3] != 0 && (phase & 127) == 120)
                ClockTimerDomain(3, cycles);
            if (m_timer_source_masks[4] != 0 && (phase & 255) == 120)
                ClockTimerDomain(4, cycles);
            if (m_timer_source_masks[5] != 0 && (phase & 511) == 376)
                ClockTimerDomain(5, cycles);
        }

        if (source_event)
            m_timer_source_countdown = CalculateNextTimerSourceCycles(phase);

        ExpireTimerStatusSlot(slot);

        if (slot < 12 && IS_SET_BIT(m_timer_service_mask, slot))
        {
            m_timer_service_mask = UNSET_BIT(m_timer_service_mask, slot);

            if (slot < 8)
                ServiceTimer(slot);
            else
                ServiceAudio(slot - 8);

            UpdateTimerStatusMask(slot);
            UpdateTimerServiceMask(slot);
        }
    }

    m_video_line_remainder = 0;
}

INLINE u32 Mikey::CalculateNextTimerSourceCycles(u32 phase)
{
#if !defined(NDEBUG)
    u8 key = m_timer_active_source_mask;
    if (IS_SET_BIT(m_state.MTEST0, 4))
        key = SET_BIT(key, 0);

    assert(phase < 1024);
    assert(key == m_timer_source_key);
    assert(m_timer_source_distance[phase] == CalculateNextTimerSourceCyclesSlow(phase, key));
#endif

    return m_timer_source_distance[phase];
}

INLINE u32 Mikey::GetNextTimerServiceCycles(u32 phase)
{
    if (m_timer_service_mask == 0)
        return 0xFFFFFFFF;

    u32 slot = phase & 0x0F;
    u32 shift = slot + 1;
    u16 rotated = (u16)((m_timer_service_mask >> shift) |
        ((u32)m_timer_service_mask << (16 - shift)));
    return t_zero16(rotated) + 1;
}

INLINE void Mikey::ExpireTimerStatus(u32 phase, u32 cycles)
{
    if (m_timer_status_mask == 0 || cycles == 0)
        return;

    u16 slot_mask;

    if (cycles >= 16)
        slot_mask = 0x0FFF;
    else
    {
        u32 start = (phase + 1) & 0x0F;
        u32 range = ((1U << cycles) - 1) << start;
        slot_mask = (u16)((range | (range >> 16)) & 0x0FFF);
    }

    u16 expired = m_timer_status_mask & slot_mask;
    m_timer_status_mask &= ~expired;

    while (expired != 0)
    {
        int unit = (int)t_zero16(expired);
        expired &= expired - 1;

        if (unit < 8)
            m_state.timers[unit].control_b &= 0xFC;
        else
            m_state.audio[unit - 8].other &= 0xFC;
    }
}

INLINE void Mikey::ExpireTimerStatusSlot(int slot)
{
    if (slot >= 12 || IS_NOT_SET_BIT(m_timer_status_mask, slot))
        return;

    m_timer_status_mask = UNSET_BIT(m_timer_status_mask, slot);

    if (slot < 8)
        m_state.timers[slot].control_b &= 0xFC;
    else
        m_state.audio[slot - 8].other &= 0xFC;
}

INLINE void Mikey::ClockTimerDomain(int prescaler, u32 remaining_cycles)
{
    u16 mask = m_timer_source_masks[prescaler];

    while (mask != 0)
    {
        int unit = (int)t_zero16(mask);
        mask &= mask - 1;

        if (unit < 8)
        {
            GLYNX_Mikey_Timer* t = &m_state.timers[unit];
            if (unit == 0 && t->counter == 0)
                m_video_line_remainder = remaining_cycles;

            ClockTimer(unit);
            UpdateTimerStatusMask(unit);
        }
        else
        {
            ClockAudio(unit - 8);
            UpdateTimerStatusMask(unit);
        }
    }
}

INLINE void Mikey::RebuildTimerCaches()
{
    memset(m_timer_source_masks, 0, sizeof(m_timer_source_masks));
    m_timer_service_mask = 0;
    m_timer_status_mask = 0;
    m_timer_active_source_mask = 0;
    m_timer_source_countdown = 0;

    for (int timer = 0; timer < 8; timer++)
    {
        u8 control = m_state.timers[timer].control_a;
        int prescaler = control & 7;
        if (IS_SET_BIT(control, 3) && prescaler < 7)
        {
            m_timer_source_masks[prescaler] = SET_BIT(m_timer_source_masks[prescaler], timer);
            m_timer_active_source_mask = SET_BIT(m_timer_active_source_mask, prescaler);
        }

        UpdateTimerStatusMask(timer);
        UpdateTimerServiceMask(timer);
    }

    for (int channel = 0; channel < 4; channel++)
    {
        u8 control = m_state.audio[channel].control;
        int prescaler = control & 7;
        if (IS_SET_BIT(control, 3) && prescaler < 7)
        {
            m_timer_source_masks[prescaler] = SET_BIT(m_timer_source_masks[prescaler], channel + 8);
            m_timer_active_source_mask = SET_BIT(m_timer_active_source_mask, prescaler);
        }

        UpdateTimerStatusMask(channel + 8);
        UpdateTimerServiceMask(channel + 8);
    }

    RebuildTimerSourceDistances();
    m_timer_source_countdown = CalculateNextTimerSourceCycles(m_state.timer_source_phase);
}

INLINE void Mikey::UpdateTimerStatusMask(int unit)
{
    bool active;

    if (unit < 8)
        active = (m_state.timers[unit].control_b & 0x03) != 0;
    else
        active = (m_state.audio[unit - 8].other & 0x03) != 0;

    if (active)
        m_timer_status_mask = SET_BIT(m_timer_status_mask, unit);
    else
        m_timer_status_mask = UNSET_BIT(m_timer_status_mask, unit);
}

INLINE void Mikey::UpdateTimerServiceMask(int unit)
{
    bool active = false;

    if (unit < 8)
    {
        GLYNX_Mikey_Timer* t = &m_state.timers[unit];
        bool linked = IS_SET_BIT(t->control_a, 3) && t->internal_period_cycles == 0;
        bool reset_done = IS_SET_BIT(t->control_a, 6) && IS_SET_BIT(t->control_b, 3);
        bool blocked = IS_NOT_SET_BIT(t->control_a, 4) && IS_SET_BIT(t->control_b, 3) && !reset_done;
        active = linked && (reset_done || (t->internal_pending_ticks > 0 && !blocked));
    }
    else
    {
        GLYNX_Mikey_Audio* c = &m_state.audio[unit - 8];
        bool linked = IS_SET_BIT(c->control, 3) && c->internal_period_cycles == 0;
        bool reset_done = IS_SET_BIT(c->control, 6) && IS_SET_BIT(c->other, 3);
        bool blocked = IS_NOT_SET_BIT(c->control, 4) && IS_SET_BIT(c->other, 3) && !reset_done;
        active = linked && (reset_done || (c->internal_pending_ticks > 0 && !blocked));
    }

    if (active)
        m_timer_service_mask = SET_BIT(m_timer_service_mask, unit);
    else
        m_timer_service_mask = UNSET_BIT(m_timer_service_mask, unit);
}

INLINE void Mikey::ClockTimer(int i)
{
    GLYNX_Mikey_Timer* t = &m_state.timers[i];

    t->control_b = UNSET_BIT(t->control_b, 0);
    t->control_b = UNSET_BIT(t->control_b, 1);

    if (IS_SET_BIT(t->control_a, 6))
        t->control_b = UNSET_BIT(t->control_b, 3);

    if (IS_NOT_SET_BIT(t->control_a, 4) && IS_SET_BIT(t->control_b, 3))
        return;

    t->control_b = SET_BIT(t->control_b, 1);
    BorrowInTimer(i, t);
}

INLINE void Mikey::ClockAudio(int i)
{
    GLYNX_Mikey_Audio* c = &m_state.audio[i];

    c->other = UNSET_BIT(c->other, 0);
    c->other = UNSET_BIT(c->other, 1);

    if (IS_SET_BIT(c->control, 6))
        c->other = UNSET_BIT(c->other, 3);

    if (IS_NOT_SET_BIT(c->control, 4) && IS_SET_BIT(c->other, 3))
        return;

    c->other = SET_BIT(c->other, 1);
    BorrowInChannel(i, c);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    TraceAudioEvent(TRACE_MIKEY_AUDIO_CLOCK, i, 0, 0);
#endif
}

INLINE void Mikey::ServiceTimer(int i)
{
    GLYNX_Mikey_Timer* t = &m_state.timers[i];

    t->control_b = UNSET_BIT(t->control_b, 0);
    t->control_b = UNSET_BIT(t->control_b, 1);

    if (IS_NOT_SET_BIT(t->control_a, 3) || t->internal_period_cycles != 0)
        return;

    if (IS_SET_BIT(t->control_a, 6))
        t->control_b = UNSET_BIT(t->control_b, 3);

    if (IS_NOT_SET_BIT(t->control_a, 4) && IS_SET_BIT(t->control_b, 3))
        return;

    int tick = t->internal_pending_ticks;
    t->internal_pending_ticks = 0;

    if (tick > 0)
        t->control_b = SET_BIT(t->control_b, 1);

    while (tick-- > 0)
    {
        if (!BorrowInTimer(i, t))
            break;
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
                m_timer_status_mask = SET_BIT(m_timer_status_mask, link);
                if (IS_NOT_SET_BIT(m_timer_service_mask, link))
                    UpdateTimerServiceMask(link);
            }
            else
            {
                m_state.audio[0].internal_pending_ticks++;
                m_state.audio[0].other = SET_BIT(m_state.audio[0].other, 1);
                m_timer_status_mask = SET_BIT(m_timer_status_mask, 8);
                if (IS_NOT_SET_BIT(m_timer_service_mask, 8))
                    UpdateTimerServiceMask(8);
            }
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            TraceTimerEvent(TRACE_MIKEY_TIMER_LINK, i, (u8)link, 0);
#endif
        }

        bool one_shot = IS_NOT_SET_BIT(t->control_a, 4);

        if (!one_shot)
            t->counter = t->backup;

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        TraceTimerEvent(TRACE_MIKEY_TIMER_UNDERFLOW, i);
#endif

        // IRQ on borrow attempt (except timer 4 / UART baud)
        if (IS_SET_BIT(t->control_a, 7) && (i != 4))
        {
            m_state.irq_pending = SET_BIT(m_state.irq_pending, i);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            TraceTimerEvent(TRACE_MIKEY_TIMER_IRQ, i);
#endif
        }

        if (likely(i == 0))
            HorizontalBlank();
        else if (i == 4 && IS_NOT_SET_BIT(m_state.MTEST0, 4))
            UartClock<false>();

        // In one-shot, after DONE we must not consume more clocks
        if (one_shot && IS_SET_BIT(t->control_b, 3))
            return false;
    }

    return true;
}

INLINE void Mikey::ServiceAudio(int i)
{
    GLYNX_Mikey_Audio* c = &m_state.audio[i];

    c->other = UNSET_BIT(c->other, 0);
    c->other = UNSET_BIT(c->other, 1);

    if (IS_NOT_SET_BIT(c->control, 3) || c->internal_period_cycles != 0)
        return;

    if (IS_SET_BIT(c->control, 6))
        c->other = UNSET_BIT(c->other, 3);

    if (IS_NOT_SET_BIT(c->control, 4) && IS_SET_BIT(c->other, 3))
        return;

    int tick = c->internal_pending_ticks;
    c->internal_pending_ticks = 0;

    if (tick > 0)
        c->other = SET_BIT(c->other, 1);

    while (tick-- > 0)
    {
        bool keep_clocking = BorrowInChannel(i, c);
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        TraceAudioEvent(TRACE_MIKEY_AUDIO_CLOCK, i, 0, 0);
#endif
        if (!keep_clocking)
            break;
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
            m_timer_status_mask = SET_BIT(m_timer_status_mask, link + 8);
            if (IS_NOT_SET_BIT(m_timer_service_mask, link + 8))
                UpdateTimerServiceMask(link + 8);
        }
        else // audio ch 3 links to timer 1
        {
            m_state.timers[1].internal_pending_ticks++;
            m_state.timers[1].control_b = SET_BIT(m_state.timers[1].control_b, 1);
            m_timer_status_mask = SET_BIT(m_timer_status_mask, 1);
            if (IS_NOT_SET_BIT(m_timer_service_mask, 1))
                UpdateTimerServiceMask(1);
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

INLINE void Mikey::AdvanceLFSR(u8 channel)
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

    // When channel is disabled games can use direct PCM writes
    if (IS_NOT_SET_BIT(c->control, 3))
    {
        c->internal_mix = true;
        return;
    }

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
    TraceInterruptEvent(TRACE_MIKEY_INTERRUPT_LINE);
    m_m6502->AssertIRQ(effective_irqs != 0, effective_irqs);

    if ((effective_irqs != 0) && m_m6502->IsHalted())
        m_m6502->Halt(false);
}

INLINE void Mikey::SetSuzyDone()
{
    m_state.suzy_done_pending = true;
    m_m6502->Halt(false);
}

INLINE void Mikey::UartRelevelIRQ()
{
    bool tx_level = (m_state.uart.tx_int_en && m_state.uart.tx_ready);
    bool rx_level = (m_state.uart.rx_int_en && m_state.uart.rx_ready);

    if (tx_level || rx_level)
        m_state.irq_pending = SET_BIT(m_state.irq_pending, 4);

    bool level = tx_level || rx_level;
    TraceUARTEvent(TRACE_MIKEY_UART_IRQ, level ? 1 : 0,
        (tx_level ? TRACE_MIKEY_UART_IRQ_SOURCE_TX : 0) |
        (rx_level ? TRACE_MIKEY_UART_IRQ_SOURCE_RX : 0));

    UpdateIRQs();
}

INLINE void Mikey::UartRxReflectHead()
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
    }
}

INLINE void Mikey::UartRxPush(u8 data, bool parbit, bool parerr, bool framerr, bool rxbreak, u8 source)
{
    u8 flags = (parbit ? TRACE_MIKEY_UART_FLAG_PARITY_BIT : 0) |
        (parerr ? TRACE_MIKEY_UART_FLAG_PARITY_ERROR : 0) |
        (framerr ? TRACE_MIKEY_UART_FLAG_FRAMING_ERROR : 0) |
        (rxbreak ? TRACE_MIKEY_UART_FLAG_BREAK : 0);

    bool room = (m_state.uart.rxq_count == 0) ||
                (m_state.uart.rxq_count == 1 && (source != 0 || m_state.uart.rx_age_cycles >= GLYNX_UART_RX_HOLD_CYCLES));
    bool lost = !room;

    if (lost)
        m_state.uart.ovr_err = true;

    u8 slot = room ? ((m_state.uart.rxq_head + m_state.uart.rxq_count) & 1)
                   : ((m_state.uart.rxq_head + m_state.uart.rxq_count - 1) & 1);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    TraceUARTEvent(TRACE_MIKEY_UART_RX_LATCH, data,
            lost ? (u8)(flags | TRACE_MIKEY_UART_FLAG_OVERRUN) : flags, source,
            lost ? m_state.uart.rxq_data[slot] : 0);
    if ((flags & 0x0E) != 0 || lost)
        TraceUARTEvent(TRACE_MIKEY_UART_PROBLEM, data,
            lost ? (u8)(flags | TRACE_MIKEY_UART_FLAG_OVERRUN) : flags, source,
            lost ? m_state.uart.rxq_data[slot] : 0);

    if (source != 0)
    {
        if (rxbreak)
            TraceRedEyeProblemEvent(1, TRACE_REDEYE_PROBLEM_BREAK, data);
        else if (framerr)
            TraceRedEyeProblemEvent(1, TRACE_REDEYE_PROBLEM_FRAMING, data);
        else
            TraceRedEyeEvent(1, data);
    }
#endif

    m_state.uart.rx_age_cycles = 0;

    m_state.uart.rxq_data[slot] = data;
    m_state.uart.rxq_flags[slot] = flags;

    if (room)
        m_state.uart.rxq_count++;

    UartRxReflectHead();
}

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
INLINE void Mikey::UartBeginFrame(u8 data, bool chained, bool trace)
#else
INLINE void Mikey::UartBeginFrame(u8 data, bool chained)
#endif
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_uart_tx_trace_active = false;
#endif
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

    m_uart_tx_wire_bit_cycles = GetUartBitCycles();
    m_uart_tx_wire_bits = (u16)(((u16)data << 1) |
        ((u16)(m_state.uart.tx_parbit ? 1 : 0) << 9) | 0x0400);
    m_uart_tx_wire_published = false;

    if (!chained && IsUartTurbo() && m_comlynx_cable_connected && m_state.uart.tx_open && m_comlynx_publish_callback)
    {
        u32 cycles_to_clock = (1 - m_state.timer_source_phase) & 15;

        if (cycles_to_clock == 0)
            cycles_to_clock = GLYNX_UART_TURBO_BIT_CYCLES;

        m_uart_tx_wire_start = m_comlynx_cycle + cycles_to_clock + GLYNX_UART_TURBO_BIT_CYCLES;
        m_comlynx_publish_callback(m_uart_tx_wire_start, m_uart_tx_wire_bit_cycles, m_uart_tx_wire_bits, m_comlynx_user_data);
        m_uart_tx_wire_published = true;
    }

    if (chained)
    {
        m_uart_tx_wire_start = m_comlynx_cycle + m_uart_tx_wire_bit_cycles;
        if (m_comlynx_cable_connected && m_state.uart.tx_open && m_comlynx_publish_callback)
        {
            m_comlynx_publish_callback(m_uart_tx_wire_start, m_uart_tx_wire_bit_cycles,
                m_uart_tx_wire_bits, m_comlynx_user_data);
        }
        m_uart_tx_wire_published = true;
    }

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (trace)
        TraceUARTEvent(TRACE_MIKEY_UART_TX_START, data,
            m_state.uart.tx_parbit ? TRACE_MIKEY_UART_FLAG_PARITY_BIT : 0,
            TRACE_MIKEY_UART_SOURCE_LOOPBACK, 0, chained);

    if (trace)
        TraceRedEyeEvent(0, data);
#endif

    m_state.uart.tx_active = true;
    m_state.uart.tx_empty = false;
    m_state.uart.tx_ready = false;
    m_state.uart.tx_empty_bits = 0;
    m_state.uart.tx_empty_cycles = 0;
    m_state.uart.tx_started_from_chain = false;
}

INLINE bool Mikey::UartWireLevel() const
{
    if (m_state.uart.tx_brk)
        return false;

    if (!m_state.uart.tx_active || m_state.uart.tx_start_bits > 0)
        return true;

    u8 bit = m_state.uart.tx_bit_index;
    if (bit >= 11)
        return true;

    return (m_uart_tx_wire_bits & (1u << bit)) != 0;
}

INLINE void Mikey::UartReceiveWire(bool level, bool peer_low)
{
    if (m_uart_rx_wire_state == 0)
    {
        if (!level)
        {
            m_uart_rx_wire_state = 1;
            m_uart_rx_wire_bit = 0;
            m_uart_rx_wire_data = 0;
            m_uart_rx_wire_parity = false;
            m_uart_rx_wire_link = peer_low;
        }
        return;
    }

    m_uart_rx_wire_link = m_uart_rx_wire_link || peer_low;

    if (m_uart_rx_wire_state == 1)
    {
        if (level)
            m_uart_rx_wire_data |= (u8)(1u << m_uart_rx_wire_bit);

        m_uart_rx_wire_bit++;
        if (m_uart_rx_wire_bit >= 8)
            m_uart_rx_wire_state = 2;
        return;
    }

    if (m_uart_rx_wire_state == 2)
    {
        m_uart_rx_wire_parity = level;
        m_uart_rx_wire_state = 3;
        return;
    }

    bool parity_error;
    if (m_state.uart.par_en)
    {
        bool odd = (parity8(m_uart_rx_wire_data) != 0);
        bool expected = m_state.uart.par_even ? odd : !odd;
        parity_error = m_uart_rx_wire_parity != expected;
    }
    else
        parity_error = m_uart_rx_wire_parity != m_state.uart.par_even;

    UartRxPush(m_uart_rx_wire_data, m_uart_rx_wire_parity, parity_error,
        !level, !level && m_uart_rx_wire_data == 0,
        m_uart_rx_wire_link ? TRACE_MIKEY_UART_SOURCE_COMLYNX :
        TRACE_MIKEY_UART_SOURCE_LOOPBACK);

    UartRelevelIRQ();

    m_uart_rx_wire_state = 0;
}

template<bool turbo>
inline void Mikey::UartClock()
{
    // If break is asserted, keep line busy and do not advance a normal frame
    if (m_state.uart.tx_brk)
    {
        m_state.uart.tx_empty = false;
        return;
    }

    if (!turbo)
    {
        m_state.uart.prescaler = (m_state.uart.prescaler + 1) & 7;

        if (m_state.uart.prescaler != 0)
            return;
    }

    u64 measured_bit_cycles = m_uart_last_bit_cycle == 0 ? 0 :
        m_comlynx_cycle - m_uart_last_bit_cycle;
    m_uart_last_bit_cycle = m_comlynx_cycle;

    if (measured_bit_cycles > 0 && measured_bit_cycles <= 0xFFFFFFFFULL)
        m_uart_tx_wire_bit_cycles = (u32)measured_bit_cycles;

    bool local_level = UartWireLevel();

    bool wire_level = local_level;

    if (m_comlynx_cable_connected)
    {
        if (turbo && m_comlynx_turbo_sample_callback)
            wire_level = wire_level && m_comlynx_turbo_sample_callback(m_comlynx_cycle, m_comlynx_turbo_user_data);
        else if (!turbo && m_comlynx_sample_callback)
            wire_level = wire_level && m_comlynx_sample_callback(m_comlynx_cycle, m_comlynx_user_data);
    }

    UartReceiveWire(wire_level, local_level && !wire_level);

    if (!m_state.uart.tx_active)
    {
        if (m_state.uart.tx_hold_valid)
        {
            if (m_state.uart.tx_ready_bits > 0)
            {
                m_state.uart.tx_ready_bits--;
                if (m_state.uart.tx_ready_bits != 0)
                    return;
            }

            u8 next = m_state.uart.tx_hold_data;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            bool trace = m_uart_tx_hold_trace;
#endif
            m_state.uart.tx_hold_valid = false;
            m_state.uart.tx_suppress_eof_loopback = false;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            UartBeginFrame(next, true, trace);
#else
            UartBeginFrame(next, true);
#endif
            m_state.uart.tx_ready = true;
            m_state.uart.tx_ready_bits = 0;
            UartRelevelIRQ();
            return;
        }

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

    if (m_state.uart.tx_start_bits > 0)
    {
        m_state.uart.tx_start_bits--;

        if (m_state.uart.tx_start_bits == 0 && !m_uart_tx_wire_published)
        {
            m_uart_tx_wire_start = m_comlynx_cycle + m_uart_tx_wire_bit_cycles;

            if (m_comlynx_cable_connected && m_state.uart.tx_open && m_comlynx_publish_callback)
            {
                m_comlynx_publish_callback(m_uart_tx_wire_start, m_uart_tx_wire_bit_cycles,
                    m_uart_tx_wire_bits, m_comlynx_user_data);
            }

            m_uart_tx_wire_published = true;
        }
        return;
    }

    m_state.uart.tx_bit_index++;

    if (m_state.uart.tx_bit_index >= 11)
    {
        // Frame complete on TX side
        m_state.uart.tx_active = false;

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        if (m_uart_tx_trace_active)
            TraceUARTEvent(TRACE_MIKEY_UART_TX_END, m_state.uart.tx_data,
                m_state.uart.tx_parbit ? TRACE_MIKEY_UART_FLAG_PARITY_BIT : 0);
        m_uart_tx_trace_active = false;
#endif

        if (m_state.uart.tx_suppress_eof_loopback)
            m_state.uart.tx_suppress_eof_loopback = false;

        // If there is a holding byte queued, start it now
        if (m_state.uart.tx_hold_valid)
        {
            u8 next = m_state.uart.tx_hold_data;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            bool trace = m_uart_tx_hold_trace;
#endif
            m_state.uart.tx_hold_valid = false;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            UartBeginFrame(next, true, trace);
#else
            UartBeginFrame(next, true);
#endif
            m_state.uart.tx_ready = true;
            m_state.uart.tx_ready_bits = 0;
            m_state.uart.tx_empty_bits = 0;
            m_state.uart.tx_started_from_chain = true;
        }
        else
        {
            m_state.uart.tx_ready = true;
            m_state.uart.tx_empty_bits = (m_state.uart.tx_started_from_chain ? 0 : 1);
            m_state.uart.tx_empty_cycles = (m_state.uart.tx_started_from_chain ? 1 : 0);
            m_state.uart.tx_started_from_chain = false;
            m_state.uart.tx_empty = (m_state.uart.tx_empty_bits == 0) && (m_state.uart.tx_empty_cycles == 0);
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
        // Coalesce refresh with enabled LCD transfers on visible lines.
        if (IS_NOT_SET_BIT(m_state.DISPCTL, 0) || m_lcd_screen->GetState()->in_vblank)
            m_bus->InjectCycles(k_mikey_refresh_inject_cycles);
    }
}

INLINE void Mikey::TraceTimerEvent(u8 event, int timer, u8 reg, u8 raw)
{
    if (IsValidPointer(m_trace_logger) && m_trace_logger->IsEventEnabled(TRACE_MIKEY_TIMER, event))
        LogTimerEvent(event, timer, reg, raw);
}

INLINE void Mikey::TraceInterruptEvent(u8 event, u8 reg, u8 raw)
{
    if (IsValidPointer(m_trace_logger) && m_trace_logger->IsEventEnabled(TRACE_MIKEY_INTERRUPT, event))
        LogInterruptEvent(event, reg, raw);
}

INLINE void Mikey::TraceDisplayEvent(u8 event, u8 reg, u8 raw, int line)
{
    if (IsValidPointer(m_trace_logger) && m_trace_logger->IsEventEnabled(TRACE_MIKEY_DISPLAY, event))
        LogDisplayEvent(event, reg, raw, line);
}

INLINE void Mikey::TracePaletteEvent(u8 index, u8 raw, u16 rgb444)
{
    if (IsValidPointer(m_trace_logger) &&
        m_trace_logger->IsEventEnabled(TRACE_MIKEY_DISPLAY, TRACE_MIKEY_DISPLAY_PALETTE))
        LogPaletteEvent(index, raw, rgb444);
}

INLINE void Mikey::TraceAudioEvent(u8 event, int channel, u8 reg, u8 raw)
{
    if (IsValidPointer(m_trace_logger) && m_trace_logger->IsEventEnabled(TRACE_MIKEY_AUDIO, event))
        LogAudioEvent(event, channel, reg, raw);
}

INLINE void Mikey::TraceUARTEvent(u8 event, u8 data, u8 flags, u8 source,
    u8 lost, bool chained)
{
    if (IsValidPointer(m_trace_logger) && m_trace_logger->IsEventEnabled(TRACE_MIKEY_UART, event))
        LogUARTEvent(event, data, flags, source, lost, chained);
}

INLINE void Mikey::TraceUARTConfigEvent(u8 value, bool register_write)
{
    if (IsValidPointer(m_trace_logger) &&
        m_trace_logger->IsEventEnabled(TRACE_MIKEY_UART, TRACE_MIKEY_UART_REGISTER))
        LogUARTConfigEvent(value, register_write);
}

INLINE void Mikey::TraceRedEyeEvent(u8 dir, u8 data)
{
    if (IsValidPointer(m_trace_logger) &&
        (m_trace_logger->IsEventEnabled(TRACE_REDEYE, TRACE_REDEYE_PACKET) ||
         m_trace_logger->IsEventEnabled(TRACE_REDEYE, TRACE_REDEYE_PROBLEM)))
        LogRedEyeEvent(dir, data);
}

INLINE void Mikey::TraceRedEyeProblemEvent(u8 dir, u8 problem, u8 value,
    u8 expected, u8 actual)
{
    if (IsValidPointer(m_trace_logger) &&
        (m_trace_logger->IsEventEnabled(TRACE_REDEYE, TRACE_REDEYE_PACKET) ||
         m_trace_logger->IsEventEnabled(TRACE_REDEYE, TRACE_REDEYE_PROBLEM)))
        LogRedEyeProblemEvent(dir, problem, value, expected, actual);
}

INLINE void Mikey::TraceRedEyeTimeoutEvent()
{
    if (IsValidPointer(m_trace_logger) &&
        m_trace_logger->IsEventEnabled(TRACE_REDEYE, TRACE_REDEYE_PROBLEM))
        LogRedEyeTimeoutEvent();
}

INLINE void Mikey::TraceCartridgeAddressEvent()
{
    if (IsValidPointer(m_trace_logger) &&
        m_trace_logger->IsEventEnabled(TRACE_CARTRIDGE, TRACE_CARTRIDGE_ADDRESS))
        LogCartridgeAddressEvent();
}

INLINE void Mikey::TraceCartridgeIOEvent(u8 event, u8 operation, u8 value)
{
    if (IsValidPointer(m_trace_logger) && m_trace_logger->IsEventEnabled(TRACE_CARTRIDGE, event))
        LogCartridgeIOEvent(event, operation, value);
}

INLINE void Mikey::TraceDebugMessageEvent(u16 address, u8 value)
{
    if (IsValidPointer(m_trace_logger) &&
        m_trace_logger->IsEventEnabled(TRACE_DEBUG_MESSAGE, TRACE_DEBUG_MESSAGE_OUTPUT))
        LogDebugMessageEvent(address, value);
}

#endif /* MIKEY_INLINE_H */
