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
#include "random.h"
#include "state_serializer.h"
#include "lcd_screen.h"
#include "trace_logger.h"

Mikey::Mikey(Suzy* suzy, Media* media, M6502* m6502, Bus* bus, Random* random)
{
    m_suzy = suzy;
    m_media = media;
    m_m6502 = m6502;
    m_bus = bus;
    m_random = random;
    InitPointer(m_audio);
    InitPointer(m_memory);
    InitPointer(m_lcd_screen);
    InitPointer(m_trace_logger);
    m_debug_output_enabled = false;
    m_cpu_read_cycles = 0;
    m_comlynx_publish_callback = NULL;
    m_comlynx_sample_callback = NULL;
    m_comlynx_break_callback = NULL;
    m_comlynx_sync_callback = NULL;
    m_comlynx_user_data = NULL;
    m_comlynx_turbo_sample_callback = NULL;
    m_comlynx_turbo_sync_callback = NULL;
    m_comlynx_turbo_user_data = NULL;
    m_comlynx_cable_connected = false;
    m_comlynx_cycle = 0;
    m_uart_last_bit_cycle = 0;
    m_uart_tx_wire_start = 0;
    m_uart_tx_wire_bit_cycles = 0;
    m_uart_tx_wire_bits = 0x07FF;
    m_uart_tx_wire_published = false;
    m_uart_rx_wire_state = 0;
    m_uart_rx_wire_bit = 0;
    m_uart_rx_wire_data = 0;
    m_uart_rx_wire_parity = false;
    m_uart_rx_wire_link = false;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_uart_tx_trace_active = false;
    m_uart_tx_hold_trace = false;
    m_uart_trace_cfg = 0xFF;
    m_uart_trace_backup = 0xFF;
    m_uart_trace_control = 0xFF;
    m_uart_trace_turbo = 0xFF;
    m_trace_effective_irqs = 0;
    m_trace_uart_irq = false;
    memset(m_redeye, 0, sizeof(m_redeye));
#endif
}

Mikey::~Mikey()
{
    SafeDelete(m_lcd_screen);
}

void Mikey::Init(Memory* memory, GLYNX_Pixel_Format pixel_format)
{
    m_memory = memory;
    m_lcd_screen = new LcdScreen(this, memory, m_bus);
    m_lcd_screen->Init(pixel_format);
    Reset(true);
}

void Mikey::SetAudio(Audio* audio)
{
    m_audio = audio;
}

void Mikey::SetTraceLogger(TraceLogger* trace_logger)
{
    m_trace_logger = trace_logger;
}

void Mikey::SetDebugOutputEnabled(bool enabled)
{
    m_debug_output_enabled = enabled;
}

bool Mikey::IsDebugOutputEnabled()
{
    return m_debug_output_enabled;
}

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
void Mikey::ResetTraceUARTEventPairing()
{
    m_uart_tx_trace_active = false;
}
#endif

void Mikey::LogCartridgeAddressEvent()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_CARTRIDGE;
    entry.cart.event = TRACE_CARTRIDGE_ADDRESS;
    entry.cart.addr_shift = (u8)m_media->GetAddressShift();
    entry.cart.bit = m_media->GetShiftRegisterBit() ? 1 : 0;
    entry.cart.page = (u16)m_media->GetCounterValue();
    m_trace_logger->TraceLog(entry);
#endif
}

void Mikey::LogTimerEvent(u8 event, int timer, u8 reg, u8 raw)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    GLYNX_Mikey_Timer* state = &m_state.timers[timer];
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_MIKEY_TIMER;
    entry.timer.event = event;
    entry.timer.timer_id = (u8)timer;
    entry.timer.reg = reg;
    entry.timer.raw = raw;
    entry.timer.destination = event == TRACE_MIKEY_TIMER_LINK ? reg : 0xFF;
    entry.timer.backup = state->backup;
    entry.timer.counter = state->counter;
    entry.timer.control_a = state->control_a;
    entry.timer.control_b = state->control_b;
    if (event == TRACE_MIKEY_TIMER_REGISTER)
    {
        switch (reg)
        {
            case 0: entry.timer.effective = state->backup; break;
            case 1: entry.timer.effective = state->control_a; break;
            case 2: entry.timer.effective = state->counter; break;
            default: entry.timer.effective = state->control_b; break;
        }
    }
    else
        entry.timer.effective = state->counter;
    entry.timer.irq_pending = m_state.irq_pending;
    entry.timer.irq_mask = m_state.irq_mask;
    entry.timer.irq_effective = m_state.irq_pending & m_state.irq_mask;
    entry.timer.linked = state->internal_period_cycles == 0;
    entry.timer.reload = IS_SET_BIT(state->control_a, 4);
    entry.timer.one_shot = IS_NOT_SET_BIT(state->control_a, 4);
    m_trace_logger->TraceLog(entry);
#else
    UNUSED(event);
    UNUSED(timer);
    UNUSED(reg);
    UNUSED(raw);
#endif
}

void Mikey::LogInterruptEvent(u8 event, u8 reg, u8 raw)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (event == TRACE_MIKEY_INTERRUPT_LINE)
    {
        u8 effective = m_state.irq_pending & m_state.irq_mask;
        if (effective == m_trace_effective_irqs)
            return;
        m_trace_effective_irqs = effective;
    }
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_MIKEY_INTERRUPT;
    entry.interrupt.event = event;
    entry.interrupt.reg = reg;
    entry.interrupt.kind = event == TRACE_MIKEY_INTERRUPT_LINE ?
        TRACE_MIKEY_INTERRUPT_LINE_CHANGE : reg;
    entry.interrupt.raw = raw;
    entry.interrupt.pending = m_state.irq_pending;
    entry.interrupt.mask = m_state.irq_mask;
    entry.interrupt.effective = m_state.irq_pending & m_state.irq_mask;
    entry.interrupt.asserted = entry.interrupt.effective != 0;
    m_trace_logger->TraceLog(entry);
#else
    UNUSED(event);
    UNUSED(reg);
    UNUSED(raw);
#endif
}

void Mikey::LogDisplayEvent(u8 event, u8 reg, u8 raw, int line)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if ((event == TRACE_MIKEY_DISPLAY_DMA_START ||
        event == TRACE_MIKEY_DISPLAY_DMA_LINE ||
        event == TRACE_MIKEY_DISPLAY_DMA_END) &&
        IS_NOT_SET_BIT(m_state.DISPCTL, 0))
        return;

    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_MIKEY_DISPLAY;
    entry.display.event = event;
    entry.display.reg = reg;
    entry.display.raw = raw;
    entry.display.effective = raw;
    entry.display.control = m_state.DISPCTL;
    entry.display.address = m_state.DISPADR.value;
    entry.display.auxiliary = m_state.dispadr_latch;
    entry.display.line = line >= 0 ? (u8)line : (u8)m_lcd_screen->GetState()->current_line;
    m_trace_logger->TraceLog(entry);
#else
    UNUSED(event);
    UNUSED(reg);
    UNUSED(raw);
    UNUSED(line);
#endif
}

void Mikey::LogPaletteEvent(u8 index, u8 raw, u16 rgb444)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_MIKEY_DISPLAY;
    entry.display.event = TRACE_MIKEY_DISPLAY_PALETTE;
    entry.display.reg = index;
    entry.display.raw = raw;
    entry.display.value = rgb444;
    m_trace_logger->TraceLog(entry);
#else
    UNUSED(index);
    UNUSED(raw);
    UNUSED(rgb444);
#endif
}

void Mikey::LogAudioEvent(u8 event, int channel, u8 reg, u8 raw)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_MIKEY_AUDIO;
    entry.audio.event = event;
    entry.audio.channel = (u8)channel;
    entry.audio.reg = reg;
    entry.audio.value = raw;
    if (event == TRACE_MIKEY_AUDIO_CHANNEL)
    {
        GLYNX_Mikey_Audio* state = &m_state.audio[channel];
        switch (reg & 7)
        {
            case 0: entry.audio.effective = state->volume; break;
            case 1: entry.audio.effective = state->feedback; break;
            case 2: entry.audio.effective = (u8)state->output; break;
            case 3: entry.audio.effective = state->lfsr_low; break;
            case 4: entry.audio.effective = state->backup; break;
            case 5: entry.audio.effective = state->control; break;
            case 6: entry.audio.effective = state->counter; break;
            default: entry.audio.effective = state->other; break;
        }
    }
    else if (event == TRACE_MIKEY_AUDIO_CLOCK)
    {
        GLYNX_Mikey_Audio* state = &m_state.audio[channel];
        entry.audio.value = (u8)state->output;
        entry.audio.effective = (u8)state->output;
    }
    else
        entry.audio.effective = raw;
    m_trace_logger->TraceLog(entry);
#else
    UNUSED(event);
    UNUSED(channel);
    UNUSED(reg);
    UNUSED(raw);
#endif
}

void Mikey::LogUARTEvent(u8 event, u8 data, u8 flags, u8 source,
    u8 lost, bool chained)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (event == TRACE_MIKEY_UART_IRQ)
    {
        bool level = data != 0;
        if (level == m_trace_uart_irq)
            return;
        m_trace_uart_irq = level;
    }
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_MIKEY_UART;
    entry.uart.event = event;
    entry.uart.data = data;
    entry.uart.flags = flags;
    entry.uart.source = source;
    entry.uart.lost = lost;
    switch (event)
    {
        case TRACE_MIKEY_UART_REGISTER:
            entry.uart.kind = TRACE_MIKEY_UART_KIND_SERDAT_WRITE;
            break;
        case TRACE_MIKEY_UART_TX_START:
            entry.uart.kind = TRACE_MIKEY_UART_KIND_TX_START;
            break;
        case TRACE_MIKEY_UART_TX_END:
            entry.uart.kind = TRACE_MIKEY_UART_KIND_TX_END;
            break;
        case TRACE_MIKEY_UART_RX_LATCH:
            entry.uart.kind = TRACE_MIKEY_UART_KIND_RX_LATCH;
            break;
        case TRACE_MIKEY_UART_DATA_READ:
            entry.uart.kind = TRACE_MIKEY_UART_KIND_DATA_READ;
            break;
        case TRACE_MIKEY_UART_IRQ:
            entry.uart.kind = data ? TRACE_MIKEY_UART_KIND_IRQ_ASSERTED :
                TRACE_MIKEY_UART_KIND_IRQ_CLEARED;
            break;
        case TRACE_MIKEY_UART_PROBLEM:
            entry.uart.kind = TRACE_MIKEY_UART_KIND_PROBLEM;
            break;
        case TRACE_MIKEY_UART_BREAK:
            entry.uart.kind = data ? TRACE_MIKEY_UART_KIND_TX_BREAK_ASSERTED :
                TRACE_MIKEY_UART_KIND_TX_BREAK_CLEARED;
            break;
        case TRACE_MIKEY_UART_COMLYNX:
            entry.uart.kind = data ? TRACE_MIKEY_UART_KIND_CABLE_CONNECTED :
                TRACE_MIKEY_UART_KIND_CABLE_DISCONNECTED;
            break;
        default:
            entry.uart.kind = TRACE_MIKEY_UART_KIND_CONFIG;
            break;
    }
    u32 gap_cycles = (event == TRACE_MIKEY_UART_RX_LATCH ||
        event == TRACE_MIKEY_UART_DATA_READ || event == TRACE_MIKEY_UART_PROBLEM) ?
        m_state.uart.rx_age_cycles : 0;
    u32 gap_us = gap_cycles / (GLYNX_MASTER_CLOCK / 1000000);
    entry.uart.gap_us = gap_us > 0xFFFF ? 0xFFFF : (u16)gap_us;
    entry.uart.backup = m_state.timers[4].backup;
    entry.uart.control = m_state.timers[4].control_a;
    entry.uart.config = m_state.SERCTL & 0xD7;
    entry.uart.status = (m_state.uart.tx_ready ? 0x80 : 0) |
        (m_state.uart.rx_ready ? 0x40 : 0) |
        (m_state.uart.tx_empty ? 0x20 : 0) |
        (m_state.uart.par_err ? 0x10 : 0) |
        (m_state.uart.ovr_err ? 0x08 : 0) |
        (m_state.uart.fram_err ? 0x04 : 0) |
        (m_state.uart.rx_break ? 0x02 : 0) |
        (m_state.uart.par_bit ? 0x01 : 0);
    entry.uart.bit_cycles = GetUartBitCycles();
    entry.uart.chained = chained;
    m_trace_logger->TraceLog(entry);
    if (event == TRACE_MIKEY_UART_TX_START)
        m_uart_tx_trace_active = true;
#else
    UNUSED(event);
    UNUSED(data);
    UNUSED(flags);
    UNUSED(source);
    UNUSED(lost);
    UNUSED(chained);
#endif
}

void Mikey::LogUARTConfigEvent(u8 value, bool register_write)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    u8 config = value & 0xD7;
    bool reset_errors = register_write && IS_SET_BIT(value, 3);
    u8 backup = m_state.timers[4].backup;
    u8 control = m_state.timers[4].control_a;
    u8 turbo = IS_SET_BIT(m_state.MTEST0, 4) ? 1 : 0;
    if (!reset_errors && config == m_uart_trace_cfg && backup == m_uart_trace_backup &&
        control == m_uart_trace_control && turbo == m_uart_trace_turbo)
        return;
    m_uart_trace_cfg = config;
    m_uart_trace_backup = backup;
    m_uart_trace_control = control;
    m_uart_trace_turbo = turbo;
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_MIKEY_UART;
    entry.uart.event = TRACE_MIKEY_UART_REGISTER;
    entry.uart.kind = TRACE_MIKEY_UART_KIND_CONFIG;
    entry.uart.data = reset_errors ? value : config;
    entry.uart.flags = turbo ? TRACE_MIKEY_UART_FLAG_TURBO : 0;
    entry.uart.config = config;
    entry.uart.status = (m_state.uart.tx_ready ? 0x80 : 0) |
        (m_state.uart.rx_ready ? 0x40 : 0) |
        (m_state.uart.tx_empty ? 0x20 : 0) |
        (m_state.uart.par_err ? 0x10 : 0) |
        (m_state.uart.ovr_err ? 0x08 : 0) |
        (m_state.uart.fram_err ? 0x04 : 0) |
        (m_state.uart.rx_break ? 0x02 : 0) |
        (m_state.uart.par_bit ? 0x01 : 0);
    entry.uart.backup = backup;
    entry.uart.control = control;
    entry.uart.bit_cycles = GetUartBitCycles();
    m_trace_logger->TraceLog(entry);
#else
    UNUSED(value);
    UNUSED(register_write);
#endif
}

void Mikey::LogRedEyeProblemEvent(u8 dir, u8 problem, u8 value, u8 expected, u8 actual)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    RedEyeStream* stream = &m_redeye[dir & 1];
    stream->count = 0;
    stream->total = 0;
    stream->last_cycle = 0;

    if (!m_trace_logger->IsEventEnabled(TRACE_REDEYE, TRACE_REDEYE_PROBLEM))
        return;

    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_REDEYE;
    entry.redeye.event = TRACE_REDEYE_PROBLEM;
    entry.redeye.dir = dir;
    entry.redeye.problem = problem;
    entry.redeye.size = value;
    entry.redeye.value = value;
    entry.redeye.checksum_expected = expected;
    entry.redeye.checksum_actual = actual;
    m_trace_logger->TraceLog(entry);
#else
    UNUSED(dir);
    UNUSED(problem);
    UNUSED(value);
    UNUSED(expected);
    UNUSED(actual);
#endif
}

void Mikey::LogRedEyeEvent(u8 dir, u8 data)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    RedEyeStream* stream = &m_redeye[dir & 1];

    if (stream->count > 0)
    {
        u32 bit_cycles = IS_SET_BIT(m_state.MTEST0, 4) ? GLYNX_UART_TURBO_BIT_CYCLES :
            (m_state.timers[4].internal_period_cycles ? m_state.timers[4].internal_period_cycles : 16) *
            ((u32)m_state.timers[4].backup + 1) * 8;
        u64 timeout = (u64)bit_cycles * 64;
        if (m_comlynx_cycle - stream->last_cycle > timeout)
        {
            u8 count = stream->count;
            TraceRedEyeProblemEvent(dir, TRACE_REDEYE_PROBLEM_TIMEOUT, count);
            stream->count = 0;
            stream->total = 0;
            stream->last_cycle = 0;
        }
    }

    if (stream->count == 0)
    {
        if (data == 0 || data > 32)
        {
            TraceRedEyeProblemEvent(dir, TRACE_REDEYE_PROBLEM_INVALID_SIZE, data);
            return;
        }
        stream->total = (u8)(data + 2);
    }

    if (stream->count < sizeof(stream->buffer))
        stream->buffer[stream->count] = data;
    stream->count++;
    stream->last_cycle = m_comlynx_cycle;
    if (stream->count < stream->total)
        return;

    u8 size = stream->buffer[0];
    u8 header = stream->buffer[1];
    u32 sum = 0;
    for (u8 i = 0; i < size + 1u; i++)
        sum += stream->buffer[i];
    u8 expected = (u8)((255u - sum) & 0xFFu);
    bool checksum_ok = expected == stream->buffer[stream->total - 1];
    u8 actual = stream->buffer[stream->total - 1];

    if (m_trace_logger->IsEventEnabled(TRACE_REDEYE, TRACE_REDEYE_PACKET))
    {
        GLYNX_Trace_Entry entry = {};
        entry.type = TRACE_REDEYE;
        entry.redeye.event = TRACE_REDEYE_PACKET;
        entry.redeye.dir = dir;
        entry.redeye.msg = header & 0x07;
        entry.redeye.player = (header & 0x78) >> 3;
        entry.redeye.seq = (header & 0x80) ? 1 : 0;
        entry.redeye.size = size;
        entry.redeye.total = stream->total;
        entry.redeye.checksum_ok = checksum_ok;
        entry.redeye.checksum_expected = expected;
        entry.redeye.checksum_actual = actual;
        for (u8 i = 0; i < 8; i++)
        {
            u8 index = (u8)(i + 2);
            if (index + 1u < stream->total)
            {
                entry.redeye.payload[i] = stream->buffer[index];
                entry.redeye.len++;
            }
        }
        m_trace_logger->TraceLog(entry);
    }

    if (!checksum_ok)
        TraceRedEyeProblemEvent(dir, TRACE_REDEYE_PROBLEM_CHECKSUM, size, expected, actual);
    stream->count = 0;
    stream->total = 0;
    stream->last_cycle = 0;
#else
    UNUSED(dir);
    UNUSED(data);
#endif
}

void Mikey::LogRedEyeTimeoutEvent()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    u32 bit_cycles = IS_SET_BIT(m_state.MTEST0, 4) ? GLYNX_UART_TURBO_BIT_CYCLES :
        (m_state.timers[4].internal_period_cycles ? m_state.timers[4].internal_period_cycles : 16) *
        ((u32)m_state.timers[4].backup + 1) * 8;
    u64 timeout = (u64)bit_cycles * 64;
    for (u8 dir = 0; dir < 2; dir++)
    {
        RedEyeStream* stream = &m_redeye[dir];
        if (stream->count > 0 && m_comlynx_cycle - stream->last_cycle > timeout)
        {
            u8 count = stream->count;
            TraceRedEyeProblemEvent(dir, TRACE_REDEYE_PROBLEM_TIMEOUT, count);
            stream->count = 0;
            stream->total = 0;
            stream->last_cycle = 0;
        }
    }
#endif
}

void Mikey::LogCartridgeIOEvent(u8 event, u8 operation, u8 value)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_CARTRIDGE;
    entry.cart.event = event;
    entry.cart.operation = operation;
    entry.cart.value = value;
    entry.cart.write = true;
    entry.cart.addr_shift = (u8)m_media->GetAddressShift();
    entry.cart.page = (u16)m_media->GetCounterValue();
    entry.cart.audin = m_media->GetAudinValue();
    m_trace_logger->TraceLog(entry);
#else
    UNUSED(event);
    UNUSED(operation);
    UNUSED(value);
#endif
}

void Mikey::Reset(bool is_lynx2)
{
    if (m_state.uart.tx_open && m_state.uart.tx_brk &&
        m_comlynx_cable_connected && m_comlynx_break_callback)
    {
        m_comlynx_break_callback(false, m_comlynx_cycle, m_comlynx_user_data);
    }

    memset(&m_state, 0, sizeof(Mikey_State));
    m_state.suzy_done_pending = true;
    m_cpu_read_cycles = 0;

    m_is_lynx2 = is_lynx2;
    m_state.SYSCTL1 = 0x02;
    m_state.MTEST0 = 0;

    m_lcd_screen->Reset();

    ResetPalette();
    ResetTimers();
    ResetAudio();
    ResetUART();
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    ResetTraceDiagnostics(true);
#endif
}

void Mikey::ResetTimers()
{
    m_state.timer_source_phase = m_random->Next(7) + 1;

    for (int i = 0; i < 8; i++)
    {
        m_state.timers[i].backup = 0;
        m_state.timers[i].counter = 0;
        m_state.timers[i].control_a = 0;
        m_state.timers[i].control_b = 0;

        m_state.timers[i].internal_period_cycles = k_mikey_timer_period_cycles[0];
        m_state.timers[i].internal_pending_ticks = 0;
    }

    m_lcd_screen->ConfigureLineTiming();
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

        m_state.audio[i].internal_period_cycles = k_mikey_timer_period_cycles[0];
        m_state.audio[i].internal_pending_ticks = 0;
        m_state.audio[i].internal_lfsr = 0;
        m_state.audio[i].internal_taps_mask = 0;
        m_state.audio[i].internal_mix = true;
    }

    m_state.MSTEREO = 0x00;
    m_state.MPAN = 0x00;
    m_state.ATTEN_A = 0x00;
    m_state.ATTEN_B = 0x00;
    m_state.ATTEN_C = 0x00;
    m_state.ATTEN_D = 0x00;
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
    m_state.uart.tx_suppress_eof_loopback = false;
    m_state.uart.tx_parbit = false;
    m_state.uart.tx_hold_data = 0;
    m_state.uart.tx_data = 0;
    m_state.uart.rx_data = 0;
    m_state.uart.tx_bit_index = 0;
    m_state.uart.prescaler = 0;
    m_state.uart.tx_empty_bits = 0;
    m_state.uart.tx_ready_bits = 0;
    m_state.uart.tx_started_from_chain = false;
    m_state.uart.tx_empty_cycles = 0;
    m_state.uart.tx_start_bits = 0;
    m_state.uart.rx_age_cycles = 0;
    m_uart_last_bit_cycle = 0;
    m_uart_tx_wire_start = 0;
    m_uart_tx_wire_bit_cycles = 0;
    m_uart_tx_wire_bits = 0x07FF;
    m_uart_tx_wire_published = false;
    m_uart_rx_wire_state = 0;
    m_uart_rx_wire_bit = 0;
    m_uart_rx_wire_data = 0;
    m_uart_rx_wire_parity = false;
    m_uart_rx_wire_link = false;
}

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
void Mikey::ResetTraceDiagnostics(bool log_reset)
{
    m_uart_tx_trace_active = false;
    m_uart_tx_hold_trace = false;
    m_uart_trace_cfg = 0xFF;
    m_uart_trace_backup = 0xFF;
    m_uart_trace_control = 0xFF;
    m_uart_trace_turbo = 0xFF;
    m_trace_effective_irqs = m_state.irq_pending & m_state.irq_mask;
    m_trace_uart_irq = (m_state.uart.tx_int_en && m_state.uart.tx_ready) ||
        (m_state.uart.rx_int_en && m_state.uart.rx_ready);
    for (u8 dir = 0; dir < 2; dir++)
    {
        if (log_reset && m_redeye[dir].count > 0)
            TraceRedEyeProblemEvent(dir, TRACE_REDEYE_PROBLEM_RESET, m_redeye[dir].count);
        m_redeye[dir].count = 0;
        m_redeye[dir].total = 0;
        m_redeye[dir].last_cycle = 0;
    }
}
#endif

void Mikey::ResetPalette()
{
    for (int address = 0xFDA0; address < 0xFDC0; address++)
    WriteColor(address, 0xFF, true);
}

void Mikey::HorizontalBlank()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (!m_lcd_screen->GetState()->in_vblank)
        TraceDisplayEvent(TRACE_MIKEY_DISPLAY_DMA_LINE);
#endif
    m_state.refresh_cycle_counter = 0;
    m_lcd_screen->FinishLine();

    u8 counter = m_state.timers[2].counter;
    u8 backup = m_state.timers[2].backup;

    int first_visible_counter = (backup >= 104) ? 102 : (backup - 2);

    // Start of vblank 0
    if (counter == 0)
    {
        m_lcd_screen->SetVBlank(true);
        m_state.rest = true;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        TraceDisplayEvent(TRACE_MIKEY_DISPLAY_VBLANK, 0, 1);
        TraceDisplayEvent(TRACE_MIKEY_DISPLAY_DMA_END);
#endif

        // Clear lines that won't be rendered when backup < 104
        if (backup < 104)
        {
            int visible_lines = MAX(0, (int)backup - 2);
            for (int line = visible_lines; line < 102; line++)
            {
                m_lcd_screen->ClearLine(line);
            }
        }
    }
    // Start of vblank 1
    else if (counter == backup)
    {
        m_state.rest = false;
    }
    // Start of vblank 2
    else if (counter == (backup - 1))
    {
        m_state.dispadr_latch = m_state.DISPADR.value & 0xFFFC;
    }
    // Visible lines
    else if (counter <= first_visible_counter && counter >= 1)
    {
        int visible_line = first_visible_counter - counter;

        // Start of visible line 0 (end of vblank)
        if (visible_line == 0)
        {
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            TraceDisplayEvent(TRACE_MIKEY_DISPLAY_DMA_START, 0, 0, visible_line);
#endif
            m_lcd_screen->FirstDMA();
            m_lcd_screen->SetVBlank(false);
            m_state.frame_ready = true;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            TraceDisplayEvent(TRACE_MIKEY_DISPLAY_VBLANK, 0, 0, visible_line);
            TraceDisplayEvent(TRACE_MIKEY_DISPLAY_FRAME, 0, 0, visible_line);
#endif
        }
        // Start of visible line 1
        else if (visible_line == 1)
        {
            m_state.rest = true;
        }

        m_lcd_screen->ResetVisibleLine(visible_line);
    }

    m_lcd_screen->ResetLine(m_video_line_remainder);
}

bool Mikey::SwitchAudInValue()
{
    return IS_SET_BIT(m_state.IODIR, 4) && IS_SET_BIT(m_state.IODAT, 4);
}

void Mikey::SetComLynxCallbacks(GLYNX_ComLynx_Publish_Callback publish_callback,
    GLYNX_ComLynx_Sample_Callback sample_callback, GLYNX_ComLynx_Break_Callback break_callback,
    GLYNX_ComLynx_Sync_Callback sync_callback, void* user_data)
{
    m_comlynx_publish_callback = publish_callback;
    m_comlynx_sample_callback = sample_callback;
    m_comlynx_break_callback = break_callback;
    m_comlynx_sync_callback = sync_callback;
    m_comlynx_user_data = user_data;
}

void Mikey::SetComLynxTurboCallbacks(GLYNX_ComLynx_Turbo_Sample_Callback sample_callback,
    GLYNX_ComLynx_Turbo_Sync_Callback sync_callback, void* user_data)
{
    m_comlynx_turbo_sample_callback = sample_callback;
    m_comlynx_turbo_sync_callback = sync_callback;
    m_comlynx_turbo_user_data = user_data;
}

void Mikey::SetComLynxCableConnected(bool connected)
{
    bool changed = m_comlynx_cable_connected != connected;
    m_comlynx_cable_connected = connected;

    if (!connected)
    {
        m_uart_rx_wire_state = 0;
    }

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (changed)
        TraceUARTEvent(TRACE_MIKEY_UART_COMLYNX, connected ? 1 : 0);
#else
    UNUSED(changed);
#endif
}

bool Mikey::IsComLynxCableConnected() const
{
    return m_comlynx_cable_connected;
}

u64 Mikey::GetComLynxCycle() const
{
    return m_comlynx_cycle;
}

void Mikey::SaveState(std::ostream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer, GLYNX_SAVESTATE_VERSION);

    m_lcd_screen->SaveState(stream);
}

void Mikey::LoadState(std::istream& stream, int version)
{
    StateSerializer serializer(stream);
    Serialize(serializer, version);
    m_cpu_read_cycles = 0;
    m_uart_tx_wire_start = 0;
    m_uart_tx_wire_bit_cycles = 0;
    m_uart_tx_wire_bits = 0x07FF;
    m_uart_tx_wire_published = false;
    m_uart_rx_wire_state = 0;
    m_uart_rx_wire_link = false;

    m_lcd_screen->LoadState(stream);
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    ResetTraceDiagnostics(false);
#endif
}

void Mikey::Serialize(StateSerializer& s, int version)
{
    u32 legacy_timer0_cycles = 0;

    if (version >= 13)
        G_SERIALIZE(s, m_is_lynx2);

    for (int i = 0; i < 8; i++)
    {
        G_SERIALIZE(s, m_state.timers[i].backup);
        G_SERIALIZE(s, m_state.timers[i].control_a);
        G_SERIALIZE(s, m_state.timers[i].control_b);
        G_SERIALIZE(s, m_state.timers[i].counter);

        if (version < 24)
        {
            u32 legacy_cycles = 0;
            G_SERIALIZE(s, legacy_cycles);
            if (i == 0)
                legacy_timer0_cycles = legacy_cycles;
        }
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

        if (version < 24)
        {
            u32 legacy_cycles = 0;
            G_SERIALIZE(s, legacy_cycles);
        }
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
    G_SERIALIZE(s, m_state.uart.tx_suppress_eof_loopback);
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

    if (version >= 22)
    {
        G_SERIALIZE(s, m_state.uart.tx_start_bits);
        G_SERIALIZE(s, m_state.uart.rx_age_cycles);
    }
    else if (s.IsLoading())
    {
        m_state.uart.tx_start_bits = 0;
        m_state.uart.rx_age_cycles = 0;
    }

    if (version >= 23)
        G_SERIALIZE(s, m_state.uart.tx_empty_cycles);
    else if (s.IsLoading())
        m_state.uart.tx_empty_cycles = 0;

    G_SERIALIZE(s, m_state.ATTEN_A);
    G_SERIALIZE(s, m_state.ATTEN_B);
    G_SERIALIZE(s, m_state.ATTEN_C);
    G_SERIALIZE(s, m_state.ATTEN_D);
    G_SERIALIZE(s, m_state.MPAN);
    G_SERIALIZE(s, m_state.MSTEREO);
    G_SERIALIZE(s, m_state.SYSCTL1);
    if (s.IsLoading() && version < 16)
        m_state.SYSCTL1 = SET_BIT(m_state.SYSCTL1, 1);
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
    G_SERIALIZE(s, m_state.dispadr_latch);
    G_SERIALIZE(s, m_state.rest);
    G_SERIALIZE(s, m_state.refresh_cycle_counter);

    if (version >= 23)
        G_SERIALIZE(s, m_state.timer_source_phase);
    else if (s.IsLoading())
        m_state.timer_source_phase = legacy_timer0_cycles & 1023;

    if (version >= 20)
        G_SERIALIZE(s, m_state.suzy_done_pending);
    else if (s.IsLoading())
        m_state.suzy_done_pending = false;

    if (version >= 25)
        G_SERIALIZE(s, m_state.MTEST0);
    else if (s.IsLoading())
        m_state.MTEST0 = 0;
}

void Mikey::LogDebugMessageEvent(u16 address, u8 value)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    switch (address)
    {
        case MIKEY_DBGASCII:
            if (m_state.debug_msg_pos < (GLYNX_DEBUG_MSG_MAX_SIZE - 1))
                m_state.debug_msg_buffer[m_state.debug_msg_pos++] = (char)value;
            break;
        case MIKEY_DBGHEX:
            if (m_state.debug_msg_pos < (GLYNX_DEBUG_MSG_MAX_SIZE - 2))
            {
                static const char k_hex[] = "0123456789ABCDEF";
                m_state.debug_msg_buffer[m_state.debug_msg_pos++] = k_hex[(value >> 4) & 0x0F];
                m_state.debug_msg_buffer[m_state.debug_msg_pos++] = k_hex[value & 0x0F];
            }
            break;
        case MIKEY_DBGSTRL:
            m_state.debug_str_addr.low = value;
            break;
        case MIKEY_DBGSTRH:
        {
            m_state.debug_str_addr.high = value;
            u16 string_address = m_state.debug_str_addr.value;
            int max_copy = (GLYNX_DEBUG_MSG_MAX_SIZE - 1) - m_state.debug_msg_pos;
            for (int i = 0; i < max_copy; i++)
            {
                u8 character = m_memory->Read<true>(string_address++);
                if (character == 0)
                    break;
                m_state.debug_msg_buffer[m_state.debug_msg_pos++] = (char)character;
            }
            break;
        }
        case MIKEY_DBGOUT:
            if (value != 0 && m_state.debug_msg_pos > 0)
            {
                GLYNX_Trace_Entry entry = {};
                entry.type = TRACE_DEBUG_MESSAGE;
                int length = m_state.debug_msg_pos;
                memcpy(entry.debug_msg.text, m_state.debug_msg_buffer, length);
                entry.debug_msg.text[length] = '\0';
                m_trace_logger->TraceLog(entry);
                m_state.debug_msg_pos = 0;
            }
            break;
        default:
            break;
    }
    m_state.debug_msg_buffer[m_state.debug_msg_pos] = '\0';
#else
    UNUSED(address);
    UNUSED(value);
#endif
}
