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

#ifndef INPUT_H
#define INPUT_H

#include <iostream>
#include <fstream>
#include "common.h"

class Media;
class Suzy;

class Input
{
public:
    Input(Media* media);
    void Init(Suzy* suzy);
    void Reset();
    void KeyPressed(GLYNX_Keys key);
    void KeyReleased(GLYNX_Keys key);
    u8 ReadJoystick();
    u8 ReadSwitches();
    GLYNX_Keys MapDirectional(GLYNX_Keys key);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    Media* m_media;
    Suzy* m_suzy;
    u16 m_input;
};

#include "input_inline.h"

#endif /* INPUT_H */