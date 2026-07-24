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

#ifndef EL_CHEAPO_SD_H
#define EL_CHEAPO_SD_H

#include <string>
#include <vector>
#include "common.h"

class SdCardFileSystem;
class StateSerializer;

class ElCheapoSD
{
public:
    ElCheapoSD();
    ~ElCheapoSD();

    void Configure(bool available, const char* root_path, const u8* initial_data, u32 initial_size);
    void Reset(bool hard);
    bool IsAvailable() const;
    bool IsSelected() const;
    bool OutputBit() const;
    void ProcessIO(u8 iodir, u8 iodat);
    void ProcessCounter(u16 counter);
    u8 ReadCartByte(u32 address);
    u8 PeekCartByte(u32 address) const;
    u8 ReadSramByte(u32 address) const;
    size_t GetSaveStateSizeReserve() const;
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    enum SerialState
    {
        SERIAL_NONE = 0,
        SERIAL_ADDRESS,
        SERIAL_WRITE,
        SERIAL_READ
    };

    enum
    {
        MAILBOX_SIZE = 128,
        MAILBOX_WORDS = 64,
        MAILBOX_STATUS_WORD = 0x3F,
        MAX_FILE_NAME_SIZE = 12,
        MAX_SAVE_SIZE = 108,
        MAX_AUDIO_SIZE = 64 * 1024 * 1024,
        SRAM_SIZE = 512 * 1024
    };

private:
    void ExecuteCommand();
    void ExecuteNew();
    void ExecuteLoad();
    void ExecuteSave();
    void ExecuteAudioSetFile();
    void ExecuteAudioPlay();
    void ExecuteAudioStop();
    bool LoadAudioFile(const std::string& file_name);
    void SetResponse(const char* response);
    u16 ReadMailboxWord(u8 address) const;
    void WriteMailboxWord(u8 address, u16 value);
    std::string GetMailboxFileName(u32 offset) const;
    bool ResolveExistingPath(const std::string& file_name, std::string& host_path, bool& directory, u32& size);
    void BuildHostPath(const std::string& file_name, std::string& host_path) const;
    void Serialize(StateSerializer& serializer);

private:
    bool m_available;
    bool m_selected;
    bool m_output_bit;
    bool m_last_clock;
    u8 m_iodir;
    u8 m_iodat;
    u8 m_address;
    u32 m_serial_data;
    u16 m_read_data;
    SerialState m_serial_state;
    bool m_audio_streaming;
    u32 m_audio_offset;
    std::string m_audio_file_name;
    std::string m_root_path;
    SdCardFileSystem* m_file_system;
    u8 m_mailbox[MAILBOX_SIZE];
    std::vector<u8> m_sram;
    std::vector<u8> m_audio_data;
};

#include "el_cheapo_sd_inline.h"

#endif /* EL_CHEAPO_SD_H */
