/*
 * Gearlynx - Lynx Emulator
 * Copyright (C) 2025  Ignacio Sanchez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 */

#ifndef COMLYNX_MANAGER_H
#define COMLYNX_MANAGER_H

#include "comlynx.h"

#define COMLYNX_DETACH_US 500000

enum ComLynxMode
{
    ComLynxModeDisabled,
    ComLynxModeConnected,
    ComLynxModeFault
};

struct ComLynxStatus
{
    ComLynxMode mode;
    bool cable_connected;
    u8 session;
    u8 local_peer_id;
    int peer_count;
    u64 frames_published;
    u64 line_samples;
    u64 low_samples;
    u64 barrier_waits;
    u64 barrier_wait_us;
    u64 barrier_wait_max_us;
    u64 barrier_wait_over_1ms;
    u64 barrier_wait_over_10ms;
    u64 barrier_wait_over_50ms;
    u64 sync_gap_max_us;
    u64 sync_gap_over_50ms;
    u64 peer_detaches;
    u64 peer_detach_max_age_us;
    u64 reattachments;
    bool pacing_peer;
    char endpoint[128];
    char last_error[128];
};

class ComLynxManager
{
public:
    ComLynxManager();
    ~ComLynxManager();
    bool Connect(u8 session, u64 local_cycle);
    void Stop();
    void PublishFrame(u64 local_start_cycle, u32 bit_cycles, u16 bits);
    void SetBreak(bool asserted, u64 local_cycle);
    bool SampleLine(u64 local_cycle);
    void Synchronize(u64 local_cycle, u32 promise_cycles);
    bool SampleLineTurbo(u64 local_cycle);
    void SynchronizeTurbo(u64 local_cycle);
    bool IsActive() const;
    bool IsCableConnected() const;
    bool IsPacingPeer() const;
    ComLynxStatus GetStatus();
    void ResetMetrics();
    void SetNormalBarrierStallUs(u32 stall_us);

private:
    struct Shared;

    bool Map(u8 session);
    void Unmap();
    bool ClaimSlot(u64 local_cycle, bool reattach);
    bool EnsureAttached(u64 local_cycle);
    void MaintainTurbo(u64 local_cycle);
    void ReapStalePeers(u64 now_us, bool preserve_idle = false);
    u64 ToBusCycle(u64 local_cycle) const;
    u64 GetClockMicroseconds() const;
    void SetFault(const char* message);
    void RefreshStatus();

private:
    Shared* m_shared;
    void* m_mapping_handle;
    int m_mapping_fd;
    int m_slot;
    u32 m_generation;
    u8 m_session;
    u64 m_local_anchor;
    u64 m_bus_anchor;
    u64 m_last_sync_exit_us;
    ComLynxStatus m_status;
    u64 m_turbo_next_maintenance_cycle;
    u32 m_normal_barrier_stall_us;
};

inline u32 comlynx_normal_barrier_stall_us()
{
#if defined(_WIN32)
    return 5000;
#elif defined(__APPLE__)
    return 100;
#else
    return 250;
#endif
}

inline u64 comlynx_heartbeat_age(u64 now, u64 heartbeat)
{
    return heartbeat <= now ? now - heartbeat : 0;
}

inline bool comlynx_lease_is_unchanged_and_stale(u64 now, u64 observed_heartbeat,
    u32 observed_generation, u64 current_heartbeat, u32 current_generation)
{
    return current_heartbeat == observed_heartbeat &&
        current_generation == observed_generation &&
        comlynx_heartbeat_age(now, current_heartbeat) > COMLYNX_DETACH_US;
}

#endif /* COMLYNX_MANAGER_H */
