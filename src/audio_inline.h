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

        // Lynx II
        if (likely(m_is_lynx2))
        {
            // MSTEREO ($FD50) controls channel routing to each ear (bit=1 disables)
            // MPAN ($FD44) enables attenuation per channel/ear (bit=1 enables)
            // ATTEN_X ($FD40-$FD43) sets attenuation per channel: bits 7-4=left, 3-0=right
            // Attenuation value: 0=full volume, 15=silence

            u8 mstereo = state->MSTEREO;
            u8 mpan = state->MPAN;
            const u8* atten = &state->ATTEN_A;

            for (int ch = 0; ch < 4; ch++)
            {
                s8 sample = state->audio[ch].internal_mix ? state->audio[ch].output : 0;
                u8 ch_atten = atten[ch];

                // Left
                if (IS_NOT_SET_BIT(mstereo, 4 + ch))
                {
                    s32 att = IS_SET_BIT(mpan, 4 + ch) ? (15 - (ch_atten >> 4)) : 15;
                    m_channel[ch].buffer[m_buffer_pos + 0] = (sample * att) / 15;
                }
                else
                    m_channel[ch].buffer[m_buffer_pos + 0] = 0;

                // Right
                if (IS_NOT_SET_BIT(mstereo, ch))
                {
                    s32 att = IS_SET_BIT(mpan, ch) ? (15 - (ch_atten & 0x0F)) : 15;
                    m_channel[ch].buffer[m_buffer_pos + 1] = (sample * att) / 15;
                }
                else
                    m_channel[ch].buffer[m_buffer_pos + 1] = 0;
            }
        }
        // Lynx I
        else
        {
            for (int ch = 0; ch < 4; ch++)
            {
                s8 sample = state->audio[ch].internal_mix ? state->audio[ch].output : 0;
                m_channel[ch].buffer[m_buffer_pos + 0] = sample;
                m_channel[ch].buffer[m_buffer_pos + 1] = sample;
            }
        }

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
