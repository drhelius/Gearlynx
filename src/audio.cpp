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
#include "audio.h"
#include "mikey.h"

Audio::Audio(Mikey* mikey)
{
    m_mikey = mikey;
    m_mute = false;
    Reset();
}

Audio::~Audio()
{
}

void Audio::Init()
{
    Reset();
}

void Audio::Reset()
{
    m_cycles = 0;
    m_buffer_pos = 0;
    m_sample_left = 0;
    m_sample_right = 0;
    memset(m_buffer, 0, sizeof(s16) * GLYNX_AUDIO_BUFFER_SIZE);
    m_lpfL = 0;
    m_lpfR = 0;
    // fc=4 kHz, fs=44.1 kHz -> alpha = 1 - exp(-2PI·fc/fs) = 0.434 -> Q15 = 14230
    m_lpf_alpha_q15 = (u16)14230;
}

void Audio::Mute(bool mute)
{
    m_mute = mute;
}

void Audio::EndFrame(s16* sample_buffer, int* sample_count)
{
    *sample_count = 0;

    if (IsValidPointer(sample_buffer) && IsValidPointer(sample_count))
    {
        const int samples = m_buffer_pos;
        *sample_count = samples;

        for (int i = 0; i + 1 < samples; i += 2)
        {
            s32 xL = (s32)m_buffer[i + 0];
            s32 xR = (s32)m_buffer[i + 1];

            // 1-pole: y += alpha * (x - y)
            m_lpfL = m_lpfL + (((s32)m_lpf_alpha_q15 * (xL - m_lpfL)) >> 15);
            m_lpfR = m_lpfR + (((s32)m_lpf_alpha_q15 * (xR - m_lpfR)) >> 15);
            xL = m_lpfL;
            xR = m_lpfR;

            s32 outL = m_mute ? 0 : (xL * 40);
            s32 outR = m_mute ? 0 : (xR * 40);

            outL = CLAMP(outL, -32768, 32767);
            outR = CLAMP(outR, -32768, 32767);

            sample_buffer[i + 0] = (s16)outL;
            sample_buffer[i + 1] = (s16)outR;
        }
    }

    m_buffer_pos = 0;
}

void Audio::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Audio::LoadState(std::istream& stream)
{
    UNUSED(stream);
}