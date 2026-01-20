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

#ifndef BUS_H
#define BUS_H

#include "common.h"

class Bus
{
public:
    Bus();
    ~Bus();
    void Init();
    void Reset();
    void InjectCycles(u32 cycles);
    u32 ConsumeCycles();

private:
    u32 m_cycles;
};

static const u32 k_bus_cycles_suzy_read         = 3;    // Suzy register read
static const u32 k_bus_cycles_suzy_write        = 0;    // Suzy register write
static const u32 k_bus_cycles_cart_read         = 3;    // Cart read
static const u32 k_bus_cycles_mikey_read        = 0;    // Mikey register read
static const u32 k_bus_cycles_mikey_write       = 0;    // Mikey register write
static const u32 k_bus_cycles_int_tick_factor   = 5;    // Internal CPU cycle to tick scaling
static const u32 k_bus_cycles_timer             = 7;
static const u32 k_bus_cycles_audio             = 7;

#endif /* BUS_H */
