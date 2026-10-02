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
#include "state_serializer.h"

Bus::Bus()
{
    m_cycles = 0;
    m_suzy_stolen_cycles = 0;
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
    m_suzy_stolen_cycles = 0;
}

void Bus::SaveState(std::ostream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void Bus::LoadState(std::istream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void Bus::Serialize(StateSerializer& s)
{
    G_SERIALIZE(s, m_cycles);
    G_SERIALIZE(s, m_suzy_stolen_cycles);
}
