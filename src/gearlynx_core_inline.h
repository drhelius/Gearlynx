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

#ifndef GEARLYNX_CORE_INLINE_H
#define GEARLYNX_CORE_INLINE_H

#include "gearlynx_core.h"
#include "media.h"
#include "m6502.h"
#include "audio.h"
#include "mikey.h"
#include "suzy.h"

INLINE bool GearlynxCore::RunToVBlank(u8* frame_buffer, s16* sample_buffer, int* sample_count, GLYNX_Debug_Run* debug)
{
    if (!m_media->IsBiosLoaded())
    {
        m_mikey->RenderNoBiosScreen(frame_buffer);
        return false;
    }

    if (m_paused || !m_media->IsReady())
        return false;

#if defined(GLYNX_DISABLE_DISASSEMBLER)
    const bool debugger = false;
#else
    const bool debugger = true;
#endif

    if (debugger)
        return RunToVBlankTemplate<true>(frame_buffer, sample_buffer, sample_count, debug);
    else
        return RunToVBlankTemplate<false>(frame_buffer, sample_buffer, sample_count, debug);
}

template<bool debugger>
bool GearlynxCore::RunToVBlankTemplate(u8* frame_buffer, s16* sample_buffer, int* sample_count, GLYNX_Debug_Run* debug)
{
    m_mikey->SetBuffer(frame_buffer);

    if (debugger)
    {
        bool debug_enable = false;

        if (IsValidPointer(debug))
        {
            debug_enable = true;
            m_m6502->EnableBreakpoints(debug->stop_on_breakpoint, debug->stop_on_irq);
        }

        bool stop = false;
        u32 failsafe_cycle_count = 0;

        do
        {
            if (debug_enable && (IsValidPointer(m_debug_callback)) && !m_m6502->IsHalted())
                m_debug_callback();

            u32 cpu_cycles = m_m6502->RunInstruction();
            u32 lynx_cycles = (cpu_cycles * 5) - 1;

            m_suzy->Clock(lynx_cycles);
            stop = m_mikey->Clock(lynx_cycles);
            m_audio->Clock(lynx_cycles);

            failsafe_cycle_count += lynx_cycles;
            if (failsafe_cycle_count > 450000)
            {
                Debug("Exceeded max cycles in RunToVBlankTemplate");
                stop = true;
            }

            if (debug_enable)
            {
                if (debug->step_debugger && !m_m6502->IsHalted())
                    stop = true;

                if (m_m6502->BreakpointHit())
                    stop = true;

                if (debug->stop_on_run_to_breakpoint && m_m6502->RunToBreakpointHit())
                    stop = true;
            }
        }
        while (!stop);

        //Debug("RunToVBlankTemplate: Exiting after %u cycles", failsafe_cycle_count);

        m_audio->EndFrame(sample_buffer, sample_count);

        GLYNX_Rotation rotation = m_media->GetRotation();
        if (rotation != NO_ROTATION)
            m_mikey->RotateFrameBuffer(rotation);

        return m_m6502->BreakpointHit() || m_m6502->RunToBreakpointHit();
    }
    else
    {
        UNUSED(debug);

        bool stop = false;
        u32 failsafe_cycle_count = 0;

        do
        {
            u32 cpu_cycles = m_m6502->RunInstruction();
            u32 lynx_cycles = cpu_cycles * 5;

            m_suzy->Clock(lynx_cycles);
            stop = m_mikey->Clock(lynx_cycles);
            m_audio->Clock(lynx_cycles);

            failsafe_cycle_count += lynx_cycles;
            if (failsafe_cycle_count > 450000)
            {
                Debug("Exceeded max cycles in RunToVBlankTemplate");
                stop = true;
            }
        }
        while (!stop);

        m_audio->EndFrame(sample_buffer, sample_count);

        GLYNX_Rotation rotation = m_media->GetRotation();
        if (rotation != NO_ROTATION)
            m_mikey->RotateFrameBuffer(rotation);

        return false;
    }
}

INLINE Memory* GearlynxCore::GetMemory()
{
    return m_memory;
}

INLINE Media* GearlynxCore::GetMedia()
{
    return m_media;
}

INLINE Audio* GearlynxCore::GetAudio()
{
    return m_audio;
}

INLINE Input* GearlynxCore::GetInput()
{
    return m_input;
}

INLINE M6502* GearlynxCore::GetM6502()
{
    return m_m6502;
}

INLINE Suzy* GearlynxCore::GetSuzy()
{
    return m_suzy;
}

INLINE Mikey* GearlynxCore::GetMikey()
{
    return m_mikey;
}

#endif /* GEARLYNX_CORE_INLINE_H */