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

#ifndef COMLYNX_MANAGER_H
#define COMLYNX_MANAGER_H

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include "socket_types.h"
#include "comlynx_protocol.h"
#include "comlynx_queue.h"

enum ComLynxMode
{
    ComLynxModeDisabled,
    ComLynxModeHosting,
    ComLynxModeJoining,
    ComLynxModeConnected,
    ComLynxModeFault
};

struct ComLynxFrame
{
    u8 data;
    bool parity_bit;
};

struct ComLynxStatus
{
    ComLynxMode mode;
    bool cable_connected;
    u8 local_peer_id;
    int peer_count;
    int port;
    u64 packets_sent;
    u64 packets_received;
    u64 frames_sent;
    u64 frames_received;
    u64 duplicate_packets;
    u64 sequence_gaps;
    u64 queue_overflows;
    char endpoint[128];
    char last_error[128];
};

class ComLynxManager
{
public:
    ComLynxManager();
    ~ComLynxManager();

    bool Host(const char* bind_address, int port);
    bool Join(const char* host, int port);
    void Stop();

    bool SendFrame(u8 data, bool parity_bit);
    bool ReceiveFrame(ComLynxFrame& frame);
    void SetReceiveEnabled(bool enabled);
    void Synchronize(u64 cycles);

    bool IsActive() const;
    bool IsCableConnected() const;
    ComLynxMode GetMode() const;
    ComLynxStatus GetStatus() const;

private:
    struct Peer
    {
        Peer()
        {
            active = false;
            memset(&address, 0, sizeof(address));
            token = 0;
            id = 0;
            receive_sequence_valid = false;
            receive_sequence = 0;
            send_sequence = 0;
        }

        bool active;
        sockaddr_in address;
        u64 token;
        u8 id;
        bool receive_sequence_valid;
        u32 receive_sequence;
        u32 send_sequence;
        std::chrono::steady_clock::time_point last_seen;
    };

    void WorkerLoop();
    void HostReceive(const ComLynxPacket& packet, const sockaddr_in& source);
    void ClientReceive(const ComLynxPacket& packet, const sockaddr_in& source);
    bool QueueIncomingFrame(const ComLynxFrame& frame);
    void ProcessOutgoingFrames();
    void ProcessTimers();
    void SendJoinRequest();
    void SendJoinAccept(Peer& peer);
    void SendJoinReject(const sockaddr_in& destination, u64 token, u8 reason);
    void SendHeartbeat(Peer& peer);
    void SendClientHeartbeat();
    void SendLeavePackets();
    void BroadcastFrame(const ComLynxFrame& frame, u8 sender_id);
    bool SendPacket(const ComLynxPacket& packet, const sockaddr_in& destination);
    bool AcceptSequence(Peer& peer, u32 sequence);
    bool AcceptClientSequence(u32 sequence);
    int FindPeer(const sockaddr_in& source, u64 token, u8 id) const;
    int FindJoinPeer(const sockaddr_in& source, u64 token) const;
    int AllocatePeer() const;
    void RemovePeer(int index);
    bool IsHostSource(const sockaddr_in& source) const;
    bool ConfigureSocket(glynx_socket_t socket);
    bool SetupWakeSockets();
    void WakeWorker();
    void DrainWakeSocket();
    void CloseSocket();
    void SetMode(ComLynxMode mode);
    void SetFault(const char* message);
    void SetEndpoint(const char* address, int port);
    void ResetStatus();
    void IncrementPacketSent();
    void IncrementPacketReceived();
    void IncrementFrameSent();
    void IncrementFrameReceived();
    void IncrementDuplicate();
    void IncrementSequenceGaps(u32 count);
    void IncrementQueueOverflow();
    static bool SameAddress(const sockaddr_in& left, const sockaddr_in& right);
    static u64 MakeToken();

    glynx_socket_t m_socket;
    glynx_socket_t m_wake_receive_socket;
    glynx_socket_t m_wake_send_socket;
    sockaddr_in m_wake_address;
    sockaddr_in m_host_address;
    Peer m_peers[COMLYNX_MAX_PEERS - 1];
    std::thread m_worker;
    std::atomic<bool> m_stop_requested;
    std::atomic<int> m_mode;
    bool m_winsock_started;
    u64 m_session_id;
    u64 m_peer_token;
    u8 m_local_peer_id;
    u32 m_client_send_sequence;
    bool m_client_receive_sequence_valid;
    u32 m_client_receive_sequence;
    int m_port;
    int m_peer_count;
    std::chrono::steady_clock::time_point m_join_started;
    std::chrono::steady_clock::time_point m_last_join_request;
    std::chrono::steady_clock::time_point m_last_heartbeat;
    std::chrono::steady_clock::time_point m_last_host_packet;
    ComLynxQueue<ComLynxFrame, 1024> m_incoming_frames;
    ComLynxQueue<ComLynxFrame, 1024> m_outgoing_frames;
    bool m_receive_enabled;
    std::mutex m_receive_mutex;
    bool m_sync_valid;
    u64 m_sync_cycles;
    std::chrono::steady_clock::time_point m_sync_time;
    mutable std::mutex m_status_mutex;
    ComLynxStatus m_status;
};

#endif /* COMLYNX_MANAGER_H */