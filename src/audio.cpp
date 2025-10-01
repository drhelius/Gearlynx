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

    for (int i = 0; i < 4; i++)
    {
        m_channel[i].mute = false;
        m_channel[i].volume = 1.0f;
    }
}

void Audio::Reset()
{
    m_cycles = 0;
    m_buffer_pos = 0;
    m_frame_samples = 0;
    m_lpfL = 0;
    m_lpfR = 0;
    // fc=4 kHz, fs=44.1 kHz -> alpha = 1 - exp(-2PI·fc/fs) = 0.434 -> Q15 = 14230
    m_lpf_alpha_q15 = (u16)14230;

    for (int i = 0; i < 4; i++)
        memset(m_channel[i].buffer, 0, sizeof(s8) * GLYNX_AUDIO_BUFFER_SIZE);
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
        m_frame_samples = m_buffer_pos;
        *sample_count = m_frame_samples;

        for (u32 i = 0; i + 1 < m_frame_samples; i += 2)
        {
            s32 xL = 0;
            xL += (s32)(m_channel[0].buffer[i + 0] * (m_channel[0].mute ? 0.0f : m_channel[0].volume));
            xL += (s32)(m_channel[1].buffer[i + 0] * (m_channel[1].mute ? 0.0f : m_channel[1].volume));
            xL += (s32)(m_channel[2].buffer[i + 0] * (m_channel[2].mute ? 0.0f : m_channel[2].volume));
            xL += (s32)(m_channel[3].buffer[i + 0] * (m_channel[3].mute ? 0.0f : m_channel[3].volume));
            xL = CLAMP(xL, -32768, 32767);

            s32 xR = 0;
            xR += (s32)(m_channel[0].buffer[i + 1] * (m_channel[0].mute ? 0.0f : m_channel[0].volume));
            xR += (s32)(m_channel[1].buffer[i + 1] * (m_channel[1].mute ? 0.0f : m_channel[1].volume));
            xR += (s32)(m_channel[2].buffer[i + 1] * (m_channel[2].mute ? 0.0f : m_channel[2].volume));
            xR += (s32)(m_channel[3].buffer[i + 1] * (m_channel[3].mute ? 0.0f : m_channel[3].volume));
            xR = CLAMP(xR, -32768, 32767);

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

Audio::GLYNX_Audio_Channel* Audio::GetChannels()
{
    return m_channel;
}

u32 Audio::GetFrameSamples()
{
    return m_frame_samples;
}

void Audio::SaveState(std::ostream& stream)
{
    UNUSED(stream);
}

void Audio::LoadState(std::istream& stream)
{
    UNUSED(stream);
}