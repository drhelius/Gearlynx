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
#include "suzy.h"
#include "cartridge.h"
#include "memory.h"
#include "m6502.h"
#include "mikey.h"

Suzy::Suzy(Cartridge* cartridge, M6502* m6502)
{
    m_cartridge = cartridge;
    m_m6502 = m6502;
    InitPointer(m_mikey);
    InitPointer(m_memory);
    InitPointer(m_ram);
    Reset();
}

Suzy::~Suzy()
{
}

void Suzy::Init(Mikey* mikey, Memory* memory)
{
    m_mikey = mikey;
    m_memory = memory;
    m_ram = m_memory->GetRAM();
    Reset();
}

void Suzy::Reset()
{
    m_shift_register_address = 0;
    m_shift_register_current = 0;
    m_shift_register_bit = -1;
    memset(&m_state, 0, sizeof(Suzy_State));

    for (int i = 0; i < 16; ++i)
        m_state.pen_map[i] = i;
}

void Suzy::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Suzy::LoadState(std::istream& stream)
{
    UNUSED(stream);
}
