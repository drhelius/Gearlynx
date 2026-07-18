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

#ifndef GAME_DRIVE_INLINE_H
#define GAME_DRIVE_INLINE_H

#include "game_drive.h"

INLINE bool GameDrive::IsAvailable() const
{
    return m_available;
}

INLINE bool GameDrive::HasOutput() const
{
    return m_available && m_awake && (m_output_offset < m_output.size());
}

INLINE bool GameDrive::HasProgrammedBank() const
{
    return m_available && m_programmed;
}

INLINE size_t GameDrive::GetSaveStateSizeReserve() const
{
    size_t size = 0;

    if (!m_programmed)
        size += m_program_bank.size();
    if (m_input.size() < MAX_COMMAND_BUFFER_SIZE)
        size += MAX_COMMAND_BUFFER_SIZE - m_input.size();
    if (m_output.size() < MAX_COMMAND_BUFFER_SIZE)
        size += MAX_COMMAND_BUFFER_SIZE - m_output.size();
    if (m_open_file_guest_path.size() < MAX_GUEST_PATH_SIZE)
        size += MAX_GUEST_PATH_SIZE - m_open_file_guest_path.size();
    if (m_open_directory_guest_path.size() < MAX_GUEST_PATH_SIZE)
        size += MAX_GUEST_PATH_SIZE - m_open_directory_guest_path.size();

    return size;
}

INLINE void GameDrive::WriteByte(u8 value)
{
    if (!m_available)
        return;

    if (m_command == NO_COMMAND && value == WAKE_BYTE)
    {
        m_awake = true;
        m_low_power = false;
        m_input.clear();
        m_output.clear();
        m_output_offset = 0;
        return;
    }

    if (!m_awake)
        return;

    if (HasOutput())
        return;

    if (m_command == NO_COMMAND)
        BeginCommand(value);
    else
    {
        m_input.push_back(value);
        ProcessInput();
    }
}

INLINE u8 GameDrive::ReadByte()
{
    if (!HasOutput())
        return 0xFF;

    u8 value = m_output[m_output_offset++];

    if (m_output_offset >= m_output.size())
    {
        m_output.clear();
        m_output_offset = 0;
    }

    return value;
}

INLINE u8 GameDrive::PeekByte() const
{
    if (!HasOutput())
        return 0xFF;

    return m_output[m_output_offset];
}

INLINE u8 GameDrive::ReadProgrammedByte(u32 address) const
{
    if (!HasProgrammedBank())
        return 0xFF;

    return m_program_bank[address & (PROGRAM_BANK_SIZE - 1)];
}

INLINE void GameDrive::FinishCommand()
{
    m_command = NO_COMMAND;
    m_expected_input = 0;
    m_input.clear();
}

INLINE void GameDrive::QueueByte(u8 value)
{
    m_output.push_back(value);
}

INLINE void GameDrive::QueueWord(u16 value)
{
    QueueByte((u8)value);
    QueueByte((u8)(value >> 8));
}

INLINE void GameDrive::QueueDword(u32 value)
{
    QueueWord((u16)value);
    QueueWord((u16)(value >> 16));
}

INLINE void GameDrive::QueueResult(Result result)
{
    QueueByte((u8)result);
}

INLINE u16 GameDrive::InputWord(u32 offset) const
{
    return (u16)(m_input[offset] | (m_input[offset + 1] << 8));
}

INLINE u32 GameDrive::InputDword(u32 offset) const
{
    return (u32)(InputWord(offset) | ((u32)InputWord(offset + 2) << 16));
}

#endif /* GAME_DRIVE_INLINE_H */