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
#include "cartridge.h"
#include "input.h"
#include "audio.h"


inline u8 Memory::Read(u16 address, bool block_transfer)
{
#if defined(GLYNX_TESTING)
    return m_test_memory[address];
#endif

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
//TODO:
    //m_m6502->CheckMemoryBreakpoints(M6502::M6502_BREAKPOINT_TYPE_ROMRAM, address, true);
#endif
    return 0;
}

inline void Memory::Write(u16 address, u8 value)
{
#if defined(GLYNX_TESTING)
    m_test_memory[address] = value;
    return;
#endif

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
//TODO:
    //m_m6502->CheckMemoryBreakpoints(M6502::M6502_BREAKPOINT_TYPE_ROMRAM, address, false);
#endif
}

#endif /* MEMORY_INLINE_H */