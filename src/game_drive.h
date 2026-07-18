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

#ifndef GAME_DRIVE_H
#define GAME_DRIVE_H

#include <string>
#include <vector>
#include "common.h"

class StateSerializer;
class GameDriveFileSystem;

class GameDrive
{
public:
    GameDrive();
    ~GameDrive();

    void Configure(bool available, const char* root_path);
    void Reset(bool hard);
    bool IsAvailable() const;
    bool HasOutput() const;
    bool HasProgrammedBank() const;
    size_t GetSaveStateSizeReserve() const;
    void WriteByte(u8 value);
    u8 ReadByte();
    u8 PeekByte() const;
    u8 ReadProgrammedByte(u32 address) const;
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    enum Command
    {
        CMD_OPEN_DIR = 0,
        CMD_READ_DIR,
        CMD_OPEN_FILE,
        CMD_GET_SIZE,
        CMD_SEEK,
        CMD_READ,
        CMD_WRITE,
        CMD_CLOSE,
        CMD_PROGRAM_FILE,
        CMD_CLEAR_BLOCKS,
        CMD_LOW_POWER
    };

    enum Result
    {
        RESULT_OK = 0,
        RESULT_DISK_ERROR,
        RESULT_NOT_READY,
        RESULT_NO_FILE,
        RESULT_NOT_OPENED,
        RESULT_NOT_ENABLED,
        RESULT_NO_FILESYSTEM
    };

    enum
    {
        NO_COMMAND = 0xFF,
        WAKE_BYTE = 0xAA,
        MAX_GUEST_PATH_SIZE = 4095,
        MAX_COMMAND_BUFFER_SIZE = 65536,
        PROGRAM_BLOCK_SIZE = 2048,
        PROGRAM_BLOCK_COUNT = 256,
        PROGRAM_BANK_SIZE = PROGRAM_BLOCK_SIZE * PROGRAM_BLOCK_COUNT
    };

private:
    struct DirectoryEntry
    {
        u32 size;
        u16 date;
        u16 time;
        u8 attributes;
        char name[13];
    };

private:
    void BeginCommand(u8 command);
    void ProcessInput();
    void ExecuteCommand();
    void FinishCommand();
    void QueueByte(u8 value);
    void QueueWord(u16 value);
    void QueueDword(u32 value);
    void QueueResult(Result result);
    bool BuildHostPath(const std::string& guest_path, std::string& host_path) const;
    bool OpenFile(const std::string& guest_path);
    bool OpenDirectory(const std::string& guest_path);
    void CloseFile();
    void ReadFileData(u8* data, u32 size);
    bool WriteFileData(const u8* data, u32 size);
    u16 InputWord(u32 offset) const;
    u32 InputDword(u32 offset) const;
    void Serialize(StateSerializer& serializer);
    void SerializeString(StateSerializer& serializer, std::string& value);
    void SerializeByteVector(StateSerializer& serializer, std::vector<u8>& value);

private:
    bool m_available;
    bool m_awake;
    bool m_low_power;
    bool m_programmed;
    bool m_file_open;
    bool m_file_writable;
    u8 m_command;
    u32 m_expected_input;
    u32 m_file_offset;
    u32 m_file_size;
    u32 m_output_offset;
    u32 m_directory_index;
    std::string m_root_path;
    std::string m_open_file_guest_path;
    std::string m_open_directory_guest_path;
    GameDriveFileSystem* m_file_system;
    std::vector<u8> m_input;
    std::vector<u8> m_output;
    std::vector<u8> m_program_bank;
    std::vector<DirectoryEntry> m_directory_entries;
};

#include "game_drive_inline.h"

#endif /* GAME_DRIVE_H */