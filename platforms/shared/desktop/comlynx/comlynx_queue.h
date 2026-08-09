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

#ifndef COMLYNX_QUEUE_H
#define COMLYNX_QUEUE_H

#include <atomic>
#include "types.h"

template<typename T, u32 Capacity>
class ComLynxQueue
{
public:
    ComLynxQueue()
    {
        Clear();
    }

    bool Push(const T& value)
    {
        u32 head = m_head.load(std::memory_order_relaxed);
        u32 tail = m_tail.load(std::memory_order_acquire);
        if ((head - tail) >= Capacity)
            return false;

        m_values[head % Capacity] = value;
        m_head.store(head + 1, std::memory_order_release);
        return true;
    }

    bool Pop(T& value)
    {
        u32 tail = m_tail.load(std::memory_order_relaxed);
        u32 head = m_head.load(std::memory_order_acquire);
        if (tail == head)
            return false;

        value = m_values[tail % Capacity];
        m_tail.store(tail + 1, std::memory_order_release);
        return true;
    }

    u32 Size() const
    {
        u32 head = m_head.load(std::memory_order_acquire);
        u32 tail = m_tail.load(std::memory_order_acquire);
        return head - tail;
    }

    void Clear()
    {
        m_tail.store(0, std::memory_order_relaxed);
        m_head.store(0, std::memory_order_relaxed);
    }

private:
    T m_values[Capacity];
    std::atomic<u32> m_head;
    std::atomic<u32> m_tail;
};

#endif /* COMLYNX_QUEUE_H */