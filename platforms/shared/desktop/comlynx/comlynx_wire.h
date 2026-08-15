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

#include <atomic>
#include "types.h"

#define COMLYNX_FRAME_BITS 11

struct ComLynxLocalFrame
{
    u64 start_cycle;
    u32 bit_cycles;
    u16 bits;
};

struct ComLynxSharedFrame
{
    std::atomic<u32> sequence;
    std::atomic<u32> generation;
    std::atomic<u64> start_cycle;
    std::atomic<u32> bit_cycles;
    std::atomic<u32> bits;

    ComLynxSharedFrame() : sequence(0), generation(0), start_cycle(0),
        bit_cycles(0), bits(0) {}
};

static_assert(alignof(ComLynxSharedFrame) >= alignof(std::atomic<u64>),
    "ComLynx shared frames require aligned 64-bit atomics");

inline bool comlynx_shared_frame_atomics_lock_free(const ComLynxSharedFrame& frame)
{
    return frame.sequence.is_lock_free() && frame.generation.is_lock_free() &&
        frame.start_cycle.is_lock_free() && frame.bit_cycles.is_lock_free() &&
        frame.bits.is_lock_free();
}

inline void comlynx_publish_shared_frame(ComLynxSharedFrame& frame, u32 generation,
    u64 start_cycle, u32 bit_cycles, u16 bits)
{
    u32 sequence = frame.sequence.load(std::memory_order_relaxed);
    u32 busy_sequence = (sequence + 1) | 1u;

    frame.sequence.exchange(busy_sequence, std::memory_order_acq_rel);
    frame.generation.store(generation, std::memory_order_relaxed);
    frame.start_cycle.store(start_cycle, std::memory_order_relaxed);
    frame.bit_cycles.store(bit_cycles, std::memory_order_relaxed);
    frame.bits.store(bits & 0x07FF, std::memory_order_relaxed);
    frame.sequence.store(busy_sequence + 1, std::memory_order_release);
}

inline bool comlynx_read_shared_frame(const ComLynxSharedFrame& source,
    u32 expected_generation, ComLynxLocalFrame& frame)
{
    u32 before = source.sequence.load(std::memory_order_acquire);

    if ((before & 1) != 0)
        return false;

    u32 generation = source.generation.load(std::memory_order_relaxed);
    frame.start_cycle = source.start_cycle.load(std::memory_order_relaxed);
    frame.bit_cycles = source.bit_cycles.load(std::memory_order_relaxed);
    frame.bits = (u16)source.bits.load(std::memory_order_relaxed);

    std::atomic_thread_fence(std::memory_order_acquire);
    u32 after = source.sequence.load(std::memory_order_relaxed);

    return before == after && (after & 1) == 0 && generation == expected_generation;
}

inline bool comlynx_frame_level(const ComLynxLocalFrame& frame, u64 cycle)
{
    if (frame.bit_cycles == 0 || cycle < frame.start_cycle)
        return true;

    u64 bit = (cycle - frame.start_cycle) / frame.bit_cycles;

    if (bit >= COMLYNX_FRAME_BITS)
        return true;

    return (frame.bits & (1u << bit)) != 0;
}

inline bool comlynx_wire_level(const ComLynxLocalFrame* frames, int count, u64 cycle)
{
    for (int i = 0; i < count; i++)
    {
        if (!comlynx_frame_level(frames[i], cycle))
            return false;
    }

    return true;
}

#endif /* COMLYNX_WIRE_H */