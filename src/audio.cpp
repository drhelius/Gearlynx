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
#include <math.h>
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

    for (int i = 0; i < 4; i++)
    {
        m_channel[i].mute = false;
        m_channel[i].volume = 1.0f;
    }

    SetLowpassCutoff(1000.0f);
}

void Audio::Reset()
{
    m_cycles = 0;
    m_buffer_pos = 0;
    m_frame_samples = 0;
    m_lpf_left = 0;
    m_lpf_right = 0;

    for (int i = 0; i < 4; i++)
        memset(m_channel[i].buffer, 0, sizeof(s8) * GLYNX_AUDIO_BUFFER_SIZE);
}

void Audio::EndFrame(s16* sample_buffer, int* sample_count)
{
    *sample_count = 0;

    if (IsValidPointer(sample_buffer) && IsValidPointer(sample_count))
    {
        m_frame_samples = m_buffer_pos;
        *sample_count = m_frame_samples;

        for (u32 i = 0; i + 1 < m_frame_samples; i += 2)
        {
            s32 x_left = 0;
            x_left += (s32)(m_channel[0].buffer[i + 0] * (m_channel[0].mute ? 0.0f : m_channel[0].volume));
            x_left += (s32)(m_channel[1].buffer[i + 0] * (m_channel[1].mute ? 0.0f : m_channel[1].volume));
            x_left += (s32)(m_channel[2].buffer[i + 0] * (m_channel[2].mute ? 0.0f : m_channel[2].volume));
            x_left += (s32)(m_channel[3].buffer[i + 0] * (m_channel[3].mute ? 0.0f : m_channel[3].volume));

            s32 x_right = 0;
            x_right += (s32)(m_channel[0].buffer[i + 1] * (m_channel[0].mute ? 0.0f : m_channel[0].volume));
            x_right += (s32)(m_channel[1].buffer[i + 1] * (m_channel[1].mute ? 0.0f : m_channel[1].volume));
            x_right += (s32)(m_channel[2].buffer[i + 1] * (m_channel[2].mute ? 0.0f : m_channel[2].volume));
            x_right += (s32)(m_channel[3].buffer[i + 1] * (m_channel[3].mute ? 0.0f : m_channel[3].volume));

            // Single-pole low-pass filter
            // y += alpha * (x - y)
            m_lpf_left += ((s32)m_lpf_alpha_q15 * (x_left - m_lpf_left)) >> 15;
            m_lpf_right += ((s32)m_lpf_alpha_q15 * (x_right - m_lpf_right)) >> 15;
            s32 y_left = m_lpf_left;
            s32 y_right = m_lpf_right;

            s32 out_left = 0;
            s32 out_right = 0;

            if (!m_mute)
            {
                out_left = CLAMP(y_left * 40, -32768, 32767);
                out_right = CLAMP(y_right * 40, -32768, 32767);
            }

            sample_buffer[i + 0] = (s16)out_left;
            sample_buffer[i + 1] = (s16)out_right;
        }
    }

    m_buffer_pos = 0;
}

void Audio::SetVolume(int channel, float volume)
{
    if (channel < 0 || channel >= 4)
        return;

    m_channel[channel].volume = volume;
}

void Audio::SetLowpassCutoff(float fc)
{
    const float fs = 44100.0f;
    // alpha = 1 - exp(-2 * pi * fc / fs)
    float alpha = 1.0f - expf(-2.0f * 3.14159265358979323846f * fc / fs);
    alpha = CLAMP(alpha, 0.0f, 0.9999f);
    // convert to Q1.15 fixed point
    m_lpf_alpha_q15 = (u16)(alpha * 32768.0f + 0.5f);
}

void Audio::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Audio::LoadState(std::istream& stream)
{
    UNUSED(stream);
}
