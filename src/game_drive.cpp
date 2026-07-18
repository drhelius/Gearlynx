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

#include <algorithm>
#include <cctype>
#include <limits>
#include "game_drive.h"
#include "game_drive_filesystem.h"
#include "state_serializer.h"

static void BuildShortName(const char* file_name, char* short_name)
{
    std::string name = file_name;
    size_t dot = name.find_last_of('.');
    std::string base = (dot == std::string::npos || dot == 0) ? name : name.substr(0, dot);
    std::string extension = (dot == std::string::npos || dot == 0) ? "" : name.substr(dot + 1);
    std::string clean_base;
    std::string clean_extension;

    for (size_t index = 0; index < base.size(); index++)
    {
        unsigned char value = (unsigned char)base[index];
        if (std::isalnum(value) || value == '_' || value == '-' || value == '$')
            clean_base += (char)std::toupper(value);
    }

    for (size_t index = 0; index < extension.size(); index++)
    {
        unsigned char value = (unsigned char)extension[index];
        if (std::isalnum(value) || value == '_' || value == '-' || value == '$')
            clean_extension += (char)std::toupper(value);
    }

    if (clean_base.empty())
        clean_base = "FILE";
    if (clean_base.size() > 8)
        clean_base = clean_base.substr(0, 6) + "~1";
    if (clean_extension.size() > 3)
        clean_extension.resize(3);

    std::string result = clean_base;
    if (!clean_extension.empty())
        result += "." + clean_extension;

    memset(short_name, 0, 13);
    memcpy(short_name, result.c_str(), MIN(result.size(), (size_t)12));
}

GameDrive::GameDrive()
{
    m_file_system = CreateGameDriveFileSystem();
    m_program_bank.resize(PROGRAM_BANK_SIZE);
    m_available = false;
    m_root_path = ".";
    Reset(true);
}

GameDrive::~GameDrive()
{
    CloseFile();
    SafeDelete(m_file_system);
}

void GameDrive::Configure(bool available, const char* root_path)
{
    m_root_path = (root_path && root_path[0]) ? root_path : ".";
    m_available = available && m_file_system && m_file_system->IsAvailable() && m_file_system->IsValidRootPath(root_path);
    Reset(true);
}

void GameDrive::Reset(bool hard)
{
    CloseFile();
    m_awake = true;
    m_low_power = false;
    m_command = NO_COMMAND;
    m_expected_input = 0;
    m_file_offset = 0;
    m_file_size = 0;
    m_output_offset = 0;
    m_directory_index = 0;
    m_open_file_guest_path.clear();
    m_open_directory_guest_path.clear();
    m_input.clear();
    m_output.clear();
    m_directory_entries.clear();

    if (hard)
    {
        m_programmed = false;
        std::fill(m_program_bank.begin(), m_program_bank.end(), 0);
    }
}

void GameDrive::BeginCommand(u8 command)
{
    m_command = command;
    m_input.clear();

    switch (m_command)
    {
        case CMD_OPEN_DIR:
        case CMD_OPEN_FILE:
            m_expected_input = std::numeric_limits<u32>::max();
            break;
        case CMD_SEEK:
        case CMD_CLEAR_BLOCKS:
            m_expected_input = 4;
            break;
        case CMD_READ:
        case CMD_WRITE:
            m_expected_input = 2;
            break;
        case CMD_PROGRAM_FILE:
            m_expected_input = 5;
            break;
        case CMD_READ_DIR:
        case CMD_GET_SIZE:
        case CMD_CLOSE:
        case CMD_LOW_POWER:
            m_expected_input = 0;
            ExecuteCommand();
            break;
        default:
            QueueResult(RESULT_NOT_ENABLED);
            FinishCommand();
            break;
    }
}

void GameDrive::ProcessInput()
{
    if ((m_command == CMD_OPEN_DIR || m_command == CMD_OPEN_FILE) && !m_input.empty() && m_input.back() == 0)
    {
        ExecuteCommand();
        return;
    }

    if ((m_command == CMD_OPEN_DIR || m_command == CMD_OPEN_FILE) && m_input.size() > MAX_GUEST_PATH_SIZE)
    {
        QueueResult(RESULT_NO_FILE);
        FinishCommand();
        return;
    }

    if (m_command == CMD_WRITE && m_input.size() == 2)
        m_expected_input = 2 + InputWord(0);

    if (m_input.size() >= m_expected_input)
        ExecuteCommand();
}

void GameDrive::ExecuteCommand()
{
    switch (m_command)
    {
        case CMD_OPEN_DIR:
        {
            std::string path(reinterpret_cast<const char*>(&m_input[0]));
            QueueResult(OpenDirectory(path) ? RESULT_OK : RESULT_NO_FILE);
            break;
        }
        case CMD_READ_DIR:
            if (m_directory_index >= m_directory_entries.size())
                QueueResult(RESULT_NO_FILE);
            else
            {
                const DirectoryEntry& entry = m_directory_entries[m_directory_index++];
                QueueResult(RESULT_OK);
                QueueDword(entry.size);
                QueueWord(entry.date);
                QueueWord(entry.time);
                QueueByte(entry.attributes);
                for (u32 index = 0; index < sizeof(entry.name); index++)
                    QueueByte((u8)entry.name[index]);
            }
            break;
        case CMD_OPEN_FILE:
        {
            std::string path(reinterpret_cast<const char*>(&m_input[0]));
            QueueResult(OpenFile(path) ? RESULT_OK : RESULT_NO_FILE);
            break;
        }
        case CMD_GET_SIZE:
            QueueDword(m_file_open ? m_file_size : 0);
            break;
        case CMD_SEEK:
            if (!m_file_open)
                QueueResult(RESULT_NOT_OPENED);
            else
            {
                m_file_offset = InputDword(0);
                QueueResult(RESULT_OK);
            }
            break;
        case CMD_READ:
            if (!m_file_open)
                QueueResult(RESULT_NOT_OPENED);
            else
            {
                u16 size = InputWord(0);
                u32 output_start = (u32)m_output.size();
                m_output.resize(output_start + size);
                ReadFileData(&m_output[output_start], size);
                QueueResult(RESULT_OK);
            }
            break;
        case CMD_WRITE:
            if (!m_file_open)
                QueueResult(RESULT_NOT_OPENED);
            else if (!m_file_writable)
                QueueResult(RESULT_DISK_ERROR);
            else
            {
                u16 size = InputWord(0);
                QueueResult(WriteFileData(&m_input[2], size) ? RESULT_OK : RESULT_DISK_ERROR);
            }
            break;
        case CMD_CLOSE:
            if (!m_file_open)
                QueueResult(RESULT_NOT_OPENED);
            else
            {
                CloseFile();
                QueueResult(RESULT_OK);
            }
            break;
        case CMD_PROGRAM_FILE:
            if (!m_file_open)
                QueueResult(RESULT_NOT_OPENED);
            else
            {
                u16 start_block = InputWord(0);
                u8 block_units = m_input[2] & 0x0F;
                u16 block_count = InputWord(3);
                u32 block_size = block_units * 256;

                if ((block_units != 1 && block_units != 2 && block_units != 4 && block_units != 8) ||
                    start_block >= PROGRAM_BLOCK_COUNT || block_count > PROGRAM_BLOCK_COUNT - start_block)
                {
                    QueueResult(RESULT_DISK_ERROR);
                    break;
                }

                for (u32 block = 0; block < block_count; block++)
                {
                    u8* destination = &m_program_bank[(start_block + block) * PROGRAM_BLOCK_SIZE];
                    std::fill(destination, destination + PROGRAM_BLOCK_SIZE, 0);
                    ReadFileData(destination, block_size);
                }

                m_programmed = true;
                QueueResult(RESULT_OK);
            }
            break;
        case CMD_CLEAR_BLOCKS:
        {
            u16 start_block = InputWord(0);
            u16 block_count = InputWord(2) & 0x7FFF;

            if (start_block >= PROGRAM_BLOCK_COUNT || block_count > PROGRAM_BLOCK_COUNT - start_block)
                QueueResult(RESULT_DISK_ERROR);
            else
            {
                u8* first = &m_program_bank[start_block * PROGRAM_BLOCK_SIZE];
                std::fill(first, first + block_count * PROGRAM_BLOCK_SIZE, 0);
                m_programmed = true;
                QueueResult(RESULT_OK);
            }
            break;
        }
        case CMD_LOW_POWER:
            m_low_power = true;
            m_awake = false;
            break;
        default:
            QueueResult(RESULT_NOT_ENABLED);
            break;
    }

    FinishCommand();
}

bool GameDrive::BuildHostPath(const std::string& guest_path, std::string& host_path) const
{
    std::string path = guest_path;
    std::replace(path.begin(), path.end(), '\\', '/');

    if (path.find(':') != std::string::npos)
        return false;

    std::vector<std::string> parts;
    size_t offset = 0;

    while (offset <= path.size())
    {
        size_t separator = path.find('/', offset);
        std::string part = path.substr(offset, separator - offset);

        if (part == "..")
            return false;
        if (!part.empty() && part != ".")
            parts.push_back(part);

        if (separator == std::string::npos)
            break;
        offset = separator + 1;
    }

    host_path = m_root_path;
    for (size_t i = 0; i < parts.size(); i++)
        append_path_component(host_path, parts[i].c_str());

    return true;
}

bool GameDrive::OpenFile(const std::string& guest_path)
{
    CloseFile();

    std::string host_path;
    if (!BuildHostPath(guest_path, host_path))
        return false;

    u32 size = 0;
    if (!m_file_system->OpenFile(host_path.c_str(), m_file_writable, size))
        return false;

    m_file_open = true;
    m_file_offset = 0;
    m_file_size = size;
    m_open_file_guest_path = guest_path;
    return true;
}

bool GameDrive::OpenDirectory(const std::string& guest_path)
{
    std::string host_path;
    if (!BuildHostPath(guest_path, host_path))
        return false;

    m_directory_entries.clear();
    m_directory_index = 0;

    std::vector<GameDriveFileSystemEntry> file_system_entries;
    if (!m_file_system->ReadDirectory(host_path.c_str(), file_system_entries))
        return false;

    for (size_t index = 0; index < file_system_entries.size(); index++)
    {
        const GameDriveFileSystemEntry& file_system_entry = file_system_entries[index];
        DirectoryEntry entry = {};
        entry.size = (u32)MIN(file_system_entry.size, 0xFFFFFFFFULL);
        entry.date = file_system_entry.date;
        entry.time = file_system_entry.time;
        entry.attributes = file_system_entry.directory ? 0x10 : 0x20;
        if (file_system_entry.read_only)
            entry.attributes |= 0x01;
        if (file_system_entry.hidden)
            entry.attributes |= 0x02;
        BuildShortName(file_system_entry.name.c_str(), entry.name);
        m_directory_entries.push_back(entry);
    }

    std::sort(m_directory_entries.begin(), m_directory_entries.end(),
        [](const DirectoryEntry& left, const DirectoryEntry& right)
        {
            return strcmp(left.name, right.name) < 0;
        });

    m_open_directory_guest_path = guest_path;
    return true;
}

void GameDrive::CloseFile()
{
    if (m_file_system)
        m_file_system->CloseFile();
    m_file_open = false;
    m_file_writable = false;
    m_file_offset = 0;
    m_file_size = 0;
    m_open_file_guest_path.clear();
}

void GameDrive::ReadFileData(u8* data, u32 size)
{
    memset(data, 0, size);

    if (m_file_offset < m_file_size)
    {
        u32 readable = MIN(size, m_file_size - m_file_offset);
        m_file_system->ReadFile(m_file_offset, data, readable);
    }

    m_file_offset += size;
}

bool GameDrive::WriteFileData(const u8* data, u32 size)
{
    if (!m_file_system->WriteFile(m_file_offset, data, size))
        return false;

    m_file_offset += size;
    m_file_size = MAX(m_file_size, m_file_offset);
    return true;
}

void GameDrive::SaveState(std::ostream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void GameDrive::LoadState(std::istream& stream)
{
    CloseFile();
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void GameDrive::Serialize(StateSerializer& serializer)
{
    G_SERIALIZE(serializer, m_awake);
    G_SERIALIZE(serializer, m_low_power);
    G_SERIALIZE(serializer, m_programmed);
    G_SERIALIZE(serializer, m_file_open);
    G_SERIALIZE(serializer, m_command);
    G_SERIALIZE(serializer, m_expected_input);
    G_SERIALIZE(serializer, m_file_offset);
    G_SERIALIZE(serializer, m_output_offset);
    G_SERIALIZE(serializer, m_directory_index);
    SerializeString(serializer, m_open_file_guest_path);
    SerializeString(serializer, m_open_directory_guest_path);
    SerializeByteVector(serializer, m_input);
    SerializeByteVector(serializer, m_output);

    if (m_programmed)
        G_SERIALIZE_ARRAY(serializer, &m_program_bank[0], m_program_bank.size());
    else if (serializer.IsLoading())
        std::fill(m_program_bank.begin(), m_program_bank.end(), 0);

    if (serializer.IsLoading())
    {
        bool restore_file = m_file_open;
        u32 restore_offset = m_file_offset;
        std::string restore_file_path = m_open_file_guest_path;
        std::string restore_directory_path = m_open_directory_guest_path;
        u32 restore_directory_index = m_directory_index;

        m_file_open = false;
        m_file_writable = false;
        m_file_size = 0;

        if (restore_file && OpenFile(restore_file_path))
            m_file_offset = restore_offset;

        if (!restore_directory_path.empty() && OpenDirectory(restore_directory_path))
            m_directory_index = MIN(restore_directory_index, (u32)m_directory_entries.size());

        if (m_output_offset > m_output.size())
            m_output_offset = (u32)m_output.size();
    }
}

void GameDrive::SerializeString(StateSerializer& serializer, std::string& value)
{
    u32 size = (u32)value.size();
    G_SERIALIZE(serializer, size);

    if (serializer.IsLoading())
        value.resize(size);
    if (size > 0)
        G_SERIALIZE_ARRAY(serializer, &value[0], size);
}

void GameDrive::SerializeByteVector(StateSerializer& serializer, std::vector<u8>& value)
{
    u32 size = (u32)value.size();
    G_SERIALIZE(serializer, size);

    if (serializer.IsLoading())
        value.resize(size);
    if (size > 0)
        G_SERIALIZE_ARRAY(serializer, &value[0], size);
}