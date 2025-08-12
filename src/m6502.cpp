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
#include "memory.h"

M6502::M6502()
{
    InitOPCodeFunctors();
    m_breakpoints_enabled = false;
    m_breakpoints_irq_enabled = false;
    m_reset_value = -1;
    m_processor_state.A = &m_A;
    m_processor_state.X = &m_X;
    m_processor_state.Y = &m_Y;
    m_processor_state.S = &m_S;
    m_processor_state.P = &m_P;
    m_processor_state.PC = &m_PC;
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
    m_PC.SetLow(m_memory->Read(0xFFFC));
    m_PC.SetHigh(m_memory->Read(0xFFFD));
    m_debug_next_irq = 1;
    DisassembleNextOPCode();

    if (m_reset_value < 0)
    {
        m_A.SetValue(rand() & 0xFF);
        m_X.SetValue(rand() & 0xFF);
        m_Y.SetValue(rand() & 0xFF);
        m_S.SetValue(rand() & 0xFF);
        m_P.SetValue(rand() & 0xFF);
    }
    else
    {
        m_A.SetValue(m_reset_value & 0xFF);
        m_X.SetValue(m_reset_value & 0xFF);
        m_Y.SetValue(m_reset_value & 0xFF);
        m_S.SetValue(m_reset_value & 0xFF);
        m_P.SetValue(m_reset_value & 0xFF);
    }

    SetFlag(FLAG_UNUSED);
    ClearFlag(FLAG_DECIMAL);
    SetFlag(FLAG_INTERRUPT);
    ClearFlag(FLAG_BREAK);
    m_cycles = 0;
    m_irq_pending = 0;
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

void M6502::SetResetValue(int value)
{
    m_reset_value = value;
}

void M6502::EnableBreakpoints(bool enable, bool irqs)
{
    m_breakpoints_enabled = enable;
    m_breakpoints_irq_enabled = irqs;
}

bool M6502::BreakpointHit()
{
    return (m_cpu_breakpoint_hit || m_memory_breakpoint_hit);
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

    if ((input_len == 9) && (text[4] == '-'))
    {
        // format: AAAA-BBBB
        if (parseHexString(text, 4, &brk.address1) && 
            parseHexString(text + 5, 4, &brk.address2))
        {
            brk.range = true;
        }
        else
        {
            return false;
        }
    }
    else if ((input_len > 0) && (input_len <= 4))
    {
        // format: AAAA
        if (!parseHexString(text, input_len, &brk.address1))
        {
            return false;
        }
    }
    else
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

void M6502::ClearDisassemblerCallStack()
{
    while(!m_disassembler_call_stack.empty())
        m_disassembler_call_stack.pop();
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
    UNUSED(type);
    UNUSED(address);
    UNUSED(read);
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
    stream.write(reinterpret_cast<const char*> (&m_irq_pending), sizeof(m_irq_pending));
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
    stream.read(reinterpret_cast<char*> (&m_irq_pending), sizeof(m_irq_pending));
    stream.read(reinterpret_cast<char*> (&m_debug_next_irq), sizeof(m_debug_next_irq));
}
