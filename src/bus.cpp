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

#include "bus.h"

Bus::Bus()
{
    m_cycles = 0;
}

Bus::~Bus()
{
}

void Bus::Init()
{
    Reset();
}

void Bus::Reset()
{
    m_cycles = 0;
}

void Bus::InjectCycles(u32 cycles)
{
    m_cycles += cycles;
}

u32 Bus::ConsumeCycles()
{
    u32 ret = m_cycles;
    m_cycles = 0;
    return ret;
}
