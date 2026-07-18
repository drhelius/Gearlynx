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

#include "game_drive_filesystem_libretro.h"
#include "game_drive_filesystem.h"
#include "libretro.h"
#include "common.h"

static const retro_vfs_interface* s_vfs_interface = NULL;

class GameDriveFileSystemLibretro : public GameDriveFileSystem
{
public:
    GameDriveFileSystemLibretro();
    virtual ~GameDriveFileSystemLibretro();

    virtual bool IsAvailable() const;
    virtual bool IsValidRootPath(const char* root_path) const;
    virtual bool OpenFile(const char* path, bool& writable, u32& size);
    virtual void CloseFile();
    virtual s64 ReadFile(u32 offset, void* data, u32 size);
    virtual bool WriteFile(u32 offset, const void* data, u32 size);
    virtual bool ReadDirectory(const char* path, std::vector<GameDriveFileSystemEntry>& entries);

private:
    const retro_vfs_interface* m_vfs_interface;
    retro_vfs_file_handle* m_file;
};

GameDriveFileSystemLibretro::GameDriveFileSystemLibretro()
{
    m_vfs_interface = s_vfs_interface;
    m_file = NULL;
}

GameDriveFileSystemLibretro::~GameDriveFileSystemLibretro()
{
    CloseFile();
}

bool GameDriveFileSystemLibretro::IsAvailable() const
{
    return m_vfs_interface && m_vfs_interface->open && m_vfs_interface->close &&
        m_vfs_interface->size && m_vfs_interface->seek && m_vfs_interface->read &&
        m_vfs_interface->opendir && m_vfs_interface->readdir &&
        m_vfs_interface->dirent_get_name && m_vfs_interface->dirent_is_dir &&
        m_vfs_interface->closedir;
}

bool GameDriveFileSystemLibretro::IsValidRootPath(const char* root_path) const
{
    return root_path && root_path[0];
}

bool GameDriveFileSystemLibretro::OpenFile(const char* path, bool& writable, u32& size)
{
    CloseFile();
    writable = false;
    size = 0;

    if (!IsAvailable())
        return false;

    if (m_vfs_interface->write && m_vfs_interface->flush)
    {
        unsigned mode = RETRO_VFS_FILE_ACCESS_READ_WRITE | RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING;
        m_file = m_vfs_interface->open(path, mode, RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS);
        writable = m_file != NULL;
    }

    if (!m_file)
        m_file = m_vfs_interface->open(path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS);

    if (!m_file)
        return false;

    s64 file_size = (s64)m_vfs_interface->size(m_file);
    if (file_size < 0 || (u64)file_size > 0xFFFFFFFFULL)
    {
        CloseFile();
        return false;
    }

    size = (u32)file_size;
    return true;
}

void GameDriveFileSystemLibretro::CloseFile()
{
    if (m_file && m_vfs_interface && m_vfs_interface->close)
        m_vfs_interface->close(m_file);

    m_file = NULL;
}

s64 GameDriveFileSystemLibretro::ReadFile(u32 offset, void* data, u32 size)
{
    if (!m_file || !m_vfs_interface || !m_vfs_interface->seek || !m_vfs_interface->read)
        return -1;

    if (m_vfs_interface->seek(m_file, offset, RETRO_VFS_SEEK_POSITION_START) < 0)
        return -1;

    return (s64)m_vfs_interface->read(m_file, data, size);
}

bool GameDriveFileSystemLibretro::WriteFile(u32 offset, const void* data, u32 size)
{
    if (!m_file || !m_vfs_interface || !m_vfs_interface->seek || !m_vfs_interface->write || !m_vfs_interface->flush)
        return false;

    if (m_vfs_interface->seek(m_file, offset, RETRO_VFS_SEEK_POSITION_START) < 0)
        return false;

    s64 written = (s64)m_vfs_interface->write(m_file, data, size);
    return written == size && m_vfs_interface->flush(m_file) == 0;
}

bool GameDriveFileSystemLibretro::ReadDirectory(const char* path, std::vector<GameDriveFileSystemEntry>& entries)
{
    entries.clear();

    if (!IsAvailable())
        return false;

    retro_vfs_dir_handle* directory = m_vfs_interface->opendir(path, true);
    if (!directory)
        return false;

    while (m_vfs_interface->readdir(directory))
    {
        const char* name = m_vfs_interface->dirent_get_name(directory);
        if (!name || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;

        GameDriveFileSystemEntry entry = {};
        entry.name = name;
        entry.directory = m_vfs_interface->dirent_is_dir(directory);
        entry.hidden = name[0] == '.';

        if (!entry.directory)
        {
            std::string item_path = path;
            append_path_component(item_path, name);
            retro_vfs_file_handle* file = m_vfs_interface->open(item_path.c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
            if (file)
            {
                s64 file_size = (s64)m_vfs_interface->size(file);
                if (file_size > 0)
                    entry.size = (u64)file_size;
                m_vfs_interface->close(file);
            }
        }

        entries.push_back(entry);
    }

    m_vfs_interface->closedir(directory);
    return true;
}

void game_drive_set_vfs_interface(const retro_vfs_interface* vfs_interface)
{
    s_vfs_interface = vfs_interface;
}

GameDriveFileSystem* CreateGameDriveFileSystem()
{
    return new GameDriveFileSystemLibretro();
}
