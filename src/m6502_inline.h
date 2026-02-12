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
#include "bus.h"

INLINE u32 M6502::RunInstruction()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_memory_breakpoint_hit = false;
    m_cpu_breakpoint_hit = false;
#endif

    m_s.cycles = 0;
    m_s.memory_accesses = 0;
    m_s.onebyte_un_nop = false;
    m_s.page_mode_discounts = 0;

    if (unlikely(m_s.halted))
    {
        if (m_s.irq_asserted)
        {
            m_s.halted = false;
            CheckIRQs();
            if(m_s.irq_pending && !m_skip_irq_on_step)
                HandleIRQ();
        }
        else
            return 8;
    }
    else
    {
        u8 opcode = FetchOpcode8();

        CheckIRQs();
        (this->*m_opcodes[opcode])();

        if(m_s.irq_pending && !m_s.onebyte_un_nop && !m_skip_irq_on_step)
            HandleIRQ();

        DisassembleNextOPCode();

        m_s.cycles += m_opcode_cycles[opcode];
    }

    u32 ticks = (m_s.cycles * k_bus_cycles_int_tick_factor) - (u32)m_s.page_mode_discounts;

    m_s.last_ticks = ticks;
    m_s.total_ticks += ticks;

    return ticks;
}

inline void M6502::HandleIRQ()
{
    u16 pc = m_s.PC.GetValue();
    StackPush16(pc);
    StackPush8(m_s.P.GetValue() & ~FLAG_BREAK);
    SetFlag(FLAG_INTERRUPT);
    ClearFlag(FLAG_DECIMAL);

    m_s.PC.SetLow(MemRead8(0xFFFE));
    m_s.PC.SetHigh(MemRead8(0xFFFF));

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

INLINE void M6502::AssertIRQ(bool asserted, u8 irq_mask)
{
    m_s.irq_asserted = asserted;
    m_s.debug_irq_mask = irq_mask;
}

INLINE void M6502::Halt(bool halted)
{
    m_s.halted = halted;
}

INLINE bool M6502::IsHalted()
{
    return m_s.halted;
}

INLINE void M6502::InjectCycles(unsigned int cycles)
{
    m_s.cycles += cycles;
}

INLINE void M6502::SetPageModeEnabled(bool enabled)
{
    m_page_mode_tick_discount = enabled ? 1 : 0;
}

INLINE void M6502::SetSkipIRQOnStep(bool skip)
{
    m_skip_irq_on_step = skip;
}

INLINE u8 M6502::FetchOpcode8()
{
    const u16 addr = m_s.PC.GetValue();

    bool page_mode = m_stream_open && ((addr & 0x0F) != 0);

    u8 value = m_memory->Read(addr);
    m_s.PC.Increment();

    m_stream_open = true;

    if (page_mode)
        m_s.page_mode_discounts += m_page_mode_tick_discount;

    return value;
}

INLINE u8 M6502::FetchOperand8()
{
    const u16 addr = m_s.PC.GetValue();

    bool page_mode = m_stream_open && ((addr & 0x0F) != 0);

    u8 value = m_memory->Read(addr);
    m_s.PC.Increment();

    if (page_mode)
        m_s.page_mode_discounts += m_page_mode_tick_discount;

    return value;
}

INLINE u16 M6502::FetchOperand16()
{
    const u16 addr = m_s.PC.GetValue();

    u8 discounts = 0;
    if (m_stream_open)
    {
        if ((addr & 0x0F) != 0)
            discounts++;
        if (((addr + 1) & 0x0F) != 0)
            discounts++;
    }

    const u8 l = m_memory->Read(addr);
    const u8 h = m_memory->Read(addr + 1);
    m_s.PC.SetValue(addr + 2);

    m_s.page_mode_discounts += discounts * m_page_mode_tick_discount;

    return (static_cast<u16>(h) << 8) | l;
}

INLINE void M6502::NotifyBusBreak()
{
    m_stream_open = false;
}

INLINE u8 M6502::MemRead8(u16 address)
{
    m_stream_open = false;
    return m_memory->Read(address);
}

INLINE void M6502::MemWrite8(u16 address, u8 value)
{
    m_stream_open = false;
    m_memory->Write(address, value);
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
    MemWrite8(STACK_ADDR | m_s.S.GetValue(), static_cast<u8>(value >> 8));
    m_s.S.Decrement();
    MemWrite8(STACK_ADDR | m_s.S.GetValue(), static_cast<u8>(value & 0x00FF));
    m_s.S.Decrement();
}

INLINE void M6502::StackPush8(u8 value)
{
    MemWrite8(STACK_ADDR | m_s.S.GetValue(), value);
    m_s.S.Decrement();
}

INLINE u16 M6502::StackPop16()
{
    m_s.S.Increment();
    u8 l = MemRead8(STACK_ADDR | m_s.S.GetValue());
    m_s.S.Increment();
    u8 h = MemRead8(STACK_ADDR | m_s.S.GetValue());
    return Address16(h, l);
}

INLINE u8 M6502::StackPop8()
{
    m_s.S.Increment();
    return MemRead8(STACK_ADDR | m_s.S.GetValue());
}

INLINE u8 M6502::ImmediateAddressing()
{
    return FetchOperand8();
}

INLINE u16 M6502::ZeroPageAddressing()
{
    return ZERO_PAGE_ADDR | FetchOperand8();
}

INLINE u16 M6502::ZeroPageAddressing(EightBitRegister* reg)
{
    return ZERO_PAGE_ADDR | ((FetchOperand8() + reg->GetValue()) & 0xFF);
}

INLINE u16 M6502::ZeroPageRelativeAddressing()
{
    u16 address = ZeroPageAddressing();
    s8 offset = static_cast<s8>(FetchOperand8());
    return address + offset;
}

INLINE u16 M6502::ZeroPageIndirectAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = MemRead8(address);
    u8 h = MemRead8((address + 1) & 0x00FF);
    return Address16(h, l);
}

INLINE u16 M6502::ZeroPageIndexedIndirectAddressing()
{
    u16 address = (ZeroPageAddressing() + m_s.X.GetValue()) & 0x20FF;
    u8 l = MemRead8(address);
    u8 h = MemRead8((address + 1) & 0x20FF);
    return Address16(h, l);
}

INLINE u16 M6502::ZeroPageIndirectIndexedAddressing()
{
    u16 address = ZeroPageAddressing();
    u8 l = MemRead8(address);
    u8 h = MemRead8((address + 1) & 0x20FF);
    return Address16(h, l) + m_s.Y.GetValue();
}

INLINE s8 M6502::RelativeAddressing()
{
    return static_cast<s8>(FetchOperand8());
}

INLINE u16 M6502::AbsoluteAddressing()
{
    return FetchOperand16();
}

INLINE u16 M6502::AbsoluteAddressing(EightBitRegister* reg)
{
    u16 address = FetchOperand16();
    u16 result = address + reg->GetValue();
    return result;
}

INLINE u16 M6502::AbsoluteIndirectAddressing()
{
    u16 address = FetchOperand16();
    u8 l = MemRead8(address);
    u8 h = MemRead8(address + 1);
    return Address16(h, l);
}

INLINE u16 M6502::AbsoluteIndexedIndirectAddressing()
{
    u16 address = FetchOperand16() + m_s.X.GetValue();
    u8 l = MemRead8(address);
    u8 h = MemRead8(address + 1);
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
    if (m_disassembler_call_stack_size < 256)
    {
        GLYNX_CallStackEntry entry;
        entry.src = src;
        entry.dest = dest;
        entry.back = back;
        m_disassembler_call_stack.push(entry);
        m_disassembler_call_stack_size++;
    }
#else
    UNUSED(src);
    UNUSED(dest);
    UNUSED(back);
#endif
}

INLINE void M6502::PopCallStack()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (m_disassembler_call_stack_size > 0)
    {
        m_disassembler_call_stack.pop();
        m_disassembler_call_stack_size--;
    }
#endif
}

INLINE void M6502::CheckBreakpoints()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)

    bool irq_hit = (m_s.irq_asserted && ((m_s.debug_irq_mask & m_breakpoints_irq_enabled) != 0));

    m_cpu_breakpoint_hit = (irq_hit && m_s.debug_next_irq == 3);
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

    u8 opcode = m_memory->Read<true>(address);
    u8 opcode_size = m_opcode_sizes[opcode];

    bool changed = (record->opcodes[0] != opcode);
    record->opcodes[0] = opcode;

    for (int i = 1; i < opcode_size; i++)
    {
        u8 mem_byte = m_memory->Read<true>(address + i);

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

    InvalidateOverlappingRecords(address, opcode_size);

    PopulateDisassemblerRecord(record, opcode, address);
#endif
}

INLINE void M6502::InvalidateOverlappingRecords(u16 address, u8 opcode_size)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)

    for (int back = 1; back < 3; ++back)
    {
        int prev_start = (int)address - back;
        if (prev_start < 0)
            continue;

        GLYNX_Disassembler_Record* prev = m_memory->GetDisassemblerRecord(prev_start);
        if (!IsValidPointer(prev) || prev->size == 0)
            continue;

        int distance = address - prev_start;
        if (prev->size > distance)
        {
            prev->size = 0;
            prev->name[0] = 0;
            prev->bytes[0] = 0;
        }
    }

    if (opcode_size > 1)
    {
        for (int fwd = 1; fwd < opcode_size; ++fwd)
        {
            u16 fwd_addr = address + fwd;
            GLYNX_Disassembler_Record* fwd_record = m_memory->GetDisassemblerRecord(fwd_addr);
            if (!IsValidPointer(fwd_record) || fwd_record->size == 0)
                continue;

            fwd_record->size = 0;
            fwd_record->name[0] = 0;
            fwd_record->bytes[0] = 0;
        }
    }
#else
    UNUSED(address);
    UNUSED(opcode_size);
#endif
}

INLINE void M6502::PopulateDisassemblerRecord(GLYNX_Disassembler_Record* record, u8 opcode, u16 address)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    u8 opcode_size = m_opcode_sizes[opcode];

    record->address = address;
    record->rom = (address >= 0xFE00) && IS_NOT_SET_BIT(m_memory->GetState()->MAPCTL, 2);
    record->name[0] = 0;
    record->bytes[0] = 0;
    record->segment[0] = 0;
    record->size = opcode_size;
    record->jump = false;
    record->jump_address = 0;
    record->jump_bank = 0;
    record->subroutine = false;
    record->irq = 0;
    record->has_operand_address = false;
    record->operand_address = 0;
    record->operand_is_zp = false;

    if (m_s.debug_next_irq > 0)
    {
        record->irq = m_s.debug_next_irq;
        m_s.debug_next_irq = 0;
    }

    int pos = 0;
    for (int i = 0; i < opcode_size; i++)
    {
        static const char hex_chars[] = "0123456789ABCDEF";
        u8 byte = record->opcodes[i];
        record->bytes[pos++] = hex_chars[byte >> 4];
        record->bytes[pos++] = hex_chars[byte & 0x0F];
        record->bytes[pos++] = ' ';
    }
    record->bytes[pos] = 0;

    u8 op1 = record->opcodes[1];
    u8 op2 = record->opcodes[2];

    switch (k_m6502_opcode_names[opcode].type)
    {
        case GLYNX_OPCode_Type_Implied:
        {
            snprintf(record->name, 64, "%s", k_m6502_opcode_names[opcode].name);
            break;
        }
        case GLYNX_OPCode_Type_1b:
        {
            if (!strstr(k_m6502_opcode_names[opcode].name, "#$"))
            {
                record->has_operand_address = true;
                record->operand_address = op1;
                record->operand_is_zp = true;
            }
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, op1);
            break;
        }
        case GLYNX_OPCode_Type_1b_1b:
        {
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, op1, op2);
            break;
        }
        case GLYNX_OPCode_Type_1b_2b:
        {
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, op1, op2 | (m_memory->Read<true>(address + 3) << 8));
            break;
        }
        case GLYNX_OPCode_Type_2b:
        {
            u16 operand = op1 | (op2 << 8);
            record->has_operand_address = true;
            record->operand_address = operand;
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, operand);
            break;
        }
        case GLYNX_OPCode_Type_2b_2b_2b:
        {
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, op1 | (op2 << 8), m_memory->Read<true>(address + 3) | (m_memory->Read<true>(address + 4) << 8), m_memory->Read<true>(address + 5) | (m_memory->Read<true>(address + 6) << 8));
            break;
        }
        case GLYNX_OPCode_Type_1b_Relative:
        {
            s8 rel = (s8)op1;
            u16 jump_address = address + 2 + rel;
            record->jump = true;
            record->jump_address = jump_address;
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, jump_address, rel);
            break;
        }
        case GLYNX_OPCode_Type_1b_1b_Relative:
        {
            s8 rel = (s8)op2;
            u16 jump_address = address + 3 + rel;
            record->jump = true;
            record->jump_address = jump_address;
            snprintf(record->name, 64, k_m6502_opcode_names[opcode].name, op1, jump_address, rel);
            break;
        }
        default:
        {
            break;
        }
    }

    // JMP $nn, JSR $nn
    if (opcode == 0x4C || opcode == 0x20)
    {
        u16 jump_address = Address16(record->opcodes[2], record->opcodes[1]);
        record->jump = true;
        record->jump_address = jump_address;
    }

    // JSR $nn
    if (opcode == 0x20)
    {
        record->subroutine = true;
    }

    if (record->rom)
        strncpy(record->segment, "BIOS", 5);
    else
        strncpy(record->segment, "RAM ", 5);

#else
    UNUSED(record);
    UNUSED(opcode);
    UNUSED(address);
#endif
}

INLINE void M6502::DisassembleAhead(int count)
{
    DisassembleAhead(m_s.PC.GetValue(), count, 0);
}

inline void M6502::DisassembleAhead(u16 start_address, int count, int depth)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (depth > 3)
        return;

    u16 address = start_address;
    int disassembled = 0;

    while (disassembled < count && address < 0xFFFF)
    {
        GLYNX_Disassembler_Record* record = m_memory->GetOrCreateDisassemblerRecord(address);

        if (!IsValidPointer(record))
            break;

        u8 opcode = m_memory->Read<true>(address);
        u8 opcode_size = m_opcode_sizes[opcode];

        if ((u32)address + opcode_size > 0xFFFF)
            break;

        bool changed = (record->opcodes[0] != opcode);
        record->opcodes[0] = opcode;

        for (int i = 1; i < opcode_size; i++)
        {
            u8 mem_byte = m_memory->Read<true>(address + i);
            if (record->opcodes[i] != mem_byte)
            {
                changed = true;
                record->opcodes[i] = mem_byte;
            }
        }

        if (changed || record->size == 0)
        {
            InvalidateOverlappingRecords(address, opcode_size);

            int saved_irq = m_s.debug_next_irq;
            m_s.debug_next_irq = 0;
            PopulateDisassemblerRecord(record, opcode, address);
            m_s.debug_next_irq = saved_irq;
        }

        if (record->jump && record->jump_address != 0)
        {
            DisassembleAhead(record->jump_address, count / 2, depth + 1);
        }

        address += opcode_size;
        disassembled++;

        // Stop at unconditional control flow (end of block)
        // 0x40=RTI, 0x4C=JMP abs, 0x60=RTS, 0x6C=JMP (ind), 0x7C=JMP (abs,X), 0x80=BRA
        if (opcode == 0x40 || opcode == 0x4C || opcode == 0x60 || opcode == 0x6C || opcode == 0x7C || opcode == 0x80)
            break;
    }
#else
    UNUSED(start_address);
    UNUSED(count);
    UNUSED(depth);
#endif
}

#endif /* M6502_INLINE_H */
