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

static const u32 k_bus_cycles_opcode_page   = 5;    // Page mode opcode fetch
static const u32 k_bus_cycles_opcode_normal = 5;    // Normal opcode fetch
static const u32 k_bus_cycles_ram_read      = 5;    // RAM/ROM data read
static const u32 k_bus_cycles_ram_write     = 5;    // RAM/ROM data write
static const u32 k_bus_cycles_suzy_read     = 11;   // Suzy register read (midpoint 9-15)
static const u32 k_bus_cycles_suzy_write    = 5;    // Suzy register write (blind write)
static const u32 k_bus_cycles_cart_read     = 14;   // Cart read
static const u32 k_bus_cycles_mikey_read    = 5;    // Mikey register read
static const u32 k_bus_cycles_mikey_write   = 5;    // Mikey register write
static const u32 k_bus_cycles_audio_ram_rw = 12;    // Audio DPRAM (midpoint 5-20)
static const u32 k_bus_cycles_int_tick_factor = 2;  // Internal CPU cycle to tick scaling

#endif /* BUS_H */
