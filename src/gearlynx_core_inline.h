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
#include "cartridge.h"
#include "m6502.h"
#include "audio.h"
#include "mikey.h"
#include "suzy.h"

INLINE bool GearlynxCore::RunToVBlank(u8* frame_buffer, s16* sample_buffer, int* sample_count, GLYNX_Debug_Run* debug)
{
    if (m_paused || !m_cartridge->IsReady())
        return false;

    if (!m_cartridge->IsBiosLoaded())
    {
        //TODO: implement bios missing message
        //RenderFrameBuffer(pFrameBuffer);
        return false;
    }

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
        bool instruction_completed = false;
        if (IsValidPointer(debug))
        {
            debug_enable = true;
            m_m6502->EnableBreakpoints(debug->stop_on_breakpoint, debug->stop_on_irq);
        }

        bool stop = false;

        u32 temp_max_cycles = 0;

        do
        {
            if (debug_enable && (IsValidPointer(m_debug_callback)))
                m_debug_callback();

            u32 cycles = m_m6502->RunInstruction(&instruction_completed);
            m_suzy->Clock(cycles);
            stop = m_mikey->Clock(cycles);

            temp_max_cycles += cycles;
            if (temp_max_cycles > 90000)
            {
                Debug("Exceeded max cycles in RunToVBlankTemplate");
                stop = true;
            }

            m_audio->Clock(cycles);

            if (debug_enable)
            {
                if (debug->step_debugger)
                    stop = instruction_completed;

                if (instruction_completed)
                {
                    if (m_m6502->BreakpointHit())
                        stop = true;

                    if (debug->stop_on_run_to_breakpoint && m_m6502->RunToBreakpointHit())
                        stop = true;
                }
            }
        }
        while (!stop);

        Debug("RunToVBlankTemplate: Exiting after %u cycles", temp_max_cycles);

        m_audio->EndFrame(sample_buffer, sample_count);

        return m_m6502->BreakpointHit() || m_m6502->RunToBreakpointHit();
    }
    else
    {
        UNUSED(debug);

        bool stop = false;

        do
        {
            u32 cycles = m_m6502->RunInstruction();

            //TODO: implement video
            stop = true;

            m_audio->Clock(cycles);
        }
        while (!stop);

        m_audio->EndFrame(sample_buffer, sample_count);

        return false;
    }
}

INLINE Memory* GearlynxCore::GetMemory()
{
    return m_memory;
}

INLINE Cartridge* GearlynxCore::GetCartridge()
{
    return m_cartridge;
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