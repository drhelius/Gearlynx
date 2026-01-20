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

#ifndef MEMORY_INLINE_H
#define	MEMORY_INLINE_H

#include <assert.h>
#include "memory.h"
#include "m6502.h"
#include "bus.h"

INLINE u8* Memory::GetRAM()
{
    return m_state.ram;
}

template<bool debug>
INLINE u8 Memory::Read(u16 address)
{
#if defined(GLYNX_TESTING)
    return m_state.ram[address];
#endif

    if (!debug)
    {
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        m_m6502->CheckMemoryBreakpoints(address, true);
#endif
    }

    if (unlikely(address == 0xFFF9))
        return m_state.MAPCTL;

    u8 page = hi(address);

    if (IsValidPointer(m_read_page[page]))
        return m_read_page[page][lo(address)];
    else
        return (this->*m_read_fn[page])(address);
}

template<bool debug>
INLINE void Memory::Write(u16 address, u8 value)
{
#if defined(GLYNX_TESTING)
    m_state.ram[address] = value;
    return;
#endif

    if (!debug)
    {
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
        m_m6502->CheckMemoryBreakpoints(address, false);
#endif
    }

    if (unlikely(address == 0xFFF9))
    {
        SetMapCtl(value);
        return;
    }

    u8 page = hi(address);

    if (IsValidPointer(m_write_page[page]))
        m_write_page[page][lo(address)] = value;
    else
        (this->*m_write_fn[page])(address, value);
}

INLINE Memory::Memory_State* Memory::GetState()
{
    return &m_state;
}

inline void Memory::SetMapCtl(u8 MAPCTL)
{
    if (m_state.MAPCTL != MAPCTL)
    {
        m_state.MAPCTL = MAPCTL;
        m_m6502->SetPageModeEnabled(IS_NOT_SET_BIT(MAPCTL, 7));
        RebuildMemoryMap();
    }
}

inline void Memory::RebuildMemoryMap()
{
    //Debug("Rebuilding memory map with MAPCTL: %02X", m_state.MAPCTL);

    // SUZY not visible
    if (IS_SET_BIT(m_state.MAPCTL, 0))
    {
        //Debug("SUZY not visible");
        m_read_page[0xFC] = m_state.ram + 0xFC00;
        m_write_page[0xFC] = m_state.ram + 0xFC00;
        m_read_fn[0xFC] = NULL;
        m_write_fn[0xFC] = NULL;
    }
    // SUZY visible
    else
    {
        //Debug("SUZY visible");
        m_read_page[0xFC] = NULL;
        m_write_page[0xFC] = NULL;
        m_read_fn[0xFC] = &Memory::SuzyRead;
        m_write_fn[0xFC] = &Memory::SuzyWrite;
    }

    // MIKEY not visible
    if (IS_SET_BIT(m_state.MAPCTL, 1))
    {
        //Debug("MIKEY not visible");
        m_read_page[0xFD] = m_state.ram + 0xFD00;
        m_write_page[0xFD] = m_state.ram + 0xFD00;
        m_read_fn[0xFD] = NULL;
        m_write_fn[0xFD] = NULL;
    }
    // MIKEY visible
    else
    {
        //Debug("MIKEY visible");
        m_read_page[0xFD] = NULL;
        m_write_page[0xFD] = NULL;
        m_read_fn[0xFD] = &Memory::MikeyRead;
        m_write_fn[0xFD] = &Memory::MikeyWrite;
    }

    // BIOS not visible
    if (IS_SET_BIT(m_state.MAPCTL, 2))
    {
        //Debug("BIOS not visible");
        m_read_page[0xFE] = m_state.ram + 0xFE00;
        m_write_page[0xFE] = m_state.ram + 0xFE00;
        m_read_fn[0xFE] = NULL;
        m_write_fn[0xFE] = NULL;
    }
    // BIOS visible
    else
    {
        //Debug("BIOS visible");
        m_read_page[0xFE] = NULL;
        m_write_page[0xFE] = m_state.ram + 0xFE00;
        m_read_fn[0xFE] = &Memory::BiosRead;
        m_write_fn[0xFE] = NULL;
    }
}

#endif /* MEMORY_INLINE_H */