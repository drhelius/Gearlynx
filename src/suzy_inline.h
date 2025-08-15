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

#ifndef SUZY_INLINE_H
#define SUZY_INLINE_H

#include "suzy.h"
#include "cartridge.h"

INLINE void Suzy::Clock(u32 cycles)
{
}

INLINE u8 Suzy::Read(u16 address)
{
    switch (address)
    {
        case SUZY_SUZYHREV:
            Debug("Suzy Read called for SUZYHREV");
            return 0x01;
        case (SUZY_RCART0):
            return m_cartridge->ReadBank0();
        case (SUZY_RCART1):
            return m_cartridge->ReadBank1();
        default:
            Debug("Suzy Read called with unknown address: %04X", address);
            return 0xFF;
    }
}

INLINE void Suzy::Write(u16 address, u8 value)
{
    m_registers[address] = value;

    switch (address)
    {
        case (SUZY_RCART0):
            m_cartridge->WriteBank0(value);
            break;
        case (SUZY_RCART1):
            m_cartridge->WriteBank1(value);
            break;
        default:
            Debug("Suzy Write called with unknown address: %04X, value: %02X", address, value);
    }
}

#endif /* SUZY_INLINE_H */
