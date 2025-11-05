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

#ifndef AUDIO_H
#define AUDIO_H

#include "common.h"
#include "vgm_recorder.h"

class Mikey;
class StateSerializer;

class Audio
{
public:
    struct GLYNX_Audio_Channel
    {
        bool mute;
        float volume;
        s8 buffer[GLYNX_AUDIO_BUFFER_SIZE];
    };

public:
    Audio(Mikey* mikey);
    ~Audio();
    void Init();
    void Reset();
    void Clock(u32 cycles);
    void EndFrame(s16* sample_buffer, int* sample_count);
    void Mute(bool mute);
    GLYNX_Audio_Channel* GetChannels();
    u32 GetFrameSamples();
    void SetVolume(int channel, float volume);
    void SetLowpassCutoff(float fc);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);
    void Serialize(StateSerializer& s);
    bool StartVgmRecording(const char* file_path, int clock_rate);
    void StopVgmRecording();
    bool IsVgmRecording() const;
    VgmRecorder* GetVgmRecorder();

private:
    Mikey* m_mikey;
    u32 m_cycles;
    bool m_mute;
    s32 m_lpf_left;
    s32 m_lpf_right;
    u16 m_lpf_alpha_q15;
    u32 m_buffer_pos;
    u32 m_frame_samples;
    GLYNX_Audio_Channel m_channel[4];
    VgmRecorder m_vgm_recorder;
    bool m_vgm_recording_enabled;
};

#include "audio_inline.h"

#endif /* AUDIO_H */
