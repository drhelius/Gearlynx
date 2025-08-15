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

#ifndef MIKEY_INLINE_H
#define MIKEY_INLINE_H

#include "mikey.h"
#include "cartridge.h"

INLINE void Mikey::Clock(u32 cycles)
{
}

INLINE u8 Mikey::Read(u16 address)
{
    switch (address)
    {
        case MIKEY_IODIR:
            Debug("Mikey Read called for IODIR");
            return m_registers[address]; // Placeholder value
        case MIKEY_IODAT:
            Debug("Mikey Read called for IODATA");
            return m_registers[address]; // Placeholder value
        default:
            Debug("Mikey Read called with unknown address: %04X", address);
            return m_registers[address];
    }
}

INLINE void Mikey::Write(u16 address, u8 value)
{
    m_registers[address] = value;

    switch (address)
    {
        case MIKEY_SYSCTL1:
            Debug("Mikey Write called for SYSCTL1, value: %02X", value);
            m_cartridge->ShiftRegisterStrobe(value & 0x01);
            break;
        case MIKEY_IODIR:
            Debug("Mikey Write called for IODIR, value: %02X", value);
            break;
        case MIKEY_IODAT:
            Debug("Mikey Write called for IODATA, value: %02X", value);
            m_cartridge->ShiftRegisterBit(value & 0x02);
            break;
        default:
            Debug("Mikey Write called with unknown address: %04X, value: %02X", address, value);
    }
}

#endif /* MIKEY_INLINE_H */
