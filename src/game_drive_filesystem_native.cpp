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

#include <ctime>
#include <fstream>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif
#include "common.h"
#include "game_drive_filesystem.h"

static void BuildFatDateTime(time_t timestamp, u16& date, u16& time)
{
    struct tm local_time;
#if defined(_WIN32)
    localtime_s(&local_time, &timestamp);
#else
    localtime_r(&timestamp, &local_time);
#endif
    int year = MAX(1980, MIN(2107, local_time.tm_year + 1900));
    date = (u16)(((year - 1980) << 9) | ((local_time.tm_mon + 1) << 5) | local_time.tm_mday);
    time = (u16)((local_time.tm_hour << 11) | (local_time.tm_min << 5) | (local_time.tm_sec / 2));
}

class GameDriveFileSystemNative : public GameDriveFileSystem
{
public:
    GameDriveFileSystemNative() {}
    virtual ~GameDriveFileSystemNative() { CloseFile(); }

    virtual bool IsAvailable() const { return true; }
    virtual bool IsValidRootPath(const char* root_path) const { (void)root_path; return true; }
    virtual bool OpenFile(const char* path, bool& writable, u32& size);
    virtual void CloseFile();
    virtual s64 ReadFile(u32 offset, void* data, u32 size);
    virtual bool WriteFile(u32 offset, const void* data, u32 size);
    virtual bool ReadDirectory(const char* path, std::vector<GameDriveFileSystemEntry>& entries);

private:
    std::fstream m_file;
};

bool GameDriveFileSystemNative::OpenFile(const char* path, bool& writable, u32& size)
{
    CloseFile();

    open_ifstream_utf8(m_file, path, std::ios::in | std::ios::out | std::ios::binary);
    writable = m_file.is_open();

    if (!m_file.is_open())
    {
        m_file.clear();
        open_ifstream_utf8(m_file, path, std::ios::in | std::ios::binary);
        writable = false;
    }

    if (!m_file.is_open())
        return false;

    m_file.seekg(0, std::ios::end);
    std::streamoff file_size = m_file.tellg();
    if (file_size < 0 || (u64)file_size > 0xFFFFFFFFULL)
    {
        CloseFile();
        return false;
    }

    m_file.clear();
    m_file.seekg(0, std::ios::beg);
    size = (u32)file_size;
    return true;
}

void GameDriveFileSystemNative::CloseFile()
{
    if (m_file.is_open())
        m_file.close();

    m_file.clear();
}

s64 GameDriveFileSystemNative::ReadFile(u32 offset, void* data, u32 size)
{
    if (!m_file.is_open())
        return -1;

    m_file.clear();
    m_file.seekg(offset, std::ios::beg);
    if (!m_file.good())
        return -1;

    m_file.read(reinterpret_cast<char*>(data), size);
    return (s64)m_file.gcount();
}

bool GameDriveFileSystemNative::WriteFile(u32 offset, const void* data, u32 size)
{
    if (!m_file.is_open())
        return false;

    m_file.clear();
    m_file.seekp(offset, std::ios::beg);
    m_file.write(reinterpret_cast<const char*>(data), size);
    m_file.flush();
    return m_file.good();
}

bool GameDriveFileSystemNative::ReadDirectory(const char* path, std::vector<GameDriveFileSystemEntry>& entries)
{
    entries.clear();

#if defined(_WIN32)
    std::string pattern = path;
    append_path_component(pattern, "*");

    WIN32_FIND_DATAW find_data;
    std::wstring wide_pattern = utf8_to_wstring(pattern.c_str());
    HANDLE find = FindFirstFileW(wide_pattern.c_str(), &find_data);
    if (find == INVALID_HANDLE_VALUE)
        return false;

    do
    {
        std::string name = wstring_to_utf8(find_data.cFileName);
        if (name == "." || name == "..")
            continue;

        GameDriveFileSystemEntry entry = {};
        entry.name = name;
        entry.size = ((u64)find_data.nFileSizeHigh << 32) | find_data.nFileSizeLow;
        entry.directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entry.read_only = (find_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
        entry.hidden = (find_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;

        FILETIME local_file_time;
        SYSTEMTIME system_time;
        if (FileTimeToLocalFileTime(&find_data.ftLastWriteTime, &local_file_time) && FileTimeToSystemTime(&local_file_time, &system_time))
        {
            entry.date = (u16)(((MAX(1980, (int)system_time.wYear) - 1980) << 9) | (system_time.wMonth << 5) | system_time.wDay);
            entry.time = (u16)((system_time.wHour << 11) | (system_time.wMinute << 5) | (system_time.wSecond / 2));
        }

        entries.push_back(entry);
    }
    while (FindNextFileW(find, &find_data));

    FindClose(find);
#else
    DIR* directory = opendir(path);
    if (!directory)
        return false;

    struct dirent* directory_entry;
    while ((directory_entry = readdir(directory)) != NULL)
    {
        if (strcmp(directory_entry->d_name, ".") == 0 || strcmp(directory_entry->d_name, "..") == 0)
            continue;

        std::string item_path = path;
        append_path_component(item_path, directory_entry->d_name);

        struct stat status;
        if (stat(item_path.c_str(), &status) != 0)
            continue;

        GameDriveFileSystemEntry entry = {};
        entry.name = directory_entry->d_name;
        entry.size = S_ISDIR(status.st_mode) ? 0 : (u64)status.st_size;
        entry.directory = S_ISDIR(status.st_mode);
        entry.read_only = !(status.st_mode & S_IWUSR);
        entry.hidden = directory_entry->d_name[0] == '.';
        BuildFatDateTime(status.st_mtime, entry.date, entry.time);
        entries.push_back(entry);
    }

    closedir(directory);
#endif

    return true;
}

GameDriveFileSystem* CreateGameDriveFileSystem()
{
    return new GameDriveFileSystemNative();
}
