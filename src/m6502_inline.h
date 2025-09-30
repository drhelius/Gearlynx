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

#ifndef M6502_INLINE_H
#define M6502_INLINE_H

#include <string.h>
#include "m6502.h"
#include "m6502_timing.h"
#include "m6502_names.h"
#include "memory.h"

INLINE u32 M6502::RunInstruction(bool* instruction_completed)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_memory_breakpoint_hit = false;
#else
    UNUSED(instruction_completed);
#endif

    m_s.cycles = 0;

    if (unlikely(m_halted))
    {
        if (m_s.irq_asserted)
            m_halted = false;
            
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
            if (IsValidPointer(instruction_completed))
                *instruction_completed = false;
#endif
        return 4;
    }

    u8 opcode = Fetch8();
    CheckIRQs();
    (this->*m_opcodes[opcode])();

    if(m_s.irq_pending)
        HandleIRQ();

    DisassembleNextOPCode();

    m_s.cycles += k_m6502_opcode_cycles[opcode];

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (IsValidPointer(instruction_completed))
        *instruction_completed = true;   
#endif

    return m_s.cycles;
}

inline void M6502::HandleIRQ()
{
    u16 pc = m_s.PC.GetValue();
    StackPush16(pc);
    StackPush8(m_s.P.GetValue() & ~FLAG_BREAK);
    SetFlag(FLAG_INTERRUPT);
    ClearFlag(FLAG_DECIMAL);

    m_s.PC.SetLow(m_memory->Read(0xFFFE));
    m_s.PC.SetHigh(m_memory->Read(0xFFFF));

    m_s.cycles += 7;

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_s.debug_next_irq = 3;
    u16 dest = m_s.PC.GetValue();
    PushCallStack(pc, dest, pc);
#endif
}

INLINE void M6502::CheckIRQs()
{
    m_s.irq_pending = IsSetFlag(FLAG_INTERRUPT) ? false : m_s.irq_asserted;
}

INLINE void M6502::AssertIRQ(bool asserted)
{
    m_s.irq_asserted = asserted;
}

INLINE void M6502::Halt(bool halted)
{
    m_halted = halted;
}

INLINE void M6502::InjectCycles(unsigned int cycles)
{
    m_s.cycles += cycles;
}

INLINE u8 M6502::Fetch8()
{
    u8 value = m_memory->Read(m_s.PC.GetValue());
    m_s.PC.Increment();
    return value;
}

INLINE u16 M6502::Fetch16()
{
    u16 pc = m_s.PC.GetValue();
    u8 l = m_memory->Read(pc);
    u8 h = m_memory->Read(pc + 1);
    m_s.PC.SetValue(pc + 2);
    return Address16(h, l);
}

INLINE u16 M6502::Address16(u8 high, u8 low)
{
    return static_cast<u16>(high << 8) | low;
}

INLINE bool M6502::PageCrossed(u16 old_address, u16 new_address)
{
    return (old_address ^ new_address) > 0x00FF;
}

INLINE u16 M6502::ZeroPageX()
{
    return ZERO_PAGE_ADDR | m_s.X.GetValue();
}

INLINE void M6502::SetOrClearZNFlags(u8 result)
{
    ClearFlag(FLAG_ZERO | FLAG_NEGATIVE);
    m_s.P.SetValue(m_s.P.GetValue() | m_zn_flags_lut[result]);
}

INLINE void M6502::SetZNFlags(u8 result)
{
    m_s.P.SetValue(m_s.P.GetValue() | m_zn_flags_lut[result]);
}

INLINE void M6502::SetOverflowFlag(u8 result)
{
    m_s.P.SetValue((m_s.P.GetValue() & 0xBF) | (result & 0x40));
}

INLINE void M6502::SetFlag(u8 flag)
{
    m_s.P.SetValue(m_s.P.GetValue() | flag);
}

INLINE void M6502::ClearFlag(u8 flag)
{
    m_s.P.SetValue(m_s.P.GetValue() & (~flag));
}

INLINE bool M6502::IsSetFlag(u8 flag)
{
    return (m_s.P.GetValue() & flag) != 0;
}

INLINE bool M6502::IsNotSetFlag(u8 flag)
{
    return (m_s.P.GetValue() & flag) == 0;
}

INLINE void M6502::StackPush16(u16 value)
{
    m_memory->Write(STACK_ADDR | m_s.S.GetValue(), static_cast<u8>(value >> 8));
    m_s.S.Decrement();
    m_memory->Write(STACK_ADDR | m_s.S.GetValue(), static_cast<u8>(value & 0x00FF));
    m_s.S.Decrement();
}

INLINE void M6502::StackPush8(u8 value)
{
    m_memory->Write(STACK_ADDR | m_s.S.GetValue(), value);
    m_s.S.Decrement();
}

INLINE u16 M6502::StackPop16()
{
    m_s.S.Increment();
    u8 l = m_memory->Read(STACK_ADDR | m_s.S.GetValue());
    m_s.S.Increment();
    u8 h = m_memory->Read(STACK_ADDR | m_s.S.GetValue());
    return Address16(h, l);
}

INLINE u8 M6502::StackPop8()
{
    m_s.S.Increment();
    return m_memory->Read(STACK_ADDR | m_s.S.GetValue());
}

INLINE u8 M6502::ImmediateAddressing()
{
    return Fetch8();
}

INLINE u16 M6502::ZeroPageAddressing()
{
    return ZERO_PAGE_ADDR | Fetch8();
}

INLINE u16 M6502::ZeroPageAddressing(EightBitRegister* reg)
{
    return ZERO_PAGE_ADDR | ((Fetch8() + reg->GetValue()) & 0xFF);
}

INLINE u16 M6502::ZeroPageRelativeAddressing()
{
    u16 address = ZeroPageAddressing();
    s8 offset = static_cast<s8>(Fetch8());
    return address + offset;
}

INLINE u16 M6502::ZeroPageIndirectAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read((address + 1) & 0x20FF);
    return Address16(h, l);
}

INLINE u16 M6502::ZeroPageIndexedIndirectAddressing()
{
    u16 address = (ZeroPageAddressing() + m_s.X.GetValue()) & 0x20FF;
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read((address + 1) & 0x20FF);
    return Address16(h, l);
}

INLINE u16 M6502::ZeroPageIndirectIndexedAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read((address + 1) & 0x20FF);
    return Address16(h, l) + m_s.Y.GetValue();
}

INLINE s8 M6502::RelativeAddressing()
{
    return static_cast<s8>(Fetch8());
}

INLINE u16 M6502::AbsoluteAddressing()
{
    return Fetch16();
}

INLINE u16 M6502::AbsoluteAddressing(EightBitRegister* reg)
{
    u16 address = Fetch16();
    u16 result = address + reg->GetValue();
    return result;
}

INLINE u16 M6502::AbsoluteIndirectAddressing()
{
    u16 address = Fetch16();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read(address + 1);
    return Address16(h, l);
}

INLINE u16 M6502::AbsoluteIndexedIndirectAddressing()
{
    u16 address = Fetch16() + m_s.X.GetValue();
    u8 l = m_memory->Read(address);
    u8 h = m_memory->Read(address + 1);
    return Address16(h, l);
}

INLINE bool M6502::RunToBreakpointHit()
{
    return m_run_to_breakpoint_hit;
}

INLINE std::vector<M6502::GLYNX_Breakpoint>* M6502::GetBreakpoints()
{
    return &m_breakpoints;
}

INLINE std::stack<M6502::GLYNX_CallStackEntry>* M6502::GetDisassemblerCallStack()
{
    return &m_disassembler_call_stack;
}

INLINE void M6502::PushCallStack(u16 src, u16 dest, u16 back)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    GLYNX_CallStackEntry entry;
    entry.src = src;
    entry.dest = dest;
    entry.back = back;
    if (m_disassembler_call_stack.size() < 256)
        m_disassembler_call_stack.push(entry);
#else
    UNUSED(src);
    UNUSED(dest);
    UNUSED(back);
#endif
}

INLINE void M6502::PopCallStack()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (!m_disassembler_call_stack.empty())
        m_disassembler_call_stack.pop();
#endif
}

INLINE void M6502::CheckBreakpoints()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)

    m_cpu_breakpoint_hit = (m_breakpoints_irq_enabled && m_s.debug_next_irq > 0);
    m_run_to_breakpoint_hit = false;

    if (m_run_to_breakpoint_requested)
    {
        if (m_s.PC.GetValue() == m_run_to_breakpoint.address1)
        {
            m_run_to_breakpoint_hit = true;
            m_run_to_breakpoint_requested = false;
            return;
        }
    }

    if (!m_breakpoints_enabled)
        return;

    for (int i = 0; i < (int)m_breakpoints.size(); i++)
    {
        GLYNX_Breakpoint* brk = &m_breakpoints[i];

        if (!brk->enabled)
            continue;
        if (!brk->execute)
            continue;

        if (brk->range)
        {
            if (m_s.PC.GetValue() >= brk->address1 && m_s.PC.GetValue() <= brk->address2)
            {
                m_cpu_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
        else
        {
            if (m_s.PC.GetValue() == brk->address1)
            {
                m_cpu_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
    }

#endif
}

INLINE void M6502::DisassembleNextOPCode()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)

    CheckBreakpoints();

    u16 address = m_s.PC.GetValue();
    GLYNX_Disassembler_Record* record = m_memory->GetOrCreateDisassemblerRecord(address);

    assert(IsValidPointer(record));

    u8 opcode = m_memory->Read(address);
    u8 opcode_size = k_m6502_opcode_sizes[opcode];

    bool changed = (record->opcodes[0] != opcode);
    record->opcodes[0] = opcode;

    for (int i = 1; i < opcode_size; i++)
    {
        u8 mem_byte = m_memory->Read(address + i);

        if (record->opcodes[i] != mem_byte)
        {
            changed = true;
            record->opcodes[i] = mem_byte;
        }
    }

    if (!changed && record->size != 0)
    {
        if (m_s.debug_next_irq > 0)
        {
            record->irq = m_s.debug_next_irq;
            m_s.debug_next_irq = 0;
        }
        return;
    }

    PopulateDisassemblerRecord(record, opcode, address);
#endif
}

INLINE void M6502::PopulateDisassemblerRecord(GLYNX_Disassembler_Record* record, u8 opcode, u16 address)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    u8 opcode_size = k_m6502_opcode_sizes[opcode];

    record->address = address;
    record->name[0] = 0;
    record->bytes[0] = 0;
    record->segment[0] = 0;
    record->size = opcode_size;
    record->jump = false;
    record->jump_address = 0;
    record->jump_bank = 0;
    record->subroutine = false;
    record->irq = 0;

    if (m_s.debug_next_irq > 0)
    {
        record->irq = m_s.debug_next_irq;
        m_s.debug_next_irq = 0;
    }

    for (int i = 0; i < opcode_size; i++)
    {
        char value[4];
        snprintf(value, 4, "%02X", record->opcodes[i]);
        strncat(record->bytes, value, 24);
        strncat(record->bytes, " ", 24);
    }

    switch (k_m6502_opcode_names[opcode].type)
    {
        case GLYNX_OPCode_Type_Implied:
        {
            snprintf(record->name, 64, "%s", k_m6502_opcode_names[opcode].name);
            break;
        }
        case GLYNX_OPCode_Type_1b:
        {
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, m_memory->Read(address + 1));
            break;
        }
        case GLYNX_OPCode_Type_1b_1b:
        {
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, m_memory->Read(address + 1), m_memory->Read(address + 2));
            break;
        }
        case GLYNX_OPCode_Type_1b_2b:
        {
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, m_memory->Read(address + 1), m_memory->Read(address + 2) | (m_memory->Read(address + 3) << 8));
            break;
        }
        case GLYNX_OPCode_Type_2b:
        {
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, m_memory->Read(address + 1) | (m_memory->Read(address + 2) << 8));
            break;
        }
        case GLYNX_OPCode_Type_2b_2b_2b:
        {
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, m_memory->Read(address + 1) | (m_memory->Read(address + 2) << 8), m_memory->Read(address + 3) | (m_memory->Read(address + 4) << 8), m_memory->Read(address + 5) | (m_memory->Read(address + 6) << 8));
            break;
        }
        case GLYNX_OPCode_Type_1b_Relative:
        {
            s8 rel = m_memory->Read(address + 1);
            u16 jump_address = address + 2 + rel;
            record->jump = true;
            record->jump_address = jump_address;
            //record->jump_bank = m_memory->GetBank(jump_address);
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, jump_address, rel);
            break;
        }
        case GLYNX_OPCode_Type_1b_1b_Relative:
        {
            u8 zero_page = m_memory->Read(address + 1);
            s8 rel = m_memory->Read(address + 2);
            u16 jump_address = address + 3 + rel;
            record->jump = true;
            record->jump_address = jump_address;
            //record->jump_bank = m_memory->GetBank(jump_address);
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, zero_page, jump_address, rel);
            break;
        }
        default:
        {
            break;
        }
    }

    // JMP hhll, JSR hhll
    if (opcode == 0x4C || opcode == 0x20)
    {
        u16 jump_address = Address16(m_memory->Read(address + 2), m_memory->Read(address + 1));
        record->jump = true;
        record->jump_address = jump_address;
    }

    // BSR rr, JSR hhll
    if (opcode == 0x44 || opcode == 0x20)
    {
        record->subroutine = true;
    }

    // if (record->bank < 0xF7)
    // {
    //     strncpy(record->segment, "ROM", 5);
    // }
    // else if (record->bank == 0xF7)
    // {
    //     strncpy(record->segment, "BAT", 5);
    // }
    // else if (record->bank >= 0xF8 && record->bank < 0xFC)
    // {
    //     strncpy(record->segment, "RAM", 5);
    // }
    // else
    // {
    //     strncpy(record->segment, "???", 5);
    // }
#else
    UNUSED(record);
    UNUSED(opcode);
    UNUSED(address);
#endif
}

#endif /* M6502_INLINE_H */
