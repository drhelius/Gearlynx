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

#ifndef TRACE_LOGGER_H
#define TRACE_LOGGER_H

#include "common.h"

#define TRACE_BUFFER_SIZE 100000

enum GLYNX_Trace_Type : u8
{
    TRACE_CPU = 0,
    TRACE_CPU_IRQ,
    TRACE_SUZY_MATH,
    TRACE_SUZY_SPRITE,
    TRACE_SUZY_INPUT,
    TRACE_MIKEY_TIMER,
    TRACE_MIKEY_UART,
    TRACE_REDEYE,
    TRACE_MIKEY_AUDIO,
    TRACE_CARTRIDGE,
    TRACE_DEBUG_MESSAGE,
    TRACE_MIKEY_INTERRUPT,
    TRACE_MIKEY_DISPLAY,
    TRACE_TYPE_COUNT,
};

#define TRACE_FLAG_CPU          (1U << TRACE_CPU)
#define TRACE_FLAG_CPU_IRQ      (1U << TRACE_CPU_IRQ)
#define TRACE_FLAG_SUZY_MATH    (1U << TRACE_SUZY_MATH)
#define TRACE_FLAG_SUZY_SPRITE  (1U << TRACE_SUZY_SPRITE)
#define TRACE_FLAG_SUZY_INPUT   (1U << TRACE_SUZY_INPUT)
#define TRACE_FLAG_MIKEY_TIMER  (1U << TRACE_MIKEY_TIMER)
#define TRACE_FLAG_MIKEY_INTERRUPT (1U << TRACE_MIKEY_INTERRUPT)
#define TRACE_FLAG_MIKEY_DISPLAY (1U << TRACE_MIKEY_DISPLAY)
#define TRACE_FLAG_MIKEY_UART   (1U << TRACE_MIKEY_UART)
#define TRACE_FLAG_REDEYE       (1U << TRACE_REDEYE)
#define TRACE_FLAG_MIKEY_AUDIO  (1U << TRACE_MIKEY_AUDIO)
#define TRACE_FLAG_CARTRIDGE    (1U << TRACE_CARTRIDGE)
#define TRACE_FLAG_DEBUG_MSG    (1U << TRACE_DEBUG_MESSAGE)
#define TRACE_FLAG_ALL          ((1U << TRACE_TYPE_COUNT) - 1)

static_assert(TRACE_TYPE_COUNT < 32, "Trace category flags exceed u32 width");

#define TRACE_CART_SHIFT        TRACE_CARTRIDGE
#define TRACE_FLAG_CART_SHIFT   TRACE_FLAG_CARTRIDGE

#define TRACE_EVENT_FLAG(event) (1U << (event))

enum GLYNX_Trace_Math_Event : u8
{
    TRACE_SUZY_MATH_OPERATION = 0,
    TRACE_SUZY_MATH_COMPLETION,
};

enum GLYNX_Trace_IRQ_Source : u8
{
    TRACE_CPU_IRQ_TIMER0 = 0x01,
    TRACE_CPU_IRQ_TIMER1 = 0x02,
    TRACE_CPU_IRQ_TIMER2 = 0x04,
    TRACE_CPU_IRQ_TIMER3 = 0x08,
    TRACE_CPU_IRQ_UART = 0x10,
    TRACE_CPU_IRQ_TIMER5 = 0x20,
    TRACE_CPU_IRQ_TIMER6 = 0x40,
    TRACE_CPU_IRQ_TIMER7 = 0x80,
};

enum GLYNX_Trace_Sprite_Event : u8
{
    TRACE_SUZY_SPRITE_ENGINE_START = 0,
    TRACE_SUZY_SPRITE_ENGINE_END = 1,
    TRACE_SUZY_SPRITE_SCB = 2,
    TRACE_SUZY_SPRITE_SKIP = 3,
    TRACE_SUZY_SPRITE_COLLISION = 4,
    TRACE_SUZY_SPRITE_ROW = 5,
    TRACE_SUZY_SPRITE_BUS = 6,
};

enum GLYNX_Trace_Sprite_Type : u8
{
    TRACE_SUZY_SPRITE_TYPE_BACKGROUND = 0,
    TRACE_SUZY_SPRITE_TYPE_BACKGROUND_NONCOLLIDABLE = 1,
    TRACE_SUZY_SPRITE_TYPE_BOUNDARY_SHADOW = 2,
    TRACE_SUZY_SPRITE_TYPE_BOUNDARY = 3,
    TRACE_SUZY_SPRITE_TYPE_NORMAL = 4,
    TRACE_SUZY_SPRITE_TYPE_NONCOLLIDABLE = 5,
    TRACE_SUZY_SPRITE_TYPE_XOR = 6,
    TRACE_SUZY_SPRITE_TYPE_SHADOW = 7,
};

enum GLYNX_Trace_Sprite_Skip : u8
{
    TRACE_SUZY_SPRITE_SKIP_NONE = 0,
    TRACE_SUZY_SPRITE_SKIP_DISABLED = 1,
    TRACE_SUZY_SPRITE_SKIP_STOPPED = 2,
    TRACE_SUZY_SPRITE_SKIP_INVALID_TERMINAL = 3,
};

enum GLYNX_Trace_Sprite_Bus_Reason : u8
{
    TRACE_SUZY_SPRITE_BUS_NONE = 0,
    TRACE_SUZY_SPRITE_BUS_DISPLAY_DMA = 1,
};

enum GLYNX_Trace_Input_Event : u8
{
    TRACE_SUZY_INPUT_READ = 0,
};

enum GLYNX_Trace_Timer_Event : u8
{
    TRACE_MIKEY_TIMER_REGISTER = 0,
    TRACE_MIKEY_TIMER_UNDERFLOW = 1,
    TRACE_MIKEY_TIMER_IRQ = 2,
    TRACE_MIKEY_TIMER_LINK = 3,
};

enum GLYNX_Trace_Interrupt_Event : u8
{
    TRACE_MIKEY_INTERRUPT_REGISTER = 0,
    TRACE_MIKEY_INTERRUPT_LINE = 1,
};

enum GLYNX_Trace_Interrupt_Kind : u8
{
    TRACE_MIKEY_INTERRUPT_CLEAR = 0,
    TRACE_MIKEY_INTERRUPT_SET = 1,
    TRACE_MIKEY_INTERRUPT_LINE_CHANGE = 2,
};

enum GLYNX_Trace_Display_Event : u8
{
    TRACE_MIKEY_DISPLAY_REGISTER = 0,
    TRACE_MIKEY_DISPLAY_PALETTE,
    TRACE_MIKEY_DISPLAY_DMA_START,
    TRACE_MIKEY_DISPLAY_DMA_LINE,
    TRACE_MIKEY_DISPLAY_DMA_END,
    TRACE_MIKEY_DISPLAY_VBLANK,
    TRACE_MIKEY_DISPLAY_FRAME,
};

enum GLYNX_Trace_Audio_Event : u8
{
    TRACE_MIKEY_AUDIO_CHANNEL = 0,
    TRACE_MIKEY_AUDIO_MIXER,
    TRACE_MIKEY_AUDIO_CLOCK,
};

enum GLYNX_Trace_UART_Event : u8
{
    TRACE_MIKEY_UART_REGISTER = 0,
    TRACE_MIKEY_UART_TX_START = 1,
    TRACE_MIKEY_UART_TX_END = 2,
    TRACE_MIKEY_UART_RX_LATCH = 3,
    TRACE_MIKEY_UART_DATA_READ = 4,
    TRACE_MIKEY_UART_IRQ = 5,
    TRACE_MIKEY_UART_PROBLEM = 6,
    TRACE_MIKEY_UART_BREAK = 7,
    TRACE_MIKEY_UART_COMLYNX = 8,
};

enum GLYNX_Trace_UART_Kind : u8
{
    TRACE_MIKEY_UART_KIND_CONFIG = 0,
    TRACE_MIKEY_UART_KIND_SERDAT_WRITE = 1,
    TRACE_MIKEY_UART_KIND_TX_START = 2,
    TRACE_MIKEY_UART_KIND_TX_END = 3,
    TRACE_MIKEY_UART_KIND_RX_LATCH = 4,
    TRACE_MIKEY_UART_KIND_DATA_READ = 5,
    TRACE_MIKEY_UART_KIND_IRQ_ASSERTED = 6,
    TRACE_MIKEY_UART_KIND_IRQ_CLEARED = 7,
    TRACE_MIKEY_UART_KIND_PROBLEM = 8,
    TRACE_MIKEY_UART_KIND_TX_BREAK_ASSERTED = 9,
    TRACE_MIKEY_UART_KIND_TX_BREAK_CLEARED = 10,
    TRACE_MIKEY_UART_KIND_CABLE_CONNECTED = 11,
    TRACE_MIKEY_UART_KIND_CABLE_DISCONNECTED = 12,
};

enum GLYNX_Trace_UART_Source : u8
{
    TRACE_MIKEY_UART_SOURCE_LOOPBACK = 0,
    TRACE_MIKEY_UART_SOURCE_COMLYNX = 1,
};

enum GLYNX_Trace_UART_IRQ_Source : u8
{
    TRACE_MIKEY_UART_IRQ_SOURCE_TX = 0x01,
    TRACE_MIKEY_UART_IRQ_SOURCE_RX = 0x02,
};

enum GLYNX_Trace_UART_Flag : u8
{
    TRACE_MIKEY_UART_FLAG_PARITY_BIT = 0x01,
    TRACE_MIKEY_UART_FLAG_PARITY_ERROR = 0x02,
    TRACE_MIKEY_UART_FLAG_FRAMING_ERROR = 0x04,
    TRACE_MIKEY_UART_FLAG_BREAK = 0x08,
    TRACE_MIKEY_UART_FLAG_OVERRUN = 0x10,
    TRACE_MIKEY_UART_FLAG_TURBO = 0x20,
};

enum GLYNX_Trace_RedEye_Event : u8
{
    TRACE_REDEYE_PACKET = 0,
    TRACE_REDEYE_PROBLEM,
};

enum GLYNX_Trace_RedEye_Message : u8
{
    TRACE_REDEYE_MESSAGE_LOGON = 0,
    TRACE_REDEYE_MESSAGE_START = 2,
    TRACE_REDEYE_MESSAGE_DATA = 3,
    TRACE_REDEYE_MESSAGE_REQ = 4,
    TRACE_REDEYE_MESSAGE_MASTER_RESEND = 5,
};

enum GLYNX_Trace_RedEye_Problem : u8
{
    TRACE_REDEYE_PROBLEM_INVALID_SIZE = 1,
    TRACE_REDEYE_PROBLEM_CHECKSUM,
    TRACE_REDEYE_PROBLEM_TIMEOUT,
    TRACE_REDEYE_PROBLEM_FRAMING,
    TRACE_REDEYE_PROBLEM_BREAK,
    TRACE_REDEYE_PROBLEM_RESET,
};

enum GLYNX_Trace_Cartridge_Event : u8
{
    TRACE_CARTRIDGE_ADDRESS = 0,
    TRACE_CARTRIDGE_ACCESS,
    TRACE_CARTRIDGE_EEPROM,
    TRACE_CARTRIDGE_AUDIN,
    TRACE_CARTRIDGE_STORAGE = 6,
};

enum GLYNX_Trace_EEPROM_Operation : u8
{
    TRACE_EEPROM_READ = 0,
    TRACE_EEPROM_WRITE = 1,
    TRACE_EEPROM_ERASE = 2,
    TRACE_EEPROM_EWDS = 3,
    TRACE_EEPROM_EWEN = 4,
    TRACE_EEPROM_WRAL = 5,
    TRACE_EEPROM_ERAL = 6,
    TRACE_EEPROM_READY = 7,
};

enum GLYNX_Trace_Debug_Event : u8
{
    TRACE_DEBUG_MESSAGE_OUTPUT = 0,
};

static_assert(TRACE_SUZY_MATH_COMPLETION < 32 && TRACE_SUZY_SPRITE_BUS < 32 &&
    TRACE_SUZY_INPUT_READ < 32 && TRACE_MIKEY_TIMER_LINK < 32 &&
    TRACE_MIKEY_INTERRUPT_LINE < 32 && TRACE_MIKEY_DISPLAY_FRAME < 32 &&
    TRACE_MIKEY_AUDIO_CLOCK < 32 && TRACE_MIKEY_UART_COMLYNX < 32 &&
    TRACE_REDEYE_PROBLEM < 32 && TRACE_CARTRIDGE_STORAGE < 32 &&
    TRACE_DEBUG_MESSAGE_OUTPUT < 32, "Trace event filters exceed u32 width");

#define TRACE_SUZY_MATH_FILTER_OPERATIONS   TRACE_EVENT_FLAG(TRACE_SUZY_MATH_OPERATION)
#define TRACE_SUZY_MATH_FILTER_COMPLETIONS  TRACE_EVENT_FLAG(TRACE_SUZY_MATH_COMPLETION)
#define TRACE_SUZY_MATH_FILTER_ALL          (TRACE_SUZY_MATH_FILTER_OPERATIONS | TRACE_SUZY_MATH_FILTER_COMPLETIONS)
#define TRACE_SUZY_SPRITE_FILTER_ENGINE     (TRACE_EVENT_FLAG(TRACE_SUZY_SPRITE_ENGINE_START) | TRACE_EVENT_FLAG(TRACE_SUZY_SPRITE_ENGINE_END))
#define TRACE_SUZY_SPRITE_FILTER_SCBS       TRACE_EVENT_FLAG(TRACE_SUZY_SPRITE_SCB)
#define TRACE_SUZY_SPRITE_FILTER_SKIPS      TRACE_EVENT_FLAG(TRACE_SUZY_SPRITE_SKIP)
#define TRACE_SUZY_SPRITE_FILTER_COLLISIONS TRACE_EVENT_FLAG(TRACE_SUZY_SPRITE_COLLISION)
#define TRACE_SUZY_SPRITE_FILTER_ROWS       TRACE_EVENT_FLAG(TRACE_SUZY_SPRITE_ROW)
#define TRACE_SUZY_SPRITE_FILTER_BUS        TRACE_EVENT_FLAG(TRACE_SUZY_SPRITE_BUS)
#define TRACE_SUZY_SPRITE_FILTER_ALL        (TRACE_SUZY_SPRITE_FILTER_ENGINE | TRACE_SUZY_SPRITE_FILTER_SCBS | TRACE_SUZY_SPRITE_FILTER_SKIPS | TRACE_SUZY_SPRITE_FILTER_COLLISIONS | TRACE_SUZY_SPRITE_FILTER_ROWS | TRACE_SUZY_SPRITE_FILTER_BUS)
#define TRACE_SUZY_INPUT_FILTER_READS       TRACE_EVENT_FLAG(TRACE_SUZY_INPUT_READ)
#define TRACE_SUZY_INPUT_FILTER_ALL         TRACE_SUZY_INPUT_FILTER_READS
#define TRACE_MIKEY_TIMER_FILTER_REGISTERS  TRACE_EVENT_FLAG(TRACE_MIKEY_TIMER_REGISTER)
#define TRACE_MIKEY_TIMER_FILTER_UNDERFLOWS TRACE_EVENT_FLAG(TRACE_MIKEY_TIMER_UNDERFLOW)
#define TRACE_MIKEY_TIMER_FILTER_IRQS       TRACE_EVENT_FLAG(TRACE_MIKEY_TIMER_IRQ)
#define TRACE_MIKEY_TIMER_FILTER_LINKS      TRACE_EVENT_FLAG(TRACE_MIKEY_TIMER_LINK)
#define TRACE_MIKEY_TIMER_FILTER_ALL        (TRACE_MIKEY_TIMER_FILTER_REGISTERS | TRACE_MIKEY_TIMER_FILTER_UNDERFLOWS | TRACE_MIKEY_TIMER_FILTER_IRQS | TRACE_MIKEY_TIMER_FILTER_LINKS)
#define TRACE_MIKEY_INTERRUPT_FILTER_ALL    (TRACE_EVENT_FLAG(TRACE_MIKEY_INTERRUPT_REGISTER) | TRACE_EVENT_FLAG(TRACE_MIKEY_INTERRUPT_LINE))
#define TRACE_MIKEY_DISPLAY_FILTER_REGISTERS TRACE_EVENT_FLAG(TRACE_MIKEY_DISPLAY_REGISTER)
#define TRACE_MIKEY_DISPLAY_FILTER_PALETTE  TRACE_EVENT_FLAG(TRACE_MIKEY_DISPLAY_PALETTE)
#define TRACE_MIKEY_DISPLAY_FILTER_DMA      (TRACE_EVENT_FLAG(TRACE_MIKEY_DISPLAY_DMA_START) | TRACE_EVENT_FLAG(TRACE_MIKEY_DISPLAY_DMA_LINE) | TRACE_EVENT_FLAG(TRACE_MIKEY_DISPLAY_DMA_END))
#define TRACE_MIKEY_DISPLAY_FILTER_TIMING   (TRACE_EVENT_FLAG(TRACE_MIKEY_DISPLAY_VBLANK) | TRACE_EVENT_FLAG(TRACE_MIKEY_DISPLAY_FRAME))
#define TRACE_MIKEY_DISPLAY_FILTER_ALL      (TRACE_MIKEY_DISPLAY_FILTER_REGISTERS | TRACE_MIKEY_DISPLAY_FILTER_PALETTE | TRACE_MIKEY_DISPLAY_FILTER_DMA | TRACE_MIKEY_DISPLAY_FILTER_TIMING)
#define TRACE_MIKEY_AUDIO_FILTER_CHANNELS   TRACE_EVENT_FLAG(TRACE_MIKEY_AUDIO_CHANNEL)
#define TRACE_MIKEY_AUDIO_FILTER_MIXER      TRACE_EVENT_FLAG(TRACE_MIKEY_AUDIO_MIXER)
#define TRACE_MIKEY_AUDIO_FILTER_CLOCKS     TRACE_EVENT_FLAG(TRACE_MIKEY_AUDIO_CLOCK)
#define TRACE_MIKEY_AUDIO_FILTER_ALL        (TRACE_MIKEY_AUDIO_FILTER_CHANNELS | TRACE_MIKEY_AUDIO_FILTER_MIXER | TRACE_MIKEY_AUDIO_FILTER_CLOCKS)
#define TRACE_MIKEY_UART_FILTER_REGISTERS   TRACE_EVENT_FLAG(TRACE_MIKEY_UART_REGISTER)
#define TRACE_MIKEY_UART_FILTER_TRANSFERS   (TRACE_EVENT_FLAG(TRACE_MIKEY_UART_TX_START) | TRACE_EVENT_FLAG(TRACE_MIKEY_UART_TX_END) | TRACE_EVENT_FLAG(TRACE_MIKEY_UART_RX_LATCH) | TRACE_EVENT_FLAG(TRACE_MIKEY_UART_DATA_READ))
#define TRACE_MIKEY_UART_FILTER_IRQS        TRACE_EVENT_FLAG(TRACE_MIKEY_UART_IRQ)
#define TRACE_MIKEY_UART_FILTER_PROBLEMS    TRACE_EVENT_FLAG(TRACE_MIKEY_UART_PROBLEM)
#define TRACE_MIKEY_UART_FILTER_BREAKS      TRACE_EVENT_FLAG(TRACE_MIKEY_UART_BREAK)
#define TRACE_MIKEY_UART_FILTER_COMLYNX     TRACE_EVENT_FLAG(TRACE_MIKEY_UART_COMLYNX)
#define TRACE_MIKEY_UART_FILTER_ALL         (TRACE_MIKEY_UART_FILTER_REGISTERS | TRACE_MIKEY_UART_FILTER_TRANSFERS | TRACE_MIKEY_UART_FILTER_IRQS | TRACE_MIKEY_UART_FILTER_PROBLEMS | TRACE_MIKEY_UART_FILTER_BREAKS | TRACE_MIKEY_UART_FILTER_COMLYNX)
#define TRACE_REDEYE_FILTER_PACKETS         TRACE_EVENT_FLAG(TRACE_REDEYE_PACKET)
#define TRACE_REDEYE_FILTER_PROBLEMS        TRACE_EVENT_FLAG(TRACE_REDEYE_PROBLEM)
#define TRACE_REDEYE_FILTER_ALL             (TRACE_REDEYE_FILTER_PACKETS | TRACE_REDEYE_FILTER_PROBLEMS)
#define TRACE_CARTRIDGE_FILTER_ADDRESS      TRACE_EVENT_FLAG(TRACE_CARTRIDGE_ADDRESS)
#define TRACE_CARTRIDGE_FILTER_ACCESSES     TRACE_EVENT_FLAG(TRACE_CARTRIDGE_ACCESS)
#define TRACE_CARTRIDGE_FILTER_EEPROM       TRACE_EVENT_FLAG(TRACE_CARTRIDGE_EEPROM)
#define TRACE_CARTRIDGE_FILTER_AUDIN        TRACE_EVENT_FLAG(TRACE_CARTRIDGE_AUDIN)
#define TRACE_CARTRIDGE_FILTER_STORAGE      TRACE_EVENT_FLAG(TRACE_CARTRIDGE_STORAGE)
#define TRACE_CARTRIDGE_FILTER_ALL          (TRACE_CARTRIDGE_FILTER_ADDRESS | TRACE_CARTRIDGE_FILTER_ACCESSES | TRACE_CARTRIDGE_FILTER_EEPROM | TRACE_CARTRIDGE_FILTER_AUDIN | TRACE_CARTRIDGE_FILTER_STORAGE)
#define TRACE_DEBUG_FILTER_MESSAGES         TRACE_EVENT_FLAG(TRACE_DEBUG_MESSAGE_OUTPUT)

struct GLYNX_Trace_Entry
{
    GLYNX_Trace_Type type;
    u64 cycle;
    union
    {
        struct
        {
            u16 pc;
            u8 a, x, y, s, p;
            u8 size;
            u8 opcodes[3];
            u8 mapctl;
            char name[64];
        } cpu;

        struct
        {
            u16 pc;
            u16 vector;
            u8 irq_mask;
        } irq;

        struct
        {
            u32 op_a;
            u32 op_b;
            u32 result;
            u32 elapsed_cycles;
            u16 remainder;
            u8 event;
            bool is_divide;
            bool is_signed;
            bool accumulate;
            bool div_by_zero;
            bool completed;
        } math;

        struct
        {
            u16 scb_addr;
            u16 scb_next;
            s16 hpos;
            s16 vpos;
            u8 sprctl0;
            u8 sprctl1;
            u8 sprcoll;
            u8 sprinit;
            u8 bpp;
            u8 type;
            u8 event;
            u8 reason;
            u8 sprgo;
            u8 suzybusen;
            u8 collision_id;
            u8 depository;
            bool skipped;
            bool is_start;
            bool is_end;
            bool everon;
            u32 total_cycles;
            u32 source_pixels;
            u32 output_pixels;
            u32 charged_cycles;
        } sprite;

        struct
        {
            u8 value;
            u8 event;
            bool is_joystick;
        } input;

        struct
        {
            u8 timer_id;
            u8 backup;
            u8 counter;
            u8 control_a;
            u8 control_b;
            u8 raw;
            u8 effective;
            u8 reg;
            u8 destination;
            u8 event;
            u8 irq_pending;
            u8 irq_mask;
            u8 irq_effective;
            bool linked;
            bool reload;
            bool one_shot;
        } timer;

        struct
        {
            u8 event;
            u8 reg;
            u8 raw;
            u8 pending;
            u8 mask;
            u8 effective;
            u8 kind;
            bool asserted;
        } interrupt;

        struct
        {
            u16 address;
            u16 value;
            u16 auxiliary;
            u8 event;
            u8 reg;
            u8 raw;
            u8 effective;
            u8 control;
            u8 line;
        } display;

        struct
        {
            u8 data;
            u8 flags;
            u8 source;
            u8 lost;        // byte an overrun destroyed
            u8 kind;        // GLYNX_Trace_UART_Kind
            u8 config;      // effective SERCTL configuration
            u8 status;      // stable SERCTL status snapshot
            u8 backup;      // TIM4 backup, so the configured baud can be shown
            u8 control;     // TIM4 control A, including the clock prescaler
            u16 gap_us;     // since the previous frame was latched
            u32 bit_cycles;
            u8 event;
            bool chained;   // TX followed straight on from the previous frame
        } uart;

        struct
        {
            u8 dir;         // 0 sent, 1 received
            u8 msg;         // header bits 0-2
            u8 player;      // header bits 3-6
            u8 seq;         // header bit 7
            u8 size;        // first byte of the packet
            u8 total;
            u8 captured;    // wire bytes captured, including size/header/checksum
            u8 len;         // payload bytes captured below
            u8 payload[64];
            u8 event;
            u8 problem;
            u8 value;
            u8 checksum_expected;
            u8 checksum_actual;
            bool checksum_ok;
            bool header_valid;
        } redeye;

        struct
        {
            u8 channel;
            u8 reg;
            u8 value;
            u8 effective;
            u8 event;
        } audio;

        struct
        {
            u8 addr_shift;
            u8 bit;
            u32 address;
            u16 value;
            u8 bank;
            u16 page;
            u8 event;
            u8 operation;
            u8 data_bits;
            bool write;
            bool audin;
        } cart;

        struct
        {
            char text[GLYNX_DEBUG_MSG_MAX_SIZE];
        } debug_msg;
    };
};

static_assert(sizeof(GLYNX_Trace_Entry) <= 96, "Trace entry exceeds memory budget");

class TraceLogger
{
public:
    TraceLogger(const u64* total_cycles = NULL);
    ~TraceLogger();
    void Reset();
    bool SetCapacity(u32 capacity);
    INLINE bool IsEnabled(GLYNX_Trace_Type type) const;
    INLINE bool IsEventEnabled(GLYNX_Trace_Type type, u8 event) const;
    INLINE void TraceLog(const GLYNX_Trace_Entry& entry);
    void SetEnabledFlags(u32 flags);
    void SetEventFilter(GLYNX_Trace_Type type, u32 filter);
    u32 GetEnabledFlags() const;
    u32 GetEventFilter(GLYNX_Trace_Type type) const;
    const GLYNX_Trace_Entry* GetBuffer() const;
    u32 GetCount() const;
    u32 GetCapacity() const;
    u32 GetPosition() const;
    u64 GetTotalLogged() const;
    u64 GetSequence() const;
    const GLYNX_Trace_Entry& GetEntry(u32 index) const;

private:
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    void UpdateEnabled();
#endif
    GLYNX_Trace_Entry* m_buffer;
    u32 m_position;
    u32 m_count;
    u32 m_capacity;
    u32 m_enabled_flags;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    bool m_enabled;
#endif
    u32 m_event_filters[TRACE_TYPE_COUNT];
    u64 m_total_logged;
    u64 m_sequence;
    const u64* m_total_cycles;
};

INLINE bool TraceLogger::IsEnabled(GLYNX_Trace_Type type) const
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (likely(!m_enabled))
        return false;

    return type < TRACE_TYPE_COUNT && (m_enabled_flags & (1U << type)) != 0;
#else
    UNUSED(type);
    return false;
#endif
}

INLINE bool TraceLogger::IsEventEnabled(GLYNX_Trace_Type type, u8 event) const
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (likely(!m_enabled))
        return false;

    return type < TRACE_TYPE_COUNT && (m_enabled_flags & (1U << type)) != 0 &&
        event < 32 && (m_event_filters[type] & TRACE_EVENT_FLAG(event)) != 0;
#else
    UNUSED(type);
    UNUSED(event);
    return false;
#endif
}

INLINE void TraceLogger::TraceLog(const GLYNX_Trace_Entry& entry)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_buffer[m_position] = entry;
    if (IsValidPointer(m_total_cycles))
        m_buffer[m_position].cycle = *m_total_cycles;
    m_position++;
    if (m_position == m_capacity)
        m_position = 0;
    if (m_count < m_capacity)
        m_count++;
    m_total_logged++;
    m_sequence++;
#else
    UNUSED(entry);
#endif
}

#endif /* TRACE_LOGGER_H */
