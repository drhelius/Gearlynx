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

#define COMLYNX_PENDING_PACKET_COUNT 64
#define COMLYNX_INCOMING_FRAME_QUEUE_COUNT 16384
#define COMLYNX_BURST_STALL_LIMIT 1024

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
    u8 sender_id;
    bool parity_bit;
    bool burst_end;
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
    u64 datagrams_sent;
    u64 datagrams_received;
    u64 frames_generated;
    u64 frames_sent;
    u64 frames_received_network;
    u64 frames_queued;
    u64 frames_consumed;
    u64 frames_dropped_disabled;
    u64 frames_dropped_clear;
    u64 bursts_delivered;
    u64 bursts_forced;
    u32 max_burst_length;
    u64 send_eagain;
    u64 send_errors;
    u64 duplicate_packets;
    u64 out_of_order_packets;
    u64 sequence_gaps;
    u64 queue_overflows;
    u32 max_outgoing_queue_depth;
    u32 max_incoming_queue_depth;
    u32 max_pending_packet_depth;
    u64 frame_rx_interval_min_us;
    u64 frame_rx_interval_avg_us;
    u64 frame_rx_interval_max_us;
    u64 frame_rx_interval_variation_us;
    u64 rx_bursts;
    u32 rx_burst_max;
    u64 rx_burst_total_packets;
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

    bool SendFrame(u8 data, bool parity_bit, bool burst_end);
    bool ReceiveFrame(ComLynxFrame& frame);
    void SetReceiveEnabled(bool enabled);
    void Synchronize(u64 cycles);

    bool IsActive() const;
    bool IsCableConnected() const;
    ComLynxMode GetMode() const;
    ComLynxStatus GetStatus() const;
    void ResetMetrics();

private:
    struct PendingPacket
    {
        sockaddr_in destination;
        int size;
        bool is_frame;
        u8 data[COMLYNX_PACKET_MAX_SIZE];
    };

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
    bool HostReceive(const ComLynxPacket& packet, const sockaddr_in& source, u64 local_receive_time_us);
    bool ClientReceive(const ComLynxPacket& packet, const sockaddr_in& source, u64 local_receive_time_us);
    bool QueueIncomingFrame(const ComLynxFrame& frame);
    void ClearIncomingFrames();
    u8 SelectDeliverySender();
    u32 IncomingFrameCount() const;
    void ProcessOutgoingFrames();
    void ProcessPendingPackets();
    void ProcessTimers();
    void SendJoinRequest();
    void SendJoinAccept(Peer& peer);
    void SendJoinReject(const sockaddr_in& destination, u64 token, u8 reason);
    void SendHeartbeat(Peer& peer);
    void SendClientHeartbeat();
    void SendLeavePackets();
    void BroadcastFrame(const ComLynxFrame& frame, u8 sender_id);
    bool SendPacket(const ComLynxPacket& packet, const sockaddr_in& destination);
    bool QueuePendingPacket(const u8* data, int size, bool is_frame, const sockaddr_in& destination);
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
    void ClearPendingPackets();
    void SetMode(ComLynxMode mode);
    void SetFault(const char* message);
    void SetEndpoint(const char* address, int port);
    void ResetStatus();
    void IncrementPacketSent();
    void IncrementPacketReceived();
    void RecordDatagramReceived();
    void IncrementFrameGenerated();
    void IncrementFrameSent();
    void IncrementFrameReceivedNetwork();
    void IncrementFrameQueued();
    void IncrementFrameConsumed();
    void IncrementFramesDroppedDisabled();
    void IncrementFramesDroppedClear(u32 count);
    void RecordBurstDelivered(u32 length, bool forced);
    void IncrementSendEagain();
    void IncrementSendError();
    void IncrementDuplicate();
    void IncrementOutOfOrder();
    void IncrementSequenceGaps(u32 count);
    void IncrementQueueOverflow();
    void UpdateMaxOutgoingQueueDepth(u32 depth);
    void UpdateMaxIncomingQueueDepth(u32 depth);
    void UpdateMaxPendingPacketDepth(u32 depth);
    void RecordFrameReceive(u64 local_receive_time_us);
    void RecordReceiveBurst(u32 frame_count);
    static bool SameAddress(const sockaddr_in& left, const sockaddr_in& right);
    static bool SendWouldBlock();
    static u64 GetClockMicroseconds();
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
    ComLynxQueue<ComLynxFrame, COMLYNX_INCOMING_FRAME_QUEUE_COUNT> m_incoming_frames[COMLYNX_MAX_PEERS + 1];
    ComLynxQueue<ComLynxFrame, 1024> m_outgoing_frames;
    u8 m_delivery_sender;
    u8 m_delivery_next_sender;
    u32 m_delivery_stall_count;
    u32 m_delivery_burst_length;
    PendingPacket m_pending_packets[COMLYNX_PENDING_PACKET_COUNT];
    int m_pending_packet_head;
    int m_pending_packet_count;
    bool m_receive_enabled;
    std::mutex m_receive_mutex;
    bool m_sync_valid;
    u64 m_sync_cycles;
    std::chrono::steady_clock::time_point m_sync_time;
    bool m_frame_rx_interval_valid;
    u64 m_last_frame_rx_time_us;
    u64 m_frame_rx_interval_samples;
    u64 m_frame_rx_interval_total_us;
    u64 m_frame_rx_interval_min_us;
    u64 m_frame_rx_interval_max_us;
    u64 m_last_frame_rx_interval_us;
    u64 m_frame_rx_interval_variation_us;
    mutable std::mutex m_status_mutex;
    ComLynxStatus m_status;
};

#endif /* COMLYNX_MANAGER_H */