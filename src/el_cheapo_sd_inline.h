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

#ifndef EL_CHEAPO_SD_INLINE_H
#define EL_CHEAPO_SD_INLINE_H

#include "el_cheapo_sd.h"

INLINE bool ElCheapoSD::IsAvailable() const
{
    return m_available;
}

INLINE bool ElCheapoSD::IsSelected() const
{
    return m_available && m_selected;
}

INLINE bool ElCheapoSD::OutputBit() const
{
    return m_output_bit;
}

INLINE void ElCheapoSD::ProcessIO(u8 iodir, u8 iodat)
{
    m_iodir = iodir;
    m_iodat = iodat;
}

INLINE u8 ElCheapoSD::ReadCartByte(u32 address)
{
    if (m_audio_streaming)
    {
        if (m_audio_offset < m_audio_data.size())
            return m_audio_data[m_audio_offset++];

        return 0x00;
    }

    return ReadSramByte(address);
}

INLINE u8 ElCheapoSD::PeekCartByte(u32 address) const
{
    if (m_audio_streaming)
        return m_audio_offset < m_audio_data.size() ? m_audio_data[m_audio_offset] : 0x00;

    return ReadSramByte(address);
}

INLINE u8 ElCheapoSD::ReadSramByte(u32 address) const
{
    return m_sram[address & (SRAM_SIZE - 1)];
}

INLINE size_t ElCheapoSD::GetSaveStateSizeReserve() const
{
    return m_audio_file_name.size() < MAX_FILE_NAME_SIZE ? MAX_FILE_NAME_SIZE - m_audio_file_name.size() : 0;
}

#endif /* EL_CHEAPO_SD_INLINE_H */
