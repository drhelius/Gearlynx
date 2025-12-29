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
#include "media.h"
#include "memory.h"
#include "m6502.h"
#include "input.h"
#include "state_serializer.h"

Suzy::Suzy(Media* media, M6502* m6502, Input* input, Bus* bus)
{
    m_media = media;
    m_m6502 = m6502;
    m_input = input;
    m_bus = bus;
    InitPointer(m_memory);
    InitPointer(m_ram);
    Reset();
}
Suzy::~Suzy()
{
}

void Suzy::Init(Memory* memory)
{
    m_memory = memory;
    m_ram = m_memory->GetRAM();
    ComputeQuadLUT();
    Reset();
}

void Suzy::Reset()
{
    memset(&m_state, 0, sizeof(Suzy_State));
    m_state.shift_register_bit = -1;

    for (int i = 0; i < 16; ++i)
        m_state.pen_map[i] = i;
}

void Suzy::MathRunMultiply()
{
    DebugSuzy("MathRunMultiply called");

    m_state.sprsys_lastcarrybit = false;

    u16 ab = (u16(m_state.MATHA) << 8) | m_state.MATHB;
    u16 cd = (u16(m_state.MATHC) << 8) | m_state.MATHD;

    u32 result = (u32)ab * (u32)cd;

    bool negative_result = m_state.sprsys_sign && (m_state.math_sign_A ^ m_state.math_sign_C);
    if (negative_result)
    {
        m_state.sprsys_lastcarrybit = (result != 0);
        result = (u32)(-((s32)result));
    }

    m_state.MATHE = (result >> 24) & 0xFF;
    m_state.MATHF = (result >> 16) & 0xFF;
    m_state.MATHG = (result >> 8) & 0xFF;
    m_state.MATHH = result & 0xFF;

    if (m_state.sprsys_accumulate)
    {
        u32 acc = m_state.MATHJ << 24 | m_state.MATHK << 16 | m_state.MATHL << 8 | m_state.MATHM;
        u64 sum = u64(acc) + u64(result);
        m_state.sprsys_lastcarrybit = (sum > 0xFFFFFFFF);
        m_state.sprsys_mathbit = (sum > 0xFFFFFFFF);
        m_state.MATHJ = (sum >> 24) & 0xFF;
        m_state.MATHK = (sum >> 16) & 0xFF;
        m_state.MATHL = (sum >> 8) & 0xFF;
        m_state.MATHM = sum & 0xFF;
    }

    m_state.sprsys_unsafe = true;
    m_state.sprsys_mathbusy = true;
    m_state.math_cycles = 44 + ((m_state.sprsys_accumulate || m_state.sprsys_sign) ? 10 : 0);
}

void Suzy::MathRunDivide()
{
    DebugSuzy("MathRunDivide called");

    m_state.sprsys_lastcarrybit = false;
    m_state.sprsys_mathbit = false;

    u32 dividend = (u32(m_state.MATHE) << 24) | (u32(m_state.MATHF) << 16) | (u32(m_state.MATHG) << 8) | u32(m_state.MATHH);
    u16 divisor = (u16(m_state.MATHN) << 8) | m_state.MATHP;
    bool zero_divisor = (divisor == 0);
    u32 quotient = 0;
    u16 remainder = 0;

    if (zero_divisor)
    {
        quotient = 0xFFFFFFFF;
        m_state.sprsys_mathbit = true;
        m_state.sprsys_lastcarrybit = true;
    }
    else
    {
        quotient = dividend / divisor;
        remainder = (u16)(dividend % divisor);
        m_state.sprsys_lastcarrybit = (remainder != 0);
    }

    m_state.MATHA = (quotient >> 24) & 0xFF;
    m_state.MATHB = (quotient >> 16) & 0xFF;
    m_state.MATHC = (quotient >> 8) & 0xFF;
    m_state.MATHD = quotient & 0xFF;

    m_state.MATHJ = 0;
    m_state.MATHK = 0;
    m_state.MATHL = (remainder >> 8) & 0xFF;
    m_state.MATHM = remainder & 0xFF;

    m_state.sprsys_unsafe = true;
    m_state.sprsys_mathbusy = true;
    m_state.math_cycles = 162 + (14 * l_zero16(divisor));
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
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void Suzy::LoadState(std::istream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void Suzy::Serialize(StateSerializer& s)
{
    G_SERIALIZE(s, m_state.TMPADR);
    G_SERIALIZE(s, m_state.TILTACUM);
    G_SERIALIZE(s, m_state.HOFF);
    G_SERIALIZE(s, m_state.VOFF);
    G_SERIALIZE(s, m_state.VIDBAS);
    G_SERIALIZE(s, m_state.COLLBAS);
    G_SERIALIZE(s, m_state.VIDADR);
    G_SERIALIZE(s, m_state.COLLADR);
    G_SERIALIZE(s, m_state.SCBNEXT);
    G_SERIALIZE(s, m_state.SPRDLINE);
    G_SERIALIZE(s, m_state.HPOSSTRT);
    G_SERIALIZE(s, m_state.VPOSSTRT);
    G_SERIALIZE(s, m_state.SPRHSIZ);
    G_SERIALIZE(s, m_state.SPRVSIZ);
    G_SERIALIZE(s, m_state.STRETCH);
    G_SERIALIZE(s, m_state.TILT);
    G_SERIALIZE(s, m_state.SPRDOFF);
    G_SERIALIZE(s, m_state.SPRVPOS);
    G_SERIALIZE(s, m_state.COLLOFF);
    G_SERIALIZE(s, m_state.VSIZACUM);
    G_SERIALIZE(s, m_state.HSIZOFF);
    G_SERIALIZE(s, m_state.VSIZOFF);
    G_SERIALIZE(s, m_state.SCBADR);
    G_SERIALIZE(s, m_state.PROCADR);
    G_SERIALIZE(s, m_state.MATHD);
    G_SERIALIZE(s, m_state.MATHC);
    G_SERIALIZE(s, m_state.MATHB);
    G_SERIALIZE(s, m_state.MATHA);
    G_SERIALIZE(s, m_state.MATHP);
    G_SERIALIZE(s, m_state.MATHN);
    G_SERIALIZE(s, m_state.MATHH);
    G_SERIALIZE(s, m_state.MATHG);
    G_SERIALIZE(s, m_state.MATHF);
    G_SERIALIZE(s, m_state.MATHE);
    G_SERIALIZE(s, m_state.MATHM);
    G_SERIALIZE(s, m_state.MATHL);
    G_SERIALIZE(s, m_state.MATHK);
    G_SERIALIZE(s, m_state.MATHJ);
    G_SERIALIZE(s, m_state.SPRCTL0);
    G_SERIALIZE(s, m_state.SPRCTL1);
    G_SERIALIZE(s, m_state.SPRCOLL);
    G_SERIALIZE(s, m_state.SPRINIT);
    G_SERIALIZE(s, m_state.SUZYBUSEN);
    G_SERIALIZE(s, m_state.SPRGO);
    G_SERIALIZE(s, m_state.sprsys_sign);
    G_SERIALIZE(s, m_state.sprsys_accumulate);
    G_SERIALIZE(s, m_state.sprsys_dontcollide);
    G_SERIALIZE(s, m_state.sprsys_vstrech);
    G_SERIALIZE(s, m_state.sprsys_lefthand);
    G_SERIALIZE(s, m_state.sprsys_unsafe);
    G_SERIALIZE(s, m_state.sprsys_stopsprites);
    G_SERIALIZE(s, m_state.sprsys_mathbusy);
    G_SERIALIZE(s, m_state.sprsys_mathbit);
    G_SERIALIZE(s, m_state.sprsys_lastcarrybit);
    G_SERIALIZE(s, m_state.sprsys_spritesbusy);
    G_SERIALIZE_ARRAY(s, m_state.pen_map, 16);
    G_SERIALIZE(s, m_state.sprite_cycles);
    G_SERIALIZE(s, m_state.math_cycles);
    G_SERIALIZE(s, m_state.math_sign_A);
    G_SERIALIZE(s, m_state.math_sign_C);
    G_SERIALIZE(s, m_state.shift_register_address);
    G_SERIALIZE(s, m_state.shift_register_current);
    G_SERIALIZE(s, m_state.shift_register_bit);
    G_SERIALIZE(s, m_state.fred);
    G_SERIALIZE(s, m_state.everon);
}
