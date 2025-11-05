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
#include "state_serializer.h"

Audio::Audio(Mikey* mikey)
{
    m_mikey = mikey;
    m_mute = false;
    m_vgm_recording_enabled = false;
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

#ifndef GLYNX_DISABLE_VGMRECORDER
    if (m_vgm_recording_enabled)
        m_vgm_recorder.UpdateTiming(*sample_count / 2);
#endif

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
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void Audio::LoadState(std::istream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void Audio::Serialize(StateSerializer& s)
{
    G_SERIALIZE(s, m_cycles);
    G_SERIALIZE(s, m_lpf_left);
    G_SERIALIZE(s, m_lpf_right);
    G_SERIALIZE(s, m_buffer_pos);
    G_SERIALIZE(s, m_frame_samples);

    for (int i = 0; i < 4; i++)
    {
        G_SERIALIZE_ARRAY(s, m_channel[i].buffer, GLYNX_AUDIO_BUFFER_SIZE);
    }
}

bool Audio::StartVgmRecording(const char* file_path, int clock_rate)
{
    if (m_vgm_recording_enabled)
        return false;

    m_vgm_recorder.Start(file_path, clock_rate);
    m_vgm_recording_enabled = m_vgm_recorder.IsRecording();

    // Write initial state of all audio registers to VGM
    if (m_vgm_recording_enabled)
    {
        // Get Mikey state
        Mikey::Mikey_State* mikey_state = m_mikey->GetState();

        // Write audio channel registers (0xFD20-0xFD3F)
        for (int i = 0; i < 4; i++)
        {
            u16 base = 0xFD20 + (i * 8);

            // AUDnVOL
            m_vgm_recorder.WriteMikey(base + 0, mikey_state->audio[i].volume);

            // AUDnSHFTFB
            m_vgm_recorder.WriteMikey(base + 1, mikey_state->audio[i].feedback);

            // AUDnOUTVAL
            m_vgm_recorder.WriteMikey(base + 2, mikey_state->audio[i].output);

            // AUDnL8SHFT
            m_vgm_recorder.WriteMikey(base + 3, mikey_state->audio[i].lfsr_low);

            // AUDnTBACK
            m_vgm_recorder.WriteMikey(base + 4, mikey_state->audio[i].backup);

            // AUDnCTL
            m_vgm_recorder.WriteMikey(base + 5, mikey_state->audio[i].control);

            // AUDnCOUNT
            m_vgm_recorder.WriteMikey(base + 6, mikey_state->audio[i].counter);

            // AUDnMISC
            m_vgm_recorder.WriteMikey(base + 7, mikey_state->audio[i].other);
        }

        // Write audio extra registers
        // ATTEN_A (0xFD40)
        m_vgm_recorder.WriteMikey(0xFD40, mikey_state->ATTEN_A);

        // ATTEN_B (0xFD41)
        m_vgm_recorder.WriteMikey(0xFD41, mikey_state->ATTEN_B);

        // ATTEN_C (0xFD42)
        m_vgm_recorder.WriteMikey(0xFD42, mikey_state->ATTEN_C);

        // ATTEN_D (0xFD43)
        m_vgm_recorder.WriteMikey(0xFD43, mikey_state->ATTEN_D);

        // MPAN (0xFD44)
        m_vgm_recorder.WriteMikey(0xFD44, mikey_state->MPAN);

        // MSTEREO (0xFD50)
        m_vgm_recorder.WriteMikey(0xFD50, mikey_state->MSTEREO);
    }

    return m_vgm_recording_enabled;
}

void Audio::StopVgmRecording()
{
    if (m_vgm_recording_enabled)
    {
        m_vgm_recorder.Stop();
        m_vgm_recording_enabled = false;
    }
}

bool Audio::IsVgmRecording() const
{
    return m_vgm_recording_enabled;
}

VgmRecorder* Audio::GetVgmRecorder()
{
    return &m_vgm_recorder;
}
