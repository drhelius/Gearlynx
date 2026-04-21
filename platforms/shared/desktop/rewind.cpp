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

#include "emu.h"
#include "config.h"
#include "gearlynx.h"

#define REWIND_IMPORT
#include "rewind.h"

static u8* buffer = NULL;
static size_t sizes[REWIND_MAX_SNAPSHOTS] = { 0 };
static int head = 0;
static int count = 0;
static int capacity = 0;
static int frame_accum = 0;
static bool active = false;

static int slot_at(int age);
static void refresh_capacity(void);

bool rewind_init(void)
{
    buffer = new (std::nothrow) u8[(size_t)REWIND_MAX_SNAPSHOTS * REWIND_MAX_STATE_SIZE];
    if (!IsValidPointer(buffer))
    {
        Log("Rewind: failed to allocate %zu bytes",
            (size_t)REWIND_MAX_SNAPSHOTS * REWIND_MAX_STATE_SIZE);
        return false;
    }

    Log("Rewind: allocated %.1f MB ring buffer",
        (double)REWIND_MAX_SNAPSHOTS * REWIND_MAX_STATE_SIZE / (1024.0 * 1024.0));

    rewind_reset();
    return true;
}

void rewind_destroy(void)
{
    SafeDeleteArray(buffer);
    capacity = 0;
    count = 0;
    head = 0;
    frame_accum = 0;
    active = false;
}

void rewind_reset(void)
{
    head = 0;
    count = 0;
    frame_accum = 0;
    active = false;
    for (int i = 0; i < REWIND_MAX_SNAPSHOTS; i++)
        sizes[i] = 0;
}

void rewind_push(void)
{
    if (!config_rewind.enabled)
        return;
    if (!IsValidPointer(buffer))
        return;
    if (emu_is_empty() || emu_is_paused())
        return;
    if (active)
        return;

    refresh_capacity();

    frame_accum++;
    if (frame_accum < config_rewind.frames_per_snapshot)
        return;
    frame_accum = 0;

    u8* slot = buffer + (size_t)head * REWIND_MAX_STATE_SIZE;
    size_t size = REWIND_MAX_STATE_SIZE;

    if (!emu_get_core()->SaveState(slot, size, false))
        return;

    sizes[head] = size;
    head = (head + 1) % capacity;
    if (count < capacity)
        count++;
}

bool rewind_pop(void)
{
    if (count == 0)
        return false;
    if (!IsValidPointer(buffer))
        return false;

    int idx = slot_at(0);
    const u8* slot = buffer + (size_t)idx * REWIND_MAX_STATE_SIZE;
    size_t size = sizes[idx];

    bool ok = emu_get_core()->LoadState(slot, size);

    head = idx;
    count--;
    return ok;
}

void rewind_set_active(bool a)
{
    active = a;
    if (!a)
        frame_accum = 0;
}

bool rewind_is_active(void)
{
    return active;
}

int rewind_get_snapshot_count(void)
{
    return count;
}

size_t rewind_get_memory_usage(void)
{
    return (size_t)REWIND_MAX_SNAPSHOTS * REWIND_MAX_STATE_SIZE;
}

bool rewind_seek(int age)
{
    if (age < 0 || age >= count)
        return false;
    if (!IsValidPointer(buffer))
        return false;

    int idx = slot_at(age);
    const u8* slot = buffer + (size_t)idx * REWIND_MAX_STATE_SIZE;
    size_t size = sizes[idx];

    return emu_get_core()->LoadState(slot, size);
}

int rewind_get_capacity(void)
{
    return capacity;
}

int rewind_get_frames_per_snapshot(void)
{
    return config_rewind.frames_per_snapshot;
}

static int slot_at(int age)
{
    int idx = head - 1 - age;
    while (idx < 0)
        idx += capacity;
    return idx;
}

static void refresh_capacity(void)
{
    int fps = config_rewind.frames_per_snapshot;
    if (fps < 1)
        fps = 1;

    int target = config_rewind.buffer_seconds * (60 / fps);
    if (target < 1)
        target = 1;
    if (target > REWIND_MAX_SNAPSHOTS)
        target = REWIND_MAX_SNAPSHOTS;

    if (target != capacity)
    {
        capacity = target;
        head = 0;
        count = 0;
    }
}
