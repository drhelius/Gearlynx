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

#ifndef BUS_INLINE_H
#define BUS_INLINE_H

#include "bus.h"

INLINE void Bus::InjectCycles(u32 cycles)
{
    m_cycles += cycles;
}

INLINE void Bus::InjectSuzyStolenCycles(u32 cycles)
{
    m_suzy_stolen_cycles += cycles;
}

INLINE u32 Bus::GetCycles() const
{
    return m_cycles;
}

INLINE u32 Bus::ConsumeCycles()
{
    u32 ret = m_cycles;
    m_cycles = 0;
    return ret;
}

INLINE u32 Bus::ConsumeSuzyStolenCycles()
{
    u32 ret = m_suzy_stolen_cycles;
    m_suzy_stolen_cycles = 0;
    return ret;
}

#endif /* BUS_INLINE_H */