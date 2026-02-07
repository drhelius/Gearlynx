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

#ifndef EEPROM_INLINE_H
#define EEPROM_INLINE_H

#include "eeprom.h"
#include "bit_ops.h"

INLINE GLYNX_EEPROM EEPROM::GetType()
{
    return m_type;
}

INLINE bool EEPROM::IsAvailable()
{
    return (m_type != GLYNX_EEPROM_NONE);
}

INLINE void EEPROM::ProcessIO(u8 iodir, u8 iodat)
{
    m_iodir = iodir;
    m_iodat = iodat;
}

INLINE bool EEPROM::OutputBit()
{
    return m_audin_output;
}

INLINE u8* EEPROM::GetData()
{
    return (u8*)m_rom_data;
}

INLINE bool EEPROM::IsDirty()
{
    return m_dirty;
}

INLINE void EEPROM::ClearDirty()
{
    m_dirty = false;
}

#endif /* EEPROM_INLINE_H */
