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
#include <string.h>
#include "el_cheapo_sd.h"
#include "sd_card_filesystem.h"
#include "state_serializer.h"

ElCheapoSD::ElCheapoSD()
{
    m_file_system = CreateSdCardFileSystem();
    m_available = false;
    m_root_path = ".";
    Reset(true);
}

ElCheapoSD::~ElCheapoSD()
{
    m_file_system->CloseFile();
    SafeDelete(m_file_system);
}

void ElCheapoSD::Configure(bool available, const char* root_path, const u8* initial_data, u32 initial_size)
{
    m_root_path = (root_path && root_path[0]) ? root_path : ".";
    m_available = available && m_file_system && m_file_system->IsAvailable() && m_file_system->IsValidRootPath(root_path);
    Reset(true);

    if (m_available)
    {
        m_sram.resize(SRAM_SIZE, 0xFF);
        if (initial_data && initial_size > 0)
            memcpy(&m_sram[0], initial_data, MIN(initial_size, (u32)m_sram.size()));
    }
    else
    {
        std::vector<u8>().swap(m_sram);
    }
}

void ElCheapoSD::Reset(bool hard)
{
    if (m_file_system)
        m_file_system->CloseFile();

    m_selected = false;
    m_output_bit = true;
    m_last_clock = false;
    m_iodir = 0;
    m_iodat = 0;
    m_address = 0;
    m_serial_data = 0;
    m_read_data = 0;
    m_serial_state = SERIAL_NONE;
    m_audio_streaming = false;
    m_audio_offset = 0;
    m_audio_file_name.clear();
    m_audio_data.clear();
    memset(m_mailbox, 0, sizeof(m_mailbox));

    if (hard && !m_sram.empty())
        std::fill(m_sram.begin(), m_sram.end(), 0xFF);
}

void ElCheapoSD::ProcessCounter(u16 counter)
{
    bool selected = IS_SET_BIT(counter, 8);
    bool clock = IS_SET_BIT(counter, 1);
    bool data_in = IS_SET_BIT(m_iodir, 4) && IS_SET_BIT(m_iodat, 4);

    if (selected != m_selected)
    {
        m_selected = selected;
        m_serial_state = SERIAL_NONE;
        m_serial_data = 0;
        m_output_bit = true;
    }

    if (!selected || !clock || m_last_clock)
    {
        m_last_clock = clock;
        return;
    }

    m_last_clock = clock;

    if (m_serial_state == SERIAL_NONE)
    {
        if (data_in)
        {
            m_serial_data = 1;
            m_serial_state = SERIAL_ADDRESS;
        }
        return;
    }

    if (m_serial_state == SERIAL_READ)
    {
        m_output_bit = (m_read_data & 0x8000) != 0;
        m_read_data <<= 1;
        return;
    }

    m_serial_data = (m_serial_data << 1) | (data_in ? 1 : 0);

    if (m_serial_state == SERIAL_ADDRESS && (m_serial_data & 0x100))
    {
        u8 opcode = (u8)((m_serial_data >> 6) & 0x03);
        m_address = (u8)(m_serial_data & 0x3F);

        if (opcode == 0x02)
        {
            m_read_data = ReadMailboxWord(m_address);
            m_output_bit = false;
            m_serial_state = SERIAL_READ;
        }
        else if (opcode == 0x01)
        {
            m_serial_data = 1;
            m_serial_state = SERIAL_WRITE;
        }
        else
        {
            m_serial_state = SERIAL_NONE;
        }
    }
    else if (m_serial_state == SERIAL_WRITE && (m_serial_data & 0x10000))
    {
        WriteMailboxWord(m_address, (u16)m_serial_data);
        m_serial_state = SERIAL_NONE;
    }
}

void ElCheapoSD::ExecuteCommand()
{
    Debug("ElCheapoSD command: %02X %02X %02X %02X", m_mailbox[0], m_mailbox[1], m_mailbox[2], m_mailbox[3]);

    if (memcmp(m_mailbox, "NEW", 3) == 0)
        ExecuteNew();
    else if (memcmp(m_mailbox, "LOAD", 4) == 0)
        ExecuteLoad();
    else if (memcmp(m_mailbox, "SAVE", 4) == 0)
        ExecuteSave();
    else if (m_mailbox[0] == '&' && m_mailbox[1] == 'F')
        ExecuteAudioSetFile();
    else if (m_mailbox[0] == '&' && m_mailbox[1] == 'P')
        ExecuteAudioPlay();
    else if (m_mailbox[0] == '&' && m_mailbox[1] == 'S')
        ExecuteAudioStop();
    else
        SetResponse("ERROR!");
}

void ElCheapoSD::ExecuteNew()
{
    std::string file_name = GetMailboxFileName(3);
    std::string host_path;
    bool directory = false;
    u32 existing_size = 0;

    if (ResolveExistingPath(file_name, host_path, directory, existing_size))
    {
        SetResponse("EXISTS");
        return;
    }

    BuildHostPath(file_name, host_path);
    u32 size = read_u32_be(&m_mailbox[14]);
    SetResponse(m_file_system->CreateSizedFile(host_path.c_str(), size) ? "DONE" : "NOSPACE");
}

void ElCheapoSD::ExecuteLoad()
{
    std::string file_name = GetMailboxFileName(4);
    std::string host_path;
    bool directory = false;
    u32 file_size = 0;

    Debug("ElCheapoSD LOAD: %s", file_name.c_str());

    if (!ResolveExistingPath(file_name, host_path, directory, file_size))
    {
        SetResponse("NOFILE");
        return;
    }

    if (directory)
    {
        SetResponse("FOLDER");
        return;
    }

    u32 destination = read_u32_be(&m_mailbox[15]);
    u32 offset = read_u32_be(&m_mailbox[19]);
    u32 length = read_u32_be(&m_mailbox[23]);

    Debug("ElCheapoSD LOAD: destination=%08X offset=%u length=%u size=%u", destination, offset, length, file_size);

    if (offset > file_size)
    {
        SetResponse("EOF");
        return;
    }

    if (length == 0)
        length = file_size - offset;

    if (length > file_size - offset || destination > SRAM_SIZE || length > SRAM_SIZE - destination)
    {
        SetResponse("EOF");
        return;
    }

    bool writable = false;
    u32 open_size = 0;
    if (!m_file_system->OpenFile(host_path.c_str(), writable, open_size))
    {
        SetResponse("NOFILE");
        return;
    }

    s64 bytes_read = m_file_system->ReadFile(offset, &m_sram[destination], length);
    m_file_system->CloseFile();

    SetResponse(bytes_read == length ? "DONE" : "EOF");
}

void ElCheapoSD::ExecuteSave()
{
    std::string file_name = GetMailboxFileName(4);
    std::string host_path;
    bool directory = false;
    u32 file_size = 0;

    if (!ResolveExistingPath(file_name, host_path, directory, file_size))
    {
        SetResponse("NOFILE");
        return;
    }

    if (directory)
    {
        SetResponse("FOLDER");
        return;
    }

    u32 offset = read_u32_be(&m_mailbox[15]);
    u32 length = m_mailbox[19];
    if (length > MAX_SAVE_SIZE || offset > file_size || length > file_size - offset)
    {
        SetResponse("EOF");
        return;
    }

    bool writable = false;
    u32 open_size = 0;
    if (!m_file_system->OpenFile(host_path.c_str(), writable, open_size))
    {
        SetResponse("NOFILE");
        return;
    }

    bool success = writable && m_file_system->WriteFile(offset, &m_mailbox[20], length);
    m_file_system->CloseFile();
    SetResponse(success ? "DONE" : "ERROR!");
}

void ElCheapoSD::ExecuteAudioSetFile()
{
    std::string file_name = GetMailboxFileName(2);
    SetResponse(LoadAudioFile(file_name) ? "DONE" : "NOFILE");
}

void ElCheapoSD::ExecuteAudioPlay()
{
    m_audio_offset = 0;
    m_audio_streaming = !m_audio_data.empty();
}

void ElCheapoSD::ExecuteAudioStop()
{
    m_audio_streaming = false;
    SetResponse("DONE");
}

bool ElCheapoSD::LoadAudioFile(const std::string& file_name)
{
    std::string host_path;
    bool directory = false;
    u32 file_size = 0;

    m_audio_streaming = false;
    m_audio_offset = 0;
    m_audio_file_name.clear();
    m_audio_data.clear();

    if (!ResolveExistingPath(file_name, host_path, directory, file_size) || directory || file_size > MAX_AUDIO_SIZE)
        return false;

    bool writable = false;
    u32 open_size = 0;
    if (!m_file_system->OpenFile(host_path.c_str(), writable, open_size))
        return false;

    m_audio_data.resize(file_size);
    s64 bytes_read = file_size > 0 ? m_file_system->ReadFile(0, &m_audio_data[0], file_size) : 0;
    m_file_system->CloseFile();

    if (bytes_read != file_size)
    {
        m_audio_data.clear();
        return false;
    }

    m_audio_file_name = file_name;
    return true;
}

void ElCheapoSD::SetResponse(const char* response)
{
    Debug("ElCheapoSD response: %s", response);
    memset(m_mailbox, 0, sizeof(m_mailbox));
    strncpy_fit(reinterpret_cast<char*>(m_mailbox), response, MAILBOX_SIZE - 1);
    m_mailbox[MAILBOX_SIZE - 2] = 0xAA;
    m_mailbox[MAILBOX_SIZE - 1] = 0x55;
}

u16 ElCheapoSD::ReadMailboxWord(u8 address) const
{
    u32 offset = (address & (MAILBOX_WORDS - 1)) * 2;
    return read_u16_be(&m_mailbox[offset]);
}

void ElCheapoSD::WriteMailboxWord(u8 address, u16 value)
{
    u32 offset = (address & (MAILBOX_WORDS - 1)) * 2;
    m_mailbox[offset] = hi(value);
    m_mailbox[offset + 1] = lo(value);

    if (address == MAILBOX_STATUS_WORD && value == 0x0055)
        ExecuteCommand();
}

std::string ElCheapoSD::GetMailboxFileName(u32 offset) const
{
    std::string base(reinterpret_cast<const char*>(&m_mailbox[offset]), 8);
    std::string extension(reinterpret_cast<const char*>(&m_mailbox[offset + 8]), 3);

    while (!base.empty() && (base[base.size() - 1] == ' ' || base[base.size() - 1] == 0))
        base.resize(base.size() - 1);
    while (!extension.empty() && (extension[extension.size() - 1] == ' ' || extension[extension.size() - 1] == 0))
        extension.resize(extension.size() - 1);

    if (!extension.empty())
        return base + "." + extension;
    return base;
}

bool ElCheapoSD::ResolveExistingPath(const std::string& file_name, std::string& host_path, bool& directory, u32& size)
{
    BuildHostPath(file_name, host_path);
    if (m_file_system->GetFileInfo(host_path.c_str(), directory, size))
        return true;

    std::vector<SdCardFileSystemEntry> entries;
    if (!m_file_system->ReadDirectory(m_root_path.c_str(), entries))
        return false;

    for (size_t index = 0; index < entries.size(); index++)
    {
        if (strings_equal_ignore_case(entries[index].name, file_name))
        {
            BuildHostPath(entries[index].name, host_path);
            directory = entries[index].directory;
            size = (u32)MIN(entries[index].size, 0xFFFFFFFFULL);
            return true;
        }
    }

    return false;
}

void ElCheapoSD::BuildHostPath(const std::string& file_name, std::string& host_path) const
{
    host_path = m_root_path;
    append_path_component(host_path, file_name.c_str());
}

void ElCheapoSD::SaveState(std::ostream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void ElCheapoSD::LoadState(std::istream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer);
}

void ElCheapoSD::Serialize(StateSerializer& serializer)
{
    s32 serial_state = (s32)m_serial_state;

    G_SERIALIZE(serializer, m_selected);
    G_SERIALIZE(serializer, m_output_bit);
    G_SERIALIZE(serializer, m_last_clock);
    G_SERIALIZE(serializer, m_iodir);
    G_SERIALIZE(serializer, m_iodat);
    G_SERIALIZE(serializer, m_address);
    G_SERIALIZE(serializer, m_serial_data);
    G_SERIALIZE(serializer, m_read_data);
    G_SERIALIZE(serializer, serial_state);
    G_SERIALIZE_ARRAY(serializer, m_mailbox, sizeof(m_mailbox));
    G_SERIALIZE_ARRAY(serializer, &m_sram[0], m_sram.size());
    G_SERIALIZE(serializer, m_audio_streaming);
    G_SERIALIZE(serializer, m_audio_offset);
    serializer.SerializeString(m_audio_file_name);

    m_serial_state = (SerialState)serial_state;

    if (serializer.IsLoading())
    {
        bool restore_audio = m_audio_streaming;
        u32 restore_offset = m_audio_offset;
        std::string restore_file_name = m_audio_file_name;

        m_audio_streaming = false;
        m_audio_offset = 0;
        m_audio_file_name.clear();
        m_audio_data.clear();

        if (!restore_file_name.empty() && LoadAudioFile(restore_file_name))
        {
            m_audio_offset = MIN(restore_offset, (u32)m_audio_data.size());
            m_audio_streaming = restore_audio;
        }
    }
}

