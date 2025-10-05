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

#include "input.h"
#include "common.h"

Input::Input(Media* media)
{
    m_media = media;
    InitPointer(m_suzy);
    m_input = 0x0000;
}

void Input::Init(Suzy* suzy)
{
    m_suzy = suzy;
    Reset();
}

void Input::Reset()
{
    m_input = 0x0000;
}

void Input::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Input::LoadState(std::istream& stream)
{
    UNUSED(stream);
}