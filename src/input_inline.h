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
#include "media.h"
#include "suzy.h"

INLINE GLYNX_Keys Input::MapDirectional(GLYNX_Keys key)
{
    GLYNX_Keys mapped = key;
    GLYNX_Rotation rotation = m_media->GetRotation();
    bool lefthanded = m_suzy->GetState()->sprsys_lefthand;

    switch (rotation)
    {
        case ROTATE_RIGHT:
            if (key == GLYNX_KEY_UP)
                mapped = GLYNX_KEY_LEFT;
            else if (key == GLYNX_KEY_LEFT)
                mapped = GLYNX_KEY_DOWN;
            else if (key == GLYNX_KEY_DOWN)
                mapped = GLYNX_KEY_RIGHT;
            else if (key == GLYNX_KEY_RIGHT)
                mapped = GLYNX_KEY_UP;
            break;

        case ROTATE_LEFT:
            if (key == GLYNX_KEY_UP)
                mapped = GLYNX_KEY_RIGHT;
            else if (key == GLYNX_KEY_RIGHT)
                mapped = GLYNX_KEY_DOWN;
            else if (key == GLYNX_KEY_DOWN)
                mapped = GLYNX_KEY_LEFT;
            else if (key == GLYNX_KEY_LEFT)
                mapped = GLYNX_KEY_UP;
            break;

        default:
            break;
    }

    if (lefthanded)
    {
        if (mapped == GLYNX_KEY_UP)
            mapped = GLYNX_KEY_DOWN;
        else if (mapped == GLYNX_KEY_DOWN)
            mapped = GLYNX_KEY_UP;
        else if (mapped == GLYNX_KEY_LEFT)
            mapped = GLYNX_KEY_RIGHT;
        else if (mapped == GLYNX_KEY_RIGHT)
            mapped = GLYNX_KEY_LEFT;
    }

    return mapped;
}

INLINE void Input::KeyPressed(GLYNX_Keys key)
{
    GLYNX_Keys mapped = key;

    bool is_dpad = (key == GLYNX_KEY_UP) || (key == GLYNX_KEY_DOWN) || (key == GLYNX_KEY_LEFT) || (key == GLYNX_KEY_RIGHT);

    if (is_dpad)
        mapped = MapDirectional(key);

    m_input |= mapped;
}

INLINE void Input::KeyReleased(GLYNX_Keys key)
{
    GLYNX_Keys mapped = key;

    bool is_dpad = (key == GLYNX_KEY_UP) || (key == GLYNX_KEY_DOWN) || (key == GLYNX_KEY_LEFT) || (key == GLYNX_KEY_RIGHT);

    if (is_dpad)
        mapped = MapDirectional(key);

    m_input &= ~mapped;
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