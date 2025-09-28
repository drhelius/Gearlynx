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

#ifndef AUDIO_INLINE_H
#define AUDIO_INLINE_H

#include "audio.h"
#include "mikey.h"

inline void Audio::Clock(u32 cycles)
{
    m_cycles += cycles;

    if (m_cycles >= GLYNX_AUDIO_CYCLES_PER_SAMPLE)
    {
        m_cycles -= GLYNX_AUDIO_CYCLES_PER_SAMPLE;
        m_sample_left = 0;
        m_sample_right = 0;
        Mikey::Mikey_State* state = m_mikey->GetState();

        m_sample_left += state->audio[0].output;
        m_sample_left += state->audio[1].output;
        m_sample_left += state->audio[2].output;
        m_sample_left += state->audio[3].output;
        m_sample_right = m_sample_left;

        m_buffer[m_buffer_pos + 0] = m_sample_left;
        m_buffer[m_buffer_pos + 1] = m_sample_right;

        m_buffer_pos += 2;

        if (m_buffer_pos >= GLYNX_AUDIO_BUFFER_SIZE)
        {
            Debug("WARNING: Audio buffer overflow");
            m_buffer_pos = 0;
        }
    }
}

#endif /* AUDIO_INLINE_H */
