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

#ifndef INPUT_INLINE_H
#define INPUT_INLINE_H

#include "input.h"

INLINE void Input::KeyPressed(GLYNX_Keys key)
{
    m_input |= key;
}

INLINE void Input::KeyReleased(GLYNX_Keys key)
{
    m_input &= ~key;
}

INLINE u8 Input::ReadJoystick()
{
    return (u8)(m_input & 0xFF);
}

INLINE u8 Input::ReadSwitches()
{
    return (u8)(m_input >> 8);
}

#endif /* INPUT_INLINE_H */