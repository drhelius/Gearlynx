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
    TRACE_MIKEY_AUDIO,
    TRACE_CART_SHIFT,
    TRACE_TYPE_COUNT,
};

#define TRACE_FLAG_CPU          (1 << TRACE_CPU)
#define TRACE_FLAG_CPU_IRQ      (1 << TRACE_CPU_IRQ)
#define TRACE_FLAG_SUZY_MATH    (1 << TRACE_SUZY_MATH)
#define TRACE_FLAG_SUZY_SPRITE  (1 << TRACE_SUZY_SPRITE)
#define TRACE_FLAG_SUZY_INPUT   (1 << TRACE_SUZY_INPUT)
#define TRACE_FLAG_MIKEY_TIMER  (1 << TRACE_MIKEY_TIMER)
#define TRACE_FLAG_MIKEY_UART   (1 << TRACE_MIKEY_UART)
#define TRACE_FLAG_MIKEY_AUDIO  (1 << TRACE_MIKEY_AUDIO)
#define TRACE_FLAG_CART_SHIFT   (1 << TRACE_CART_SHIFT)
#define TRACE_FLAG_ALL          0xFF

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
        } cpu;

        struct
        {
            u16 pc;
            u16 vector;
            u8 irq_mask;
        } irq;

        struct
        {
            u16 op_a;
            u16 op_b;
            u32 result;
            u16 remainder;
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
            u8 bpp;
            u8 type;
            bool skipped;
            bool is_start;
            bool is_end;
            u32 total_cycles;
        } sprite;

        struct
        {
            u8 value;
            bool is_joystick;
        } input;

        struct
        {
            u8 timer_id;
            u8 backup;
        } timer;

        struct
        {
            u8 data;
            u8 flags;
            bool is_tx;
        } uart;

        struct
        {
            u8 channel;
            u8 reg;
            u8 value;
        } audio;

        struct
        {
            u8 addr_shift;
            u8 bit;
        } cart;
    };
};

class TraceLogger
{
public:
    TraceLogger();
    ~TraceLogger();
    void Reset();
    INLINE bool IsEnabled(GLYNX_Trace_Type type) const;
    INLINE void TraceLog(const GLYNX_Trace_Entry& entry);
    void SetEnabledFlags(u32 flags);
    u32 GetEnabledFlags() const;
    const GLYNX_Trace_Entry* GetBuffer() const;
    u32 GetCount() const;
    u32 GetPosition() const;
    u64 GetTotalLogged() const;
    const GLYNX_Trace_Entry& GetEntry(u32 index) const;

private:
    GLYNX_Trace_Entry* m_buffer;
    u32 m_position;
    u32 m_count;
    u32 m_enabled_flags;
    u64 m_total_logged;
};

INLINE bool TraceLogger::IsEnabled(GLYNX_Trace_Type type) const
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    return (m_enabled_flags & (1 << type)) != 0;
#else
    UNUSED(type);
    return false;
#endif
}

INLINE void TraceLogger::TraceLog(const GLYNX_Trace_Entry& entry)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_buffer[m_position] = entry;
    m_position = (m_position + 1) % TRACE_BUFFER_SIZE;
    if (m_count < TRACE_BUFFER_SIZE)
        m_count++;
    m_total_logged++;
#else
    UNUSED(entry);
#endif
}

#endif /* TRACE_LOGGER_H */
