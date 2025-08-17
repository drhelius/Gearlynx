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

Mikey::Mikey(Cartridge* cartridge, M6502* m6502)
{
    m_cartridge = cartridge;
    m_m6502 = m6502;
    InitPointer(m_memory);
}

Mikey::~Mikey()
{
}

void Mikey::Init(Memory* memory)
{
    m_memory = memory;
    Reset();
}

void Mikey::Reset()
{
}

void Mikey::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Mikey::LoadState(std::istream& stream)
{
    UNUSED(stream);
}