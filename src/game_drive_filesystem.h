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

#ifndef GAME_DRIVE_FILESYSTEM_H
#define GAME_DRIVE_FILESYSTEM_H

#include <string>
#include <vector>
#include "types.h"

struct GameDriveFileSystemEntry
{
    std::string name;
    u64 size;
    u16 date;
    u16 time;
    bool directory;
    bool read_only;
    bool hidden;
};

class GameDriveFileSystem
{
public:
    virtual ~GameDriveFileSystem() {}

    virtual bool IsAvailable() const = 0;
    virtual bool IsValidRootPath(const char* root_path) const = 0;
    virtual bool OpenFile(const char* path, bool& writable, u32& size) = 0;
    virtual void CloseFile() = 0;
    virtual s64 ReadFile(u32 offset, void* data, u32 size) = 0;
    virtual bool WriteFile(u32 offset, const void* data, u32 size) = 0;
    virtual bool ReadDirectory(const char* path, std::vector<GameDriveFileSystemEntry>& entries) = 0;
};

GameDriveFileSystem* CreateGameDriveFileSystem();

#endif /* GAME_DRIVE_FILESYSTEM_H */
