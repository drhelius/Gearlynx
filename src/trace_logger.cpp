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

#include "trace_logger.h"
#include <new>

TraceLogger::TraceLogger()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_buffer = new(std::nothrow) GLYNX_Trace_Entry[TRACE_BUFFER_SIZE];
#else
    m_buffer = NULL;
#endif
    m_capacity = TRACE_BUFFER_SIZE;
    m_position = 0;
    m_count = 0;
    m_enabled_flags = 0;
    m_total_logged = 0;
}

TraceLogger::~TraceLogger()
{
    SafeDeleteArray(m_buffer);
}

void TraceLogger::Reset()
{
    m_position = 0;
    m_count = 0;
    m_total_logged = 0;
}

bool TraceLogger::SetCapacity(u32 capacity)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (capacity == 0)
        return false;
    if (capacity == m_capacity && m_buffer)
        return true;

    GLYNX_Trace_Entry* buffer = new(std::nothrow) GLYNX_Trace_Entry[capacity];
    if (!buffer)
        return false;

    SafeDeleteArray(m_buffer);
    m_buffer = buffer;
    m_capacity = capacity;
    Reset();
    return true;
#else
    m_capacity = capacity;
    Reset();
    return capacity > 0;
#endif
}

void TraceLogger::SetEnabledFlags(u32 flags)
{
    m_enabled_flags = flags;
}

u32 TraceLogger::GetEnabledFlags() const
{
    return m_enabled_flags;
}

const GLYNX_Trace_Entry* TraceLogger::GetBuffer() const
{
    return m_buffer;
}

u32 TraceLogger::GetCount() const
{
    return m_count;
}

u32 TraceLogger::GetCapacity() const
{
    return m_capacity;
}

u32 TraceLogger::GetPosition() const
{
    return m_position;
}

u64 TraceLogger::GetTotalLogged() const
{
    return m_total_logged;
}

const GLYNX_Trace_Entry& TraceLogger::GetEntry(u32 index) const
{
    static const GLYNX_Trace_Entry k_empty = {};
    if (!m_buffer || m_count == 0)
        return k_empty;
    u32 actual;
    if (m_count < m_capacity)
    {
        if (index >= m_count)
            return k_empty;
        actual = index;
    }
    else
        actual = (m_position + index) % m_capacity;
    return m_buffer[actual];
}
