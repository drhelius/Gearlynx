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

#ifndef COMLYNX_WIRE_H
#define COMLYNX_WIRE_H

#include "types.h"

#define COMLYNX_FRAME_BITS 11

struct ComLynxWireFrame
{
    u64 start_cycle;
    u32 bit_cycles;
    u16 bits;
};

inline bool comlynx_frame_level(const ComLynxWireFrame& frame, u64 cycle)
{
    if (frame.bit_cycles == 0 || cycle < frame.start_cycle)
        return true;

    u64 bit = (cycle - frame.start_cycle) / frame.bit_cycles;

    if (bit >= COMLYNX_FRAME_BITS)
        return true;

    return (frame.bits & (1u << bit)) != 0;
}

inline bool comlynx_wire_level(const ComLynxWireFrame* frames, int count, u64 cycle)
{
    for (int i = 0; i < count; i++)
    {
        if (!comlynx_frame_level(frames[i], cycle))
            return false;
    }

    return true;
}

#endif /* COMLYNX_WIRE_H */