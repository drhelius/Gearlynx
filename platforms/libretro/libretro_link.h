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

#ifndef LIBRETRO_LINK_H
#define LIBRETRO_LINK_H

#include "../../src/gearlynx_core.h"
#include "libretro.h"

#define GEARLYNX_LINK_MAX_PLAYERS 4
#define GEARLYNX_LINK_SUBSYSTEM_2 0x101
#define GEARLYNX_LINK_SUBSYSTEM_3 0x102
#define GEARLYNX_LINK_SUBSYSTEM_4 0x103
#define GEARLYNX_LINK_RAM_1 ((1 << 8) | RETRO_MEMORY_SAVE_RAM)
#define GEARLYNX_LINK_RAM_2 ((2 << 8) | RETRO_MEMORY_SAVE_RAM)
#define GEARLYNX_LINK_RAM_3 ((3 << 8) | RETRO_MEMORY_SAVE_RAM)
#define GEARLYNX_LINK_RAM_4 ((4 << 8) | RETRO_MEMORY_SAVE_RAM)

enum LibretroLinkPlacement
{
    LINK_HORIZONTAL,
    LINK_VERTICAL,
    LINK_GRID
};

struct LibretroInstance
{
    GearlynxCore* core;
    u16 frame_buffer[GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT];
    s16 audio_buffer[GLYNX_AUDIO_BUFFER_SIZE];
    int sample_count;
};

class LibretroLink
{
public:
    LibretroLink(LibretroInstance* instances, unsigned count);
    ~LibretroLink();
    void Reset();
    void RunFrame();
    void Geometry(int placement, int selection, unsigned* width, unsigned* height);
    const u16* Video(int placement, bool switched, int selection);
    const s16* Audio(int selection, int* count);
    void SetSavePath(const char* content_path, const char* save_directory);
    void PersistentMemory(bool write, const retro_vfs_interface* vfs);
    bool PersistentMemory(const char* path, unsigned index, bool write, const retro_vfs_interface* vfs);

private:
    struct Frame
    {
        u64 start_cycle;
        u32 bit_cycles;
        u16 bits;
    };

    struct Break
    {
        u64 cycle;
        bool asserted;
    };

    struct Runtime
    {
        u64 frame_cycle;
        u64 origin[GEARLYNX_LINK_MAX_PLAYERS];
        Frame frames[GEARLYNX_LINK_MAX_PLAYERS][16];
        Break breaks[GEARLYNX_LINK_MAX_PLAYERS][16];
        u32 frame_count[GEARLYNX_LINK_MAX_PLAYERS];
        u32 break_count[GEARLYNX_LINK_MAX_PLAYERS];
    };

    struct Endpoint
    {
        LibretroLink* link;
        unsigned index;
    };

    static void PublishCallback(u64 start_cycle, u32 bit_cycles, u16 bits, void* data);
    static bool SampleCallback(u64 cycle, void* data);
    static void BreakCallback(bool asserted, u64 cycle, void* data);
    u64 Cycle(unsigned index);
    void Layout(int placement, unsigned* columns, unsigned* rows, unsigned* width, unsigned* height);

    LibretroInstance* m_instances;
    unsigned m_count;
    Endpoint m_endpoint[GEARLYNX_LINK_MAX_PLAYERS];
    Runtime m_runtime;
    u16 m_video[GEARLYNX_LINK_MAX_PLAYERS * GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_WIDTH];
    s16 m_mix[GLYNX_AUDIO_BUFFER_SIZE];
    char m_save_path[4096];
};

#endif /* LIBRETRO_LINK_H */
