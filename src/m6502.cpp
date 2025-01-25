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

#include <string>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "m6502.h"
#include "m6502_timing.h"
#include "m6502_names.h"
#include "memory.h"

M6502::M6502()
{
    InitOPCodeFunctors();
    m_breakpoints_enabled = false;
    m_breakpoints_irq_enabled = false;
    m_processor_state.A = &m_A;
    m_processor_state.X = &m_X;
    m_processor_state.Y = &m_Y;
    m_processor_state.S = &m_S;
    m_processor_state.P = &m_P;
    m_processor_state.PC = &m_PC;
    m_processor_state.SPEED = &m_speed;
    m_processor_state.TIMER = &m_timer_enabled;
    m_processor_state.TIMER_COUNTER = &m_timer_counter;
    m_processor_state.TIMER_RELOAD = &m_timer_reload;
    m_processor_state.IDR = &m_interrupt_disable_register;
    m_processor_state.IRR = &m_interrupt_request_register;
    m_processor_state.CYCLES = &m_last_instruction_cycles;
}

M6502::~M6502()
{
}

void M6502::Init(Memory* memory)
{
    m_memory = memory;
    CreateZNFlagsTable();
}

void M6502::Reset()
{
    m_PC.SetLow(m_memory->Read(0xFFFE));
    m_PC.SetHigh(m_memory->Read(0xFFFF));
    m_debug_next_irq = 1;
    DisassembleNextOPCode();
    m_A.SetValue(rand() & 0xFF);
    m_X.SetValue(rand() & 0xFF);
    m_Y.SetValue(rand() & 0xFF);
    m_S.SetValue(rand() & 0xFF);
    m_P.SetValue(rand() & 0xFF);
    ClearFlag(FLAG_TRANSFER);
    ClearFlag(FLAG_DECIMAL);
    SetFlag(FLAG_INTERRUPT);
    ClearFlag(FLAG_BREAK);
    m_cycles = 0;
    m_clock = 0;
    m_clock_cycles = 0;
    m_last_instruction_cycles = 0;
    m_irq_pending = 0;
    m_speed = 0;
    m_transfer = false;
    m_timer_cycles = 0;
    m_timer_enabled = false;
    m_timer_counter = 0;
    m_timer_reload = 0;
    m_interrupt_disable_register = 0;
    m_interrupt_request_register = 0;
    m_skip_flag_transfer_clear = false;
    m_cpu_breakpoint_hit = false;
    m_memory_breakpoint_hit = false;
    m_run_to_breakpoint_hit = false;
    m_run_to_breakpoint_requested = false;
    ClearDisassemblerCallStack();
}

M6502::M6502_State* M6502::GetState()
{
    return &m_processor_state;
}

void M6502::DisassembleNextOPCode()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)

    CheckBreakpoints();

    u16 address = m_PC.GetValue();
    Memory::GLYNX_Disassembler_Record* record = m_memory->GetOrCreateDisassemblerRecord(address);

    if (!IsValidPointer(record))
    {
        return;
    }

    u8 opcode = m_memory->Read(address);
    u8 opcode_size = k_m6502_opcode_sizes[opcode];

    bool changed = false;

    for (int i = 0; i < opcode_size; i++)
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
        if (m_debug_next_irq > 0)
        {
            record->irq = m_debug_next_irq;
            m_debug_next_irq = 0;
        }
        return;
    }

    record->size = opcode_size;
    // TODO: implement disassembler
    record->address = address;//m_memory->GetPhysicalAddress(address);
    record->bank = 0;//m_memory->GetBank(address);
    record->name[0] = 0;
    record->bytes[0] = 0;
    record->jump = false;
    record->jump_address = 0;
    record->jump_bank = 0;
    record->subroutine = false;
    record->irq = 0;

    if (m_debug_next_irq > 0)
    {
        record->irq = m_debug_next_irq;
        m_debug_next_irq = 0;
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
            //TODO: implement disassembler
            record->jump_bank = 0;//m_memory->GetBank(jump_address);
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
            //TODO: implement disassembler
            record->jump_bank = 0;//m_memory->GetBank(jump_address);
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
        //TODO: implement disassembler
        record->jump_bank = 0;//m_memory->GetBank(jump_address);
    }

    // BSR rr, JSR hhll
    if (opcode == 0x44 || opcode == 0x20)
    {
        record->subroutine = true;
    }

    if (record->bank < 0xF7)
    {
        strncpy(record->segment, "ROM", 5);
    }
    else if (record->bank == 0xF7)
    {
        strncpy(record->segment, "BAT", 5);
    }
    else if (record->bank >= 0xF8 && record->bank < 0xFC)
    {
        strncpy(record->segment, "RAM", 5);
    }
    else
    {
        strncpy(record->segment, "???", 5);
    }
#endif
}

void M6502::EnableBreakpoints(bool enable, bool irqs)
{
    m_breakpoints_enabled = enable;
    m_breakpoints_irq_enabled = irqs;
}

bool M6502::BreakpointHit()
{
    return (m_cpu_breakpoint_hit || m_memory_breakpoint_hit) && (m_clock_cycles == 0);
}

bool M6502::RunToBreakpointHit()
{
    return m_run_to_breakpoint_hit && (m_clock_cycles == 0);
}

void M6502::ResetBreakpoints()
{
    m_breakpoints.clear();
}

bool M6502::AddBreakpoint(int type, char* text, bool read, bool write, bool execute)
{
    int input_len = (int)strlen(text);
    GLYNX_Breakpoint brk;
    brk.enabled = true;
    brk.type = type;
    brk.address1 = 0;
    brk.address2 = 0;
    brk.range = false;
    brk.read = read;
    brk.write = write;
    brk.execute = execute;

    if (!read && !write && !execute)
        return false;

    try
    {
        if ((input_len == 9) && (text[4] == '-'))
        {
            std::string str(text);
            std::size_t separator = str.find("-");

            if (separator != std::string::npos)
            {
                brk.address1 = (u16)std::stoul(str.substr(0, separator), 0 , 16);
                brk.address2 = (u16)std::stoul(str.substr(separator + 1 , std::string::npos), 0, 16);
                brk.range = true;
            }
        }
        else if ((input_len > 0) && (input_len <= 4))
        {
            brk.address1 = (u16)std::stoul(text, 0, 16);
        }
        else
        {
            return false;
        }
    }
    catch(const std::invalid_argument&)
    {
        return false;
    }

    bool found = false;

    for (long unsigned int b = 0; b < m_breakpoints.size(); b++)
    {
        GLYNX_Breakpoint* item = &m_breakpoints[b];

        if (item->type != brk.type)
            continue;

        if (brk.range)
        {
            if (item->range && (item->address1 == brk.address1) && (item->address2 == brk.address2))
            {
                found = true;
                break;
            }
        }
        else
        {
            if (!item->range && (item->address1 == brk.address1))
            {
                found = true;
                break;
            }
        }
    }

    if (!found)
        m_breakpoints.push_back(brk);

    return true;
}

bool M6502::AddBreakpoint(u16 address)
{
    char text[6];
    snprintf(text, 6, "%04X", address);
    return AddBreakpoint(M6502_BREAKPOINT_TYPE_ROMRAM, text, false, false, true);
}

void M6502::AddRunToBreakpoint(u16 address)
{
    m_run_to_breakpoint.enabled = true;
    m_run_to_breakpoint.type = M6502_BREAKPOINT_TYPE_ROMRAM;
    m_run_to_breakpoint.address1 = address;
    m_run_to_breakpoint.address2 = 0;
    m_run_to_breakpoint.range = false;
    m_run_to_breakpoint.read = false;
    m_run_to_breakpoint.write = false;
    m_run_to_breakpoint.execute = true;
    m_run_to_breakpoint_requested = true;
}

void M6502::RemoveBreakpoint(int type, u16 address)
{
    for (long unsigned int b = 0; b < m_breakpoints.size(); b++)
    {
        GLYNX_Breakpoint* item = &m_breakpoints[b];

        if (!item->range && (item->address1 == address) && (item->type == type))
        {
            m_breakpoints.erase(m_breakpoints.begin() + b);
            break;
        }
    }
}

bool M6502::IsBreakpoint(int type, u16 address)
{
    for (long unsigned int b = 0; b < m_breakpoints.size(); b++)
    {
        GLYNX_Breakpoint* item = &m_breakpoints[b];

        if (!item->range && (item->address1 == address) && (item->type == type))
        {
            return true;
        }
    }

    return false;
}

std::vector<M6502::GLYNX_Breakpoint>* M6502::GetBreakpoints()
{
    return &m_breakpoints;
}

void M6502::ClearDisassemblerCallStack()
{
    while(!m_disassembler_call_stack.empty())
        m_disassembler_call_stack.pop();
}

std::stack<M6502::GLYNX_CallStackEntry>* M6502::GetDisassemblerCallStack()
{
    return &m_disassembler_call_stack;
}

void M6502::CheckMemoryBreakpoints(int type, u16 address, bool read)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)

    if (!m_breakpoints_enabled)
        return;

    for (int i = 0; i < (int)m_breakpoints.size(); i++)
    {
        GLYNX_Breakpoint* brk = &m_breakpoints[i];

        if (!brk->enabled)
            continue;
        if (brk->type != type)
            continue;
        if (read && !brk->read)
            continue;
        if (!read && !brk->write)
            continue;

        if (brk->range)
        {
            if (address >= brk->address1 && address <= brk->address2)
            {
                m_memory_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
        else
        {
            if (address == brk->address1)
            {
                m_memory_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
    }
#else
    UNUSED(address);
    UNUSED(read);
#endif
}

void M6502::CheckBreakpoints()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)

    m_cpu_breakpoint_hit = false;
    m_run_to_breakpoint_hit = false;

    if (m_run_to_breakpoint_requested)
    {
        if (m_PC.GetValue() == m_run_to_breakpoint.address1)
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
        if (brk->type != M6502_BREAKPOINT_TYPE_ROMRAM)
            continue;

        if (brk->range)
        {
            if (m_PC.GetValue() >= brk->address1 && m_PC.GetValue() <= brk->address2)
            {
                m_cpu_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
        else
        {
            if (m_PC.GetValue() == brk->address1)
            {
                m_cpu_breakpoint_hit = true;
                m_run_to_breakpoint_requested = false;
                return;
            }
        }
    }

#endif
}

void M6502::PushCallStack(u16 src, u16 dest, u16 back)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    GLYNX_CallStackEntry entry;
    entry.src = src;
    entry.dest = dest;
    entry.back = back;
    if (m_disassembler_call_stack.size() < 256)
        m_disassembler_call_stack.push(entry);
    // else
    // {
    //     //Debug("** M6502 --> Disassembler Call Stack Overflow");
    // }
#endif
}

void M6502::PopCallStack()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (!m_disassembler_call_stack.empty())
        m_disassembler_call_stack.pop();
#endif
}

void M6502::CreateZNFlagsTable()
{
    for (int i = 0; i < 256; i++)
    {
        m_zn_flags_lut[i] = 0;

        if (i == 0)
            m_zn_flags_lut[i] |= FLAG_ZERO;
        if (i & 0x80)
            m_zn_flags_lut[i] |= FLAG_NEGATIVE;
    }
}

void M6502::SaveState(std::ostream& stream)
{
    m_PC.SaveState(stream);
    m_A.SaveState(stream);
    m_X.SaveState(stream);
    m_Y.SaveState(stream);
    m_S.SaveState(stream);
    m_P.SaveState(stream);

    stream.write(reinterpret_cast<const char*> (&m_cycles), sizeof(m_cycles));
    stream.write(reinterpret_cast<const char*> (&m_clock), sizeof(m_clock));
    stream.write(reinterpret_cast<const char*> (&m_clock_cycles), sizeof(m_clock_cycles));
    stream.write(reinterpret_cast<const char*> (&m_last_instruction_cycles), sizeof(m_last_instruction_cycles));
    stream.write(reinterpret_cast<const char*> (&m_irq_pending), sizeof(m_irq_pending));
    stream.write(reinterpret_cast<const char*> (&m_speed), sizeof(m_speed));
    stream.write(reinterpret_cast<const char*> (&m_transfer), sizeof(m_transfer));
    stream.write(reinterpret_cast<const char*> (&m_timer_enabled), sizeof(m_timer_enabled));
    stream.write(reinterpret_cast<const char*> (&m_timer_cycles), sizeof(m_timer_cycles));
    stream.write(reinterpret_cast<const char*> (&m_timer_counter), sizeof(m_timer_counter));
    stream.write(reinterpret_cast<const char*> (&m_timer_reload), sizeof(m_timer_reload));
    stream.write(reinterpret_cast<const char*> (&m_interrupt_disable_register), sizeof(m_interrupt_disable_register));
    stream.write(reinterpret_cast<const char*> (&m_interrupt_request_register), sizeof(m_interrupt_request_register));
    stream.write(reinterpret_cast<const char*> (&m_skip_flag_transfer_clear), sizeof(m_skip_flag_transfer_clear));
    stream.write(reinterpret_cast<const char*> (&m_debug_next_irq), sizeof(m_debug_next_irq));
}

void M6502::LoadState(std::istream& stream)
{
    m_PC.LoadState(stream);
    m_A.LoadState(stream);
    m_X.LoadState(stream);
    m_Y.LoadState(stream);
    m_S.LoadState(stream);
    m_P.LoadState(stream);

    stream.read(reinterpret_cast<char*> (&m_cycles), sizeof(m_cycles));
    stream.read(reinterpret_cast<char*> (&m_clock), sizeof(m_clock));
    stream.read(reinterpret_cast<char*> (&m_clock_cycles), sizeof(m_clock_cycles));
    stream.read(reinterpret_cast<char*> (&m_last_instruction_cycles), sizeof(m_last_instruction_cycles));
    stream.read(reinterpret_cast<char*> (&m_irq_pending), sizeof(m_irq_pending));
    stream.read(reinterpret_cast<char*> (&m_speed), sizeof(m_speed));
    stream.read(reinterpret_cast<char*> (&m_transfer), sizeof(m_transfer));
    stream.read(reinterpret_cast<char*> (&m_timer_enabled), sizeof(m_timer_enabled));
    stream.read(reinterpret_cast<char*> (&m_timer_cycles), sizeof(m_timer_cycles));
    stream.read(reinterpret_cast<char*> (&m_timer_counter), sizeof(m_timer_counter));
    stream.read(reinterpret_cast<char*> (&m_timer_reload), sizeof(m_timer_reload));
    stream.read(reinterpret_cast<char*> (&m_interrupt_disable_register), sizeof(m_interrupt_disable_register));
    stream.read(reinterpret_cast<char*> (&m_interrupt_request_register), sizeof(m_interrupt_request_register));
    stream.read(reinterpret_cast<char*> (&m_skip_flag_transfer_clear), sizeof(m_skip_flag_transfer_clear));
    stream.read(reinterpret_cast<char*> (&m_debug_next_irq), sizeof(m_debug_next_irq));
}