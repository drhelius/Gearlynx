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
    m_cycles = 0;
    m_mute = false;
    m_buffer_pos = 0;
    m_sample_left = 0;
    m_sample_right = 0;
    InitPointer(m_buffer);
}

Audio::~Audio()
{
    SafeDeleteArray(m_buffer);
}

void Audio::Init()
{
    m_buffer = new s16[GLYNX_AUDIO_BUFFER_SIZE];
}

void Audio::Reset()
{
    m_cycles = 0;
    m_buffer_pos = 0;
    m_sample_left = 0;
    m_sample_right = 0;
    memset(m_buffer, 0, sizeof(s16) * GLYNX_AUDIO_BUFFER_SIZE);
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
        int samples = m_buffer_pos;
        *sample_count = samples;

        for (int i = 0; i < samples; i++)
        {
            if (m_mute)
                sample_buffer[i] = 0;
            else
            {
                s32 mix = (s32)(m_buffer[i] * 5);

                if (mix > 32767)
                    mix = 32767;
                else if (mix < -32768)
                    mix = -32768;

                sample_buffer[i] = (s16)mix;
            }
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