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
        Mikey::Mikey_State* state = m_mikey->GetState();

        s8 sample = state->audio[0].internal_mix ? state->audio[0].output : 0;
        m_channel[0].buffer[m_buffer_pos + 0] = sample;
        m_channel[0].buffer[m_buffer_pos + 1] = sample;

        sample = state->audio[1].internal_mix ? state->audio[1].output : 0;
        m_channel[1].buffer[m_buffer_pos + 0] = sample;
        m_channel[1].buffer[m_buffer_pos + 1] = sample;

        sample = state->audio[2].internal_mix ? state->audio[2].output : 0;
        m_channel[2].buffer[m_buffer_pos + 0] = sample;
        m_channel[2].buffer[m_buffer_pos + 1] = sample;

        sample = state->audio[3].internal_mix ? state->audio[3].output : 0;
        m_channel[3].buffer[m_buffer_pos + 0] = sample;
        m_channel[3].buffer[m_buffer_pos + 1] = sample;

        m_buffer_pos += 2;

        if (m_buffer_pos >= GLYNX_AUDIO_BUFFER_SIZE)
        {
            Debug("WARNING: Audio buffer overflow");
            m_buffer_pos = 0;
        }
    }
}

inline void Audio::Mute(bool mute)
{
    m_mute = mute;
}

inline Audio::GLYNX_Audio_Channel* Audio::GetChannels()
{
    return m_channel;
}

inline u32 Audio::GetFrameSamples()
{
    return m_frame_samples;
}

#endif /* AUDIO_INLINE_H */
