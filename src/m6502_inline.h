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

#ifndef HUC6280_INLINE_H
#define HUC6280_INLINE_H

#include "m6502.h"
#include "m6502_timing.h"
#include "memory.h"

inline bool M6502::Clock()
{
    if (m_clock % 3 == 0)
        ClockTimer();

    bool instruction_completed = false;

    if (m_clock % k_m6502_speed_divisor[m_speed] == 0)
    {
        if (m_clock_cycles <= 0)
        {
            if (m_irq_pending != 0)
            {
                m_clock_cycles += TickIRQ();
                if (m_clock_cycles == 0)
                    m_clock_cycles += TickOPCode();
            }
            else
                m_clock_cycles += TickOPCode();
        }

        if (m_transfer)
            CheckIRQs();

        m_clock_cycles--;
        instruction_completed = (m_clock_cycles == 0);
    }

    m_clock = (m_clock + 1) % 12;

    return instruction_completed;
}

inline u32 M6502::TickOPCode()
{
    m_transfer = false;
    m_memory_breakpoint_hit = false;
    m_skip_flag_transfer_clear = false;
    m_cycles = 0;

    u8 opcode = Fetch8();
    (this->*m_opcodes[opcode])();

#if defined(GLYNX_TESTING)
    SetFlag(FLAG_TRANSFER);
#else
    if (!m_skip_flag_transfer_clear)
        ClearFlag(FLAG_TRANSFER);
#endif

    DisassembleNextOPCode();

    m_cycles += k_m6502_opcode_cycles[opcode];

    m_last_instruction_cycles = m_cycles;

    return m_cycles;
}

inline u32 M6502::TickIRQ()
{
    assert(m_irq_pending != 0);

    m_cycles = 0;
    u16 vector = 0;

    // TIQ
    if (IsSetBit(m_irq_pending, 2) && IsSetBit(m_interrupt_request_register, 2))
    {
        vector = 0xFFFA;
        m_debug_next_irq = 3;
    }
    // IRQ1
    else if (IsSetBit(m_irq_pending, 1))
    {
        vector = 0xFFF8;
        m_debug_next_irq = 4;
    }
    // IRQ2
    else if (IsSetBit(m_irq_pending, 0))
    {
        vector = 0xFFF6;
        m_debug_next_irq = 5;
    }
    else
        return 0;

    u16 pc = m_PC.GetValue();
    StackPush16(pc);
    StackPush8(m_P.GetValue() & ~FLAG_BREAK);
    SetFlag(FLAG_INTERRUPT);
    ClearFlag(FLAG_DECIMAL | FLAG_TRANSFER);
    m_PC.SetLow(MemoryRead(vector));
    m_PC.SetHigh(MemoryRead(vector + 1));
    m_cycles += 8;

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    DisassembleNextOPCode();
    if (m_breakpoints_irq_enabled)
        m_cpu_breakpoint_hit = true;
    u16 dest = m_PC.GetValue();
    PushCallStack(pc, dest, pc);
#endif

    return m_cycles;
}

inline void M6502::AssertIRQ1(bool asserted)
{
    if (asserted)
        m_interrupt_request_register = SetBit(m_interrupt_request_register, 1);
    else
        m_interrupt_request_register = UnsetBit(m_interrupt_request_register, 1);
}

inline void M6502::AssertIRQ2(bool asserted)
{
    if (asserted)
        m_interrupt_request_register = SetBit(m_interrupt_request_register, 0);
    else
        m_interrupt_request_register = UnsetBit(m_interrupt_request_register, 0);
}

inline void M6502::InjectCycles(unsigned int cycles)
{
    m_cycles += cycles;
    CheckIRQs();
}

inline void M6502::CheckIRQs()
{
    m_irq_pending = IsSetFlag(FLAG_INTERRUPT) ? 0 : m_interrupt_request_register & ~m_interrupt_disable_register;
}

inline u8 M6502::MemoryRead(u16 address, bool block_transfer)
{
    CheckIRQs();
    return m_memory->Read(address, block_transfer);
}

inline void M6502::MemoryWrite(u16 address, u8 value)
{
    CheckIRQs();
    m_memory->Write(address, value);
}

inline u8 M6502:: ReadInterruptRegister(u16 address)
{
    if ((address & 1) == 0)
        return m_interrupt_disable_register;
    else
        return m_interrupt_request_register;
}

inline void M6502::WriteInterruptRegister(u16 address, u8 value)
{
    if ((address & 1) == 0)
        m_interrupt_disable_register = value & 0x07;
    else
    {
        // Acknowledge TIQ
        m_interrupt_request_register = UnsetBit(m_interrupt_request_register, 2);
    }
}

inline void M6502::ClockTimer()
{
    if (!m_timer_enabled)
        return;

    m_timer_cycles -= 3;

    if (m_timer_cycles == 0)
    {
        m_timer_cycles = k_m6502_timer_divisor;

        if (m_timer_counter == 0)
        {
            m_timer_counter = m_timer_reload;
            m_interrupt_request_register = SetBit(m_interrupt_request_register, 2);
        }
        else
            m_timer_counter--;
    }
}

inline u8 M6502::ReadTimerRegister()
{
    if(m_timer_counter == 0 && m_timer_cycles <= 5 * 3)
        return 0x7F;
    else
        return m_timer_counter;
}

inline void M6502::WriteTimerRegister(u16 address, u8 value)
{
    if (address & 0x01)
    {
        bool enabled = (value & 0x01);
        if (m_timer_enabled != enabled)
        {
            m_timer_enabled = enabled;
            m_timer_counter = m_timer_reload;
            m_timer_cycles = k_m6502_timer_divisor;
        }
    }
    else
        m_timer_reload = value & 0x7F;
}

inline u8 M6502::Fetch8()
{
    u8 value = MemoryRead(m_PC.GetValue());
    m_PC.Increment();
    return value;
}

inline u16 M6502::Fetch16()
{
    u16 pc = m_PC.GetValue();
    u8 l = MemoryRead(pc);
    u8 h = MemoryRead(pc + 1);
    m_PC.SetValue(pc + 2);
    return Address16(h , l);
}

inline u16 M6502::Address16(u8 high, u8 low)
{
    return static_cast<u16>(high << 8 ) | low;
}

inline bool M6502::PageCrossed(u16 old_address, u16 new_address)
{
    return (old_address ^ new_address) > 0x00FF;
}

inline u16 M6502::ZeroPageX()
{
    return ZERO_PAGE_ADDR | m_X.GetValue();
}

inline void M6502::SetOrClearZNFlags(u8 result)
{
    ClearFlag(FLAG_ZERO | FLAG_NEGATIVE);
    m_P.SetValue(m_P.GetValue() | m_zn_flags_lut[result]);
}

inline void M6502::SetZNFlags(u8 result)
{
    m_P.SetValue(m_P.GetValue() | m_zn_flags_lut[result]);
}

inline void M6502::SetOverflowFlag(u8 result)
{
    m_P.SetValue((m_P.GetValue() & 0xBF) | (result & 0x40));
}

inline void M6502::SetFlag(u8 flag)
{
    m_P.SetValue(m_P.GetValue() | flag);
}

inline void M6502::ClearFlag(u8 flag)
{
    m_P.SetValue(m_P.GetValue() & (~flag));
}

inline bool M6502::IsSetFlag(u8 flag)
{
    return (m_P.GetValue() & flag) != 0;
}

inline void M6502::StackPush16(u16 value)
{
    MemoryWrite(STACK_ADDR | m_S.GetValue(), static_cast<u8>(value >> 8));
    m_S.Decrement();
    MemoryWrite(STACK_ADDR | m_S.GetValue(), static_cast<u8>(value & 0x00FF));
    m_S.Decrement();
}

inline void M6502::StackPush8(u8 value)
{
    MemoryWrite(STACK_ADDR | m_S.GetValue(), value);
    m_S.Decrement();
}

inline u16 M6502::StackPop16()
{
    m_S.Increment();
    u8 l = MemoryRead(STACK_ADDR | m_S.GetValue());
    m_S.Increment();
    u8 h = MemoryRead(STACK_ADDR | m_S.GetValue());
    return Address16(h , l);
}

inline u8 M6502::StackPop8()
{
    m_S.Increment();
    return MemoryRead(STACK_ADDR | m_S.GetValue());
}

inline u8 M6502::ImmediateAddressing()
{
    return Fetch8();
}

inline u16 M6502::ZeroPageAddressing()
{
    return ZERO_PAGE_ADDR | Fetch8();
}

inline u16 M6502::ZeroPageAddressing(EightBitRegister* reg)
{
    return ZERO_PAGE_ADDR | ((Fetch8() + reg->GetValue()) & 0xFF);
}

inline u16 M6502::ZeroPageRelativeAddressing()
{
    u16 address = ZeroPageAddressing();
    s8 offset = static_cast<s8>(Fetch8());
    return address + offset;
}

inline u16 M6502::ZeroPageIndirectAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = MemoryRead(address);
    u8 h = MemoryRead((address + 1) & 0x20FF);
    return Address16(h, l);
}

inline u16 M6502::ZeroPageIndexedIndirectAddressing()
{
    u16 address = (ZeroPageAddressing() + m_X.GetValue()) & 0x20FF;
    u8 l = MemoryRead(address);
    u8 h = MemoryRead((address + 1) & 0x20FF);
    return Address16(h, l);
}

inline u16 M6502::ZeroPageIndirectIndexedAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = MemoryRead(address);
    u8 h = MemoryRead((address + 1) & 0x20FF);
    return Address16(h, l) + m_Y.GetValue();
}

inline s8 M6502::RelativeAddressing()
{
    return static_cast<s8>(Fetch8());
}

inline u16 M6502::AbsoluteAddressing()
{
    return Fetch16();
}

inline u16 M6502::AbsoluteAddressing(EightBitRegister* reg)
{
    u16 address = Fetch16();
    u16 result = address + reg->GetValue();
    return result;
}

inline u16 M6502::AbsoluteIndirectAddressing()
{
    u16 address = Fetch16();
    u8 l = MemoryRead(address);
    u8 h = MemoryRead(address + 1);
    return Address16(h, l);
}

inline u16 M6502::AbsoluteIndexedIndirectAddressing()
{
    u16 address = Fetch16() + m_X.GetValue();
    u8 l = MemoryRead(address);
    u8 h = MemoryRead(address + 1);
    return Address16(h, l);
}

#endif /* HUC6280_INLINE_H */