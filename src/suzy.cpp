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

#include <istream>
#include <ostream>
#include "suzy.h"
#include "cartridge.h"
#include "memory.h"
#include "m6502.h"
#include "mikey.h"

Suzy::Suzy(Cartridge* cartridge, M6502* m6502)
{
    m_cartridge = cartridge;
    m_m6502 = m6502;
    InitPointer(m_mikey);
    InitPointer(m_memory);
    InitPointer(m_ram);
    Reset();
}

Suzy::~Suzy()
{
}

void Suzy::Init(Mikey* mikey, Memory* memory)
{
    m_mikey = mikey;
    m_memory = memory;
    m_ram = m_memory->GetRAM();
    ComputeQuadLUT();
    Reset();
}

void Suzy::Reset()
{
    m_shift_register_address = 0;
    m_shift_register_current = 0;
    m_shift_register_bit = -1;
    memset(&m_state, 0, sizeof(Suzy_State));

    for (int i = 0; i < 16; ++i)
        m_state.pen_map[i] = i;
}

void Suzy::ComputeQuadLUT()
{
    static const int DR = 0;
    static const int DL = 1;
    static const int UR = 2;
    static const int UL = 3;

    static const int k_quad_sequence[4][4] = {
        { DR, UR, UL, DL },
        { DL, DR, UR, UL },
        { UR, UL, DL, DR },
        { UL, DL, DR, UR }
    };

    for (int quad = 0; quad < 4; quad++)
        for (int start = 0; start < 4; start++)
            for (int flip  = 0; flip  < 4; flip++)  // 0=none, 1=H, 2=V, 3=HV (bit0=H, bit1=V)
            {
                int final_quad = k_quad_sequence[start][quad] ^ flip;
                m_quad_lut[quad][start][flip].left = IS_SET_BIT(final_quad, 0);
                m_quad_lut[quad][start][flip].up = IS_SET_BIT(final_quad, 1);
            }
}

void Suzy::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Suzy::LoadState(std::istream& stream)
{
    UNUSED(stream);
}
