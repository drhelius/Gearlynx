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

inline u64 comlynx_heartbeat_age(u64 now, u64 heartbeat)
{
    return heartbeat <= now ? now - heartbeat : 0;
}

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
    void Synchronize(u64 local_cycle);

    bool IsActive() const;
    bool IsCableConnected() const;
    bool IsPacingPeer() const;
    ComLynxStatus GetStatus();
    void ResetMetrics();

private:
    struct Shared;

    bool Map(u8 session);
    void Unmap();
    bool ClaimSlot(u64 local_cycle, bool reattach);
    bool EnsureAttached(u64 local_cycle);
    void ReapStalePeers(u64 now_us, bool preserve_idle = false);
    u64 ToBusCycle(u64 local_cycle) const;
    u64 GetClockMicroseconds() const;
    void SetFault(const char* message);
    void RefreshStatus();

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
};

#endif /* COMLYNX_MANAGER_H */
