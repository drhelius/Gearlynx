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
#include "bus.h"
#include "mikey.h"
#include "suzy.h"

INLINE void GearlynxCore::SynchronizeComLynx()
{
    if (m_mikey->IsUartTurbo())
        return;

    u32 sync_cycles = m_mikey->GetComLynxSyncCycles();

    if (m_comlynx_sync_callback && (m_total_cycles >= m_comlynx_next_sync_cycle || sync_cycles != m_comlynx_sync_cycles))
    {
        m_comlynx_sync_callback(m_mikey->GetComLynxCycle(), m_mikey->GetComLynxPromiseCycles(), m_comlynx_sync_user_data);
        m_comlynx_sync_cycles = sync_cycles;
        m_comlynx_next_sync_cycle = m_total_cycles + sync_cycles;
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

INLINE Bus* GearlynxCore::GetBus()
{
    return m_bus;
}

INLINE u64 GearlynxCore::GetComLynxCycle() const
{
    return m_mikey->GetComLynxCycle();
}

#endif /* GEARLYNX_CORE_INLINE_H */
