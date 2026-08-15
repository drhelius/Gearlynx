/*
 * Gearlynx - Lynx Emulator
 * Copyright (C) 2025  Ignacio Sanchez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <new>
#include <thread>
#if defined(_WIN32)
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "comlynx_manager.h"
#include "comlynx_wire.h"
#include "log.h"

#define COMLYNX_SHM_MAGIC 0x584C4347
#define COMLYNX_SHM_VERSION 1
#define COMLYNX_SHARED_FRAME_COUNT 64
#define COMLYNX_BARRIER_SPIN_US 250
#define COMLYNX_BARRIER_SLEEP_US 100
#define COMLYNX_TURBO_MAINTENANCE_CYCLES 4096

struct ComLynxManager::Shared
{
    struct Frame
    {
        std::atomic<u32> sequence;
        u32 generation;
        u64 start_cycle;
        u32 bit_cycles;
        u16 bits;

        Frame() : sequence(0), generation(0), start_cycle(0), bit_cycles(0), bits(0) {}
    };

    struct Peer
    {
        std::atomic<u32> state;
        std::atomic<u32> generation;
        std::atomic<u64> heartbeat_us;
        std::atomic<u64> promise_cycle;
        std::atomic<u64> break_from;
        std::atomic<u32> write_index;
        Frame frames[COMLYNX_SHARED_FRAME_COUNT];

        Peer() : state(0), generation(0), heartbeat_us(0), promise_cycle(0),
            break_from(0), write_index(0) {}
    };

    std::atomic<u32> magic;
    u32 version;
    u8 session;
    u8 reserved[3];
    Peer peers[COMLYNX_MAX_PEERS];

    Shared() : magic(0), version(COMLYNX_SHM_VERSION), session(0)
    {
        memset(reserved, 0, sizeof(reserved));
    }
};

ComLynxManager::ComLynxManager()
{
    m_shared = NULL;
    m_mapping_handle = NULL;
    m_mapping_fd = -1;
    m_slot = -1;
    m_generation = 0;
    m_session = 0;
    m_local_anchor = 0;
    m_bus_anchor = 0;
    m_last_sync_exit_us = 0;
    m_turbo_next_maintenance_cycle = 0;
    memset(&m_status, 0, sizeof(m_status));
    m_status.mode = ComLynxModeDisabled;
}

ComLynxManager::~ComLynxManager()
{
    Stop();
}

bool ComLynxManager::Connect(u8 session, u64 local_cycle)
{
    Stop();

    if (session == 0)
    {
        SetFault("ComLynx session must be between 1 and 255");
        return false;
    }

    if (!Map(session))
        return false;

    m_session = session;

    if (!ClaimSlot(local_cycle, false))
    {
        Unmap();
        SetFault("ComLynx session is full");
        return false;
    }

    memset(&m_status, 0, sizeof(m_status));
    m_status.mode = ComLynxModeConnected;
    m_status.cable_connected = true;
    m_status.session = session;

    snprintf(m_status.endpoint, sizeof(m_status.endpoint), "Shared session %u", session);

    RefreshStatus();

    Log("ComLynx: connected to shared session %u as peer %u", session, m_slot + 1);

    return true;
}

void ComLynxManager::Stop()
{
    if (m_shared && m_slot >= 0)
    {
        Shared::Peer& peer = m_shared->peers[m_slot];
        if (peer.generation.load(std::memory_order_acquire) == m_generation)
            peer.state.store(0, std::memory_order_release);
    }

    Unmap();

    m_slot = -1;
    m_generation = 0;
    m_session = 0;
    m_turbo_next_maintenance_cycle = 0;

    memset(&m_status, 0, sizeof(m_status));
    m_status.mode = ComLynxModeDisabled;
}

void ComLynxManager::PublishFrame(u64 local_start_cycle, u32 bit_cycles, u16 bits)
{
    if (!EnsureAttached(local_start_cycle) || bit_cycles == 0)
        return;

    Shared::Peer& peer = m_shared->peers[m_slot];
    peer.heartbeat_us.store(GetClockMicroseconds(), std::memory_order_release);

    u32 index = peer.write_index.load(std::memory_order_relaxed);
    Shared::Frame& frame = peer.frames[index % COMLYNX_SHARED_FRAME_COUNT];
    u32 sequence = frame.sequence.load(std::memory_order_relaxed);

    frame.sequence.store(sequence + 1, std::memory_order_release);
    frame.generation = m_generation;
    frame.start_cycle = ToBusCycle(local_start_cycle);
    frame.bit_cycles = bit_cycles;
    frame.bits = bits & 0x07FF;
    frame.sequence.store(sequence + 2, std::memory_order_release);

    peer.write_index.store(index + 1, std::memory_order_release);

    u64 frame_end = frame.start_cycle + (u64)bit_cycles * COMLYNX_FRAME_BITS;
    u64 promise = peer.promise_cycle.load(std::memory_order_relaxed);

    if (frame_end > promise)
        peer.promise_cycle.store(frame_end, std::memory_order_release);

    m_status.frames_published++;
}

void ComLynxManager::SetBreak(bool asserted, u64 local_cycle)
{
    if (!EnsureAttached(local_cycle))
        return;

    Shared::Peer& peer = m_shared->peers[m_slot];
    peer.heartbeat_us.store(GetClockMicroseconds(), std::memory_order_release);
    peer.break_from.store(asserted ? MAX((u64)1, ToBusCycle(local_cycle)) : 0, std::memory_order_release);
}

bool ComLynxManager::SampleLine(u64 local_cycle)
{
    if (!EnsureAttached(local_cycle))
        return true;

    m_shared->peers[m_slot].heartbeat_us.store(GetClockMicroseconds(), std::memory_order_release);

    u64 cycle = ToBusCycle(local_cycle);
    u64 now = GetClockMicroseconds();

    bool level = true;

    for (int i = 0; i < COMLYNX_MAX_PEERS && level; i++)
    {
        if (i == m_slot)
            continue;

        Shared::Peer& peer = m_shared->peers[i];
        u64 heartbeat = peer.heartbeat_us.load(std::memory_order_acquire);

        if (peer.state.load(std::memory_order_acquire) != 1 || comlynx_heartbeat_age(now, heartbeat) > COMLYNX_DETACH_US)
            continue;

        u64 break_from = peer.break_from.load(std::memory_order_acquire);

        if (break_from != 0 && cycle >= break_from)
        {
            level = false;
            break;
        }

        u32 generation = peer.generation.load(std::memory_order_acquire);
        u32 write_index = peer.write_index.load(std::memory_order_acquire);
        u32 count = MIN(write_index, (u32)COMLYNX_SHARED_FRAME_COUNT);

        for (u32 offset = 0; offset < count; offset++)
        {
            Shared::Frame& source = peer.frames[(write_index - 1 - offset) % COMLYNX_SHARED_FRAME_COUNT];

            u32 before = source.sequence.load(std::memory_order_acquire);

            if ((before & 1) != 0)
                continue;

            ComLynxWireFrame frame;

            u32 frame_generation = source.generation;

            frame.start_cycle = source.start_cycle;
            frame.bit_cycles = source.bit_cycles;
            frame.bits = source.bits;

            u32 after = source.sequence.load(std::memory_order_acquire);

            if (before != after || (after & 1) != 0 || frame_generation != generation)
                continue;

            if (cycle >= frame.start_cycle + (u64)frame.bit_cycles * COMLYNX_FRAME_BITS)
                break;

            if (!comlynx_frame_level(frame, cycle))
            {
                level = false;
                break;
            }
        }
    }

    m_status.line_samples++;

    if (!level)
        m_status.low_samples++;

    return level;
}

bool ComLynxManager::SampleLineTurbo(u64 local_cycle)
{
    if (!EnsureAttached(local_cycle))
        return true;

    u64 cycle = ToBusCycle(local_cycle);
    bool level = true;

    for (int i = 0; i < COMLYNX_MAX_PEERS && level; i++)
    {
        if (i == m_slot)
            continue;

        Shared::Peer& peer = m_shared->peers[i];

        if (peer.state.load(std::memory_order_acquire) != 1)
            continue;

        u64 break_from = peer.break_from.load(std::memory_order_acquire);

        if (break_from != 0 && cycle >= break_from)
        {
            level = false;
            break;
        }

        u32 generation = peer.generation.load(std::memory_order_acquire);
        u32 write_index = peer.write_index.load(std::memory_order_acquire);
        u32 count = MIN(write_index, (u32)COMLYNX_SHARED_FRAME_COUNT);

        for (u32 offset = 0; offset < count; offset++)
        {
            Shared::Frame& source = peer.frames[(write_index - 1 - offset) % COMLYNX_SHARED_FRAME_COUNT];
            u32 before = source.sequence.load(std::memory_order_acquire);

            if ((before & 1) != 0)
                continue;

            ComLynxWireFrame frame;
            u32 frame_generation = source.generation;
            frame.start_cycle = source.start_cycle;
            frame.bit_cycles = source.bit_cycles;
            frame.bits = source.bits;
            u32 after = source.sequence.load(std::memory_order_acquire);

            if (before != after || (after & 1) != 0 || frame_generation != generation)
                continue;

            if (cycle >= frame.start_cycle + (u64)frame.bit_cycles * COMLYNX_FRAME_BITS)
                break;

            if (!comlynx_frame_level(frame, cycle))
            {
                level = false;
                break;
            }
        }
    }

    m_status.line_samples++;

    if (!level)
        m_status.low_samples++;

    return level;
}

void ComLynxManager::Synchronize(u64 local_cycle, u32 promise_cycles)
{
    if (!EnsureAttached(local_cycle))
        return;

    u64 now = GetClockMicroseconds();

    if (m_last_sync_exit_us != 0)
    {
        u64 gap = now - m_last_sync_exit_us;
        m_status.sync_gap_max_us = MAX(m_status.sync_gap_max_us, gap);
        if (gap >= 50000)
            m_status.sync_gap_over_50ms++;
    }

    ReapStalePeers(now);

    Shared::Peer& local = m_shared->peers[m_slot];

    u64 cycle = ToBusCycle(local_cycle);

    local.heartbeat_us.store(now, std::memory_order_release);
    local.promise_cycle.store(cycle + promise_cycles, std::memory_order_release);

    u64 wait_started = 0;
    u64 progress_time = now;
    u64 previous_floor = 0;

    for (;;)
    {
        u64 floor = ~0ULL;

        for (int i = 0; i < COMLYNX_MAX_PEERS; i++)
        {
            Shared::Peer& peer = m_shared->peers[i];

            if (peer.state.load(std::memory_order_acquire) == 1)
                floor = MIN(floor, peer.promise_cycle.load(std::memory_order_acquire));
        }

        if (floor == ~0ULL || cycle <= floor)
            break;

        if (wait_started == 0)
        {
            wait_started = GetClockMicroseconds();
            m_status.barrier_waits++;
        }

        now = GetClockMicroseconds();

        if (floor != previous_floor)
        {
            previous_floor = floor;
            progress_time = now;
        }

        ReapStalePeers(now);

        local.heartbeat_us.store(now, std::memory_order_release);

        if (now - progress_time >= COMLYNX_BARRIER_SPIN_US)
            std::this_thread::sleep_for(std::chrono::microseconds(COMLYNX_BARRIER_SLEEP_US));
        else
            std::this_thread::yield();
    }

    now = GetClockMicroseconds();

    if (wait_started != 0)
    {
        u64 wait = now - wait_started;

        m_status.barrier_wait_us += wait;
        m_status.barrier_wait_max_us = MAX(m_status.barrier_wait_max_us, wait);

        if (wait >= 1000)
            m_status.barrier_wait_over_1ms++;
        if (wait >= 10000)
            m_status.barrier_wait_over_10ms++;
        if (wait >= 50000)
            m_status.barrier_wait_over_50ms++;
    }

    m_last_sync_exit_us = now;
}

void ComLynxManager::SynchronizeTurbo(u64 local_cycle)
{
    if (!EnsureAttached(local_cycle))
        return;

    MaintainTurbo(local_cycle);

    Shared::Peer& local = m_shared->peers[m_slot];
    u64 cycle = ToBusCycle(local_cycle);
    local.promise_cycle.store(cycle + COMLYNX_TURBO_PROMISE_CYCLES, std::memory_order_release);

    u64 floor = ~0ULL;

    for (int i = 0; i < COMLYNX_MAX_PEERS; i++)
    {
        Shared::Peer& peer = m_shared->peers[i];

        if (peer.state.load(std::memory_order_acquire) == 1)
            floor = MIN(floor, peer.promise_cycle.load(std::memory_order_acquire));
    }

    if (floor == ~0ULL || cycle <= floor)
        return;

    u64 wait_started = GetClockMicroseconds();
    u64 progress_time = wait_started;
    u64 previous_floor = floor;
    m_status.barrier_waits++;

    for (;;)
    {
        floor = ~0ULL;

        for (int i = 0; i < COMLYNX_MAX_PEERS; i++)
        {
            Shared::Peer& peer = m_shared->peers[i];

            if (peer.state.load(std::memory_order_acquire) == 1)
                floor = MIN(floor, peer.promise_cycle.load(std::memory_order_acquire));
        }

        if (floor == ~0ULL || cycle <= floor)
            break;

        u64 now = GetClockMicroseconds();

        if (floor != previous_floor)
        {
            previous_floor = floor;
            progress_time = now;
        }

        ReapStalePeers(now);
        local.heartbeat_us.store(now, std::memory_order_release);

        if (now - progress_time >= COMLYNX_BARRIER_SPIN_US)
            std::this_thread::sleep_for(std::chrono::microseconds(COMLYNX_BARRIER_SLEEP_US));
        else
            std::this_thread::yield();
    }

    u64 now = GetClockMicroseconds();
    u64 wait = now - wait_started;

    m_status.barrier_wait_us += wait;
    m_status.barrier_wait_max_us = MAX(m_status.barrier_wait_max_us, wait);

    if (wait >= 1000)
        m_status.barrier_wait_over_1ms++;
    if (wait >= 10000)
        m_status.barrier_wait_over_10ms++;
    if (wait >= 50000)
        m_status.barrier_wait_over_50ms++;

    m_last_sync_exit_us = now;
}

bool ComLynxManager::IsActive() const
{
    return m_shared != NULL && m_slot >= 0;
}

bool ComLynxManager::IsCableConnected() const
{
    return IsActive();
}

bool ComLynxManager::IsPacingPeer() const
{
    if (!IsActive())
        return false;

    for (int i = 0; i < m_slot; i++)
    {
        if (m_shared->peers[i].state.load(std::memory_order_acquire) == 1)
            return false;
    }

    return true;
}

ComLynxStatus ComLynxManager::GetStatus()
{
    RefreshStatus();
    return m_status;
}

void ComLynxManager::ResetMetrics()
{
    m_status.frames_published = 0;
    m_status.line_samples = 0;
    m_status.low_samples = 0;
    m_status.barrier_waits = 0;
    m_status.barrier_wait_us = 0;
    m_status.barrier_wait_max_us = 0;
    m_status.barrier_wait_over_1ms = 0;
    m_status.barrier_wait_over_10ms = 0;
    m_status.barrier_wait_over_50ms = 0;
    m_status.sync_gap_max_us = 0;
    m_status.sync_gap_over_50ms = 0;
    m_status.peer_detaches = 0;
    m_status.peer_detach_max_age_us = 0;
    m_status.reattachments = 0;
    m_last_sync_exit_us = GetClockMicroseconds();
}

bool ComLynxManager::Map(u8 session)
{
    char name[64];
    bool created;

#if defined(_WIN32)
    snprintf(name, sizeof(name), "Local\\gearlynx-comlynx-%u", session);

    HANDLE mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)sizeof(Shared), name);

    if (!mapping)
    {
        SetFault("Failed to create ComLynx shared memory");
        return false;
    }

    created = GetLastError() != ERROR_ALREADY_EXISTS;
    void* address = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));

    if (!address)
    {
        CloseHandle(mapping);
        SetFault("Failed to map ComLynx shared memory");
        return false;
    }

    m_mapping_handle = mapping;
    m_shared = (Shared*)address;
#else
    snprintf(name, sizeof(name), "/gearlynx-comlynx-%u", session);

    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    created = fd >= 0;

    if (!created && errno == EEXIST)
        fd = shm_open(name, O_RDWR, 0600);

    if (fd < 0)
    {
        SetFault("Failed to open ComLynx shared memory");
        return false;
    }

    if (created && ftruncate(fd, sizeof(Shared)) != 0)
    {
        close(fd);
        shm_unlink(name);
        SetFault("Failed to size ComLynx shared memory");
        return false;
    }

    if (!created)
    {
        u64 started = GetClockMicroseconds();
        struct stat status;

        while (fstat(fd, &status) != 0 || status.st_size < (off_t)sizeof(Shared))
        {
            if (GetClockMicroseconds() - started > COMLYNX_DETACH_US)
            {
                close(fd);
                SetFault("ComLynx shared memory sizing timed out");
                return false;
            }

            std::this_thread::yield();
        }
    }

    void* address = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (address == MAP_FAILED)
    {
        close(fd);
        SetFault("Failed to map ComLynx shared memory");
        return false;
    }

    m_mapping_fd = fd;
    m_shared = (Shared*)address;
#endif

    if (created)
    {
        new (m_shared) Shared();
        m_shared->session = session;
        m_shared->magic.store(COMLYNX_SHM_MAGIC, std::memory_order_release);
    }
    else
    {
        u64 started = GetClockMicroseconds();

        while (m_shared->magic.load(std::memory_order_acquire) != COMLYNX_SHM_MAGIC)
        {
            if (GetClockMicroseconds() - started > COMLYNX_DETACH_US)
            {
                Unmap();
                SetFault("ComLynx shared memory initialization timed out");
                return false;
            }

            std::this_thread::yield();
        }

        if (m_shared->version != COMLYNX_SHM_VERSION || m_shared->session != session)
        {
            Unmap();
            SetFault("Incompatible ComLynx shared memory");
            return false;
        }
    }
    return true;
}

void ComLynxManager::Unmap()
{
    if (!m_shared)
        return;

#if defined(_WIN32)
    UnmapViewOfFile(m_shared);

    if (m_mapping_handle)
        CloseHandle((HANDLE)m_mapping_handle);
#else
    munmap(m_shared, sizeof(Shared));

    if (m_mapping_fd >= 0)
        close(m_mapping_fd);
#endif

    m_shared = NULL;
    m_mapping_handle = NULL;
    m_mapping_fd = -1;
}

bool ComLynxManager::ClaimSlot(u64 local_cycle, bool reattach)
{
    u64 now = GetClockMicroseconds();

    ReapStalePeers(now, true);

    u64 bus_anchor = 0;

    for (int i = 0; i < COMLYNX_MAX_PEERS; i++)
    {
        Shared::Peer& peer = m_shared->peers[i];
        if (peer.state.load(std::memory_order_acquire) == 1)
            bus_anchor = MAX(bus_anchor, peer.promise_cycle.load(std::memory_order_acquire));
    }

    for (int i = 0; i < COMLYNX_MAX_PEERS; i++)
    {
        Shared::Peer& peer = m_shared->peers[i];

        u32 expected = 0;
        if (!peer.state.compare_exchange_strong(expected, 2, std::memory_order_acq_rel))
            continue;

        m_slot = i;
        m_generation = peer.generation.fetch_add(1, std::memory_order_acq_rel) + 1;
        m_local_anchor = local_cycle;
        m_bus_anchor = bus_anchor;
        m_turbo_next_maintenance_cycle = local_cycle + COMLYNX_TURBO_MAINTENANCE_CYCLES;

        peer.write_index.store(0, std::memory_order_relaxed);
        peer.break_from.store(0, std::memory_order_relaxed);
        peer.promise_cycle.store(bus_anchor + COMLYNX_MAX_PROMISE_CYCLES, std::memory_order_relaxed);
        peer.heartbeat_us.store(now, std::memory_order_relaxed);
        peer.state.store(1, std::memory_order_release);

        if (reattach)
            m_status.reattachments++;

        return true;
    }

    return false;
}

bool ComLynxManager::EnsureAttached(u64 local_cycle)
{
    if (!m_shared)
        return false;

    if (m_slot >= 0)
    {
        Shared::Peer& peer = m_shared->peers[m_slot];
        if (peer.state.load(std::memory_order_acquire) == 1 &&
            peer.generation.load(std::memory_order_acquire) == m_generation)
            return true;
    }

    return ClaimSlot(local_cycle, true);
}

void ComLynxManager::MaintainTurbo(u64 local_cycle)
{
    if (local_cycle < m_turbo_next_maintenance_cycle)
        return;

    u64 now = GetClockMicroseconds();

    if (m_last_sync_exit_us != 0)
    {
        u64 gap = now - m_last_sync_exit_us;
        m_status.sync_gap_max_us = MAX(m_status.sync_gap_max_us, gap);

        if (gap >= 50000)
            m_status.sync_gap_over_50ms++;
    }

    ReapStalePeers(now);

    if (m_slot >= 0)
        m_shared->peers[m_slot].heartbeat_us.store(now, std::memory_order_release);

    m_turbo_next_maintenance_cycle = local_cycle + COMLYNX_TURBO_MAINTENANCE_CYCLES;
    m_last_sync_exit_us = now;
}

void ComLynxManager::ReapStalePeers(u64 now_us, bool preserve_idle)
{
    for (int i = 0; i < COMLYNX_MAX_PEERS; i++)
    {
        if (i == m_slot)
            continue;

        Shared::Peer& peer = m_shared->peers[i];

        u64 heartbeat = peer.heartbeat_us.load(std::memory_order_acquire);
        u32 generation = peer.generation.load(std::memory_order_acquire);
        u64 age = comlynx_heartbeat_age(now_us, heartbeat);

        if (peer.state.load(std::memory_order_acquire) != 1 || age <= COMLYNX_DETACH_US)
            continue;

        if (preserve_idle && peer.write_index.load(std::memory_order_acquire) == 0)
            continue;

        if (peer.state.load(std::memory_order_acquire) != 1)
            continue;

        u32 current_generation = peer.generation.load(std::memory_order_acquire);
        u64 current_heartbeat = peer.heartbeat_us.load(std::memory_order_acquire);

        if (!comlynx_lease_is_unchanged_and_stale(now_us, heartbeat, generation,
            current_heartbeat, current_generation))
        {
            continue;
        }

        u32 expected = 1;

        if (peer.state.compare_exchange_strong(expected, 0, std::memory_order_acq_rel))
        {
            m_status.peer_detaches++;
            m_status.peer_detach_max_age_us = MAX(m_status.peer_detach_max_age_us, age);
        }
    }
}

u64 ComLynxManager::ToBusCycle(u64 local_cycle) const
{
    return local_cycle - m_local_anchor + m_bus_anchor;
}

u64 ComLynxManager::GetClockMicroseconds() const
{
    return (u64)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void ComLynxManager::SetFault(const char* message)
{
    m_status.mode = ComLynxModeFault;
    m_status.cable_connected = false;
    snprintf(m_status.last_error, sizeof(m_status.last_error), "%s", message);
    Error("ComLynx: %s", message);
}

void ComLynxManager::RefreshStatus()
{
    if (!m_shared)
        return;

    int peers = 0;
    u8 local_peer_id = 0;

    for (int i = 0; i < COMLYNX_MAX_PEERS; i++)
    {
        Shared::Peer& peer = m_shared->peers[i];

        if (peer.state.load(std::memory_order_acquire) == 1)
        {
            peers++;

            if (i == m_slot)
                local_peer_id = (u8)peers;
        }
    }

    m_status.local_peer_id = local_peer_id;
    m_status.peer_count = peers;
    m_status.pacing_peer = IsPacingPeer();
}
