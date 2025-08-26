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

#include <istream>
#include <ostream>
#include "mikey.h"
#include "memory.h"
#include "suzy.h"

Mikey::Mikey(Cartridge* cartridge, M6502* m6502)
{
    m_cartridge = cartridge;
    m_m6502 = m6502;
    InitPointer(m_suzy);
    InitPointer(m_memory);
    Reset();
}

Mikey::~Mikey()
{
}

void Mikey::Init(Suzy* suzy, Memory* memory)
{
    m_suzy = suzy;
    m_memory = memory;
    InitTimers();
    Reset();
}

void Mikey::Reset()
{
    memset(&m_state, 0, sizeof(Mikey_State));
    memset(m_palette, 0, sizeof(m_palette));
}

void Mikey::InitTimers()
{
    m_state.timers[0].internal_linked_to = -1;
    m_state.timers[2].internal_linked_to = 0;
    m_state.timers[4].internal_linked_to = 2;

    m_state.timers[1].internal_linked_to = -1;
    m_state.timers[3].internal_linked_to = 1;
    m_state.timers[5].internal_linked_to = 3;
    m_state.timers[7].internal_linked_to = 5;

    m_state.timers[6].internal_linked_to = -1;

    for (int i = 0; i < 8; i++)
    {
        m_state.timers[i].internal_period_cycles = k_mikey_timer_period_cycles[0];
    }
}

void Mikey::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Mikey::LoadState(std::istream& stream)
{
    UNUSED(stream);
}