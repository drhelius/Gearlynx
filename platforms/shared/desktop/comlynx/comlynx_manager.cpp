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

#include <cerrno>
#include <cstdio>
#include <cstring>
#include "comlynx_manager.h"
#include "common.h"
#include "defines.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <netdb.h>
#endif

static const int k_comlynx_select_timeout_ms = 20;
static const int k_comlynx_join_retry_ms = 250;
static const int k_comlynx_join_timeout_ms = 5000;
static const int k_comlynx_heartbeat_ms = 1000;
static const int k_comlynx_peer_timeout_ms = 5000;

ComLynxManager::ComLynxManager()
{
    m_socket = GLYNX_INVALID_SOCKET;
    m_wake_receive_socket = GLYNX_INVALID_SOCKET;
    m_wake_send_socket = GLYNX_INVALID_SOCKET;
    memset(&m_wake_address, 0, sizeof(m_wake_address));
    memset(&m_host_address, 0, sizeof(m_host_address));
    m_stop_requested.store(false);
    m_mode.store(ComLynxModeDisabled);
    m_winsock_started = false;
    m_session_id = 0;
    m_peer_token = 0;
    m_local_peer_id = 0;
    m_client_send_sequence = 0;
    m_client_receive_sequence_valid = false;
    m_client_receive_sequence = 0;
    m_port = COMLYNX_DEFAULT_PORT;
    m_peer_count = 0;
    m_receive_enabled = false;
    m_sync_valid = false;
    m_sync_cycles = 0;
    ResetStatus();
}

ComLynxManager::~ComLynxManager()
{
    Stop();
}

bool ComLynxManager::Host(const char* bind_address, int port)
{
    Stop();

    if (!bind_address || port < 0 || port > 65535)
        return false;

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        SetFault("Failed to initialize Winsock");
        return false;
    }
    m_winsock_started = true;
#endif

    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == GLYNX_INVALID_SOCKET)
    {
        SetFault("Failed to create UDP socket");
        CloseSocket();
        return false;
    }

    int reuse = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons((u16)port);

    if (inet_pton(AF_INET, bind_address, &address.sin_addr) != 1)
    {
        SetFault("Invalid ComLynx bind address");
        CloseSocket();
        return false;
    }

    if (bind(m_socket, (sockaddr*)&address, sizeof(address)) < 0 ||
        !ConfigureSocket(m_socket) || !SetupWakeSockets())
    {
        SetFault("Failed to bind ComLynx UDP socket");
        CloseSocket();
        return false;
    }

    glynx_socket_len_t address_size = sizeof(address);
    if (getsockname(m_socket, (sockaddr*)&address, &address_size) == 0)
        port = ntohs(address.sin_port);

    ResetStatus();

    m_session_id = MakeToken();
    m_peer_token = MakeToken();

    m_local_peer_id = 1;
    m_port = port;
    m_peer_count = 0;

    for (int i = 0; i < COMLYNX_MAX_PEERS - 1; i++)
        m_peers[i] = Peer();

    m_incoming_frames.Clear();
    m_outgoing_frames.Clear();

    m_stop_requested.store(false);

    SetEndpoint(bind_address, port);
    SetMode(ComLynxModeHosting);

    m_last_heartbeat = std::chrono::steady_clock::now();

    m_worker = std::thread(&ComLynxManager::WorkerLoop, this);

    Log("ComLynx: hosting UDP session on %s:%d", bind_address, port);

    return true;
}

bool ComLynxManager::Join(const char* host, int port)
{
    Stop();

    if (!host || host[0] == '\0' || port < 1 || port > 65535)
        return false;

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        SetFault("Failed to initialize Winsock");
        return false;
    }
    m_winsock_started = true;
#endif

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    char port_text[16];
    snprintf(port_text, sizeof(port_text), "%d", port);
    addrinfo* result = NULL;

    if (getaddrinfo(host, port_text, &hints, &result) != 0 || !result)
    {
        SetFault("Failed to resolve ComLynx host");
        CloseSocket();
        return false;
    }

    memcpy(&m_host_address, result->ai_addr, sizeof(m_host_address));
    freeaddrinfo(result);

    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (m_socket == GLYNX_INVALID_SOCKET || !ConfigureSocket(m_socket) || !SetupWakeSockets())
    {
        SetFault("Failed to create ComLynx UDP socket");
        CloseSocket();
        return false;
    }

    ResetStatus();

    m_session_id = 0;
    m_peer_token = MakeToken();
    m_local_peer_id = 0;
    m_client_send_sequence = 0;
    m_client_receive_sequence_valid = false;
    m_client_receive_sequence = 0;
    m_port = port;
    m_peer_count = 0;

    m_incoming_frames.Clear();
    m_outgoing_frames.Clear();

    m_stop_requested.store(false);

    SetEndpoint(host, port);
    SetMode(ComLynxModeJoining);

    m_join_started = std::chrono::steady_clock::now();
    m_last_join_request = m_join_started - std::chrono::milliseconds(k_comlynx_join_retry_ms);
    m_last_host_packet = m_join_started;
    m_last_heartbeat = m_join_started;

    m_worker = std::thread(&ComLynxManager::WorkerLoop, this);

    Log("ComLynx: joining UDP session at %s:%d", host, port);

    return true;
}

void ComLynxManager::Stop()
{
    m_stop_requested.store(true);

    WakeWorker();

    if (m_worker.joinable())
        m_worker.join();

    CloseSocket();
    SetReceiveEnabled(false);

    m_outgoing_frames.Clear();
    m_session_id = 0;
    m_peer_token = 0;
    m_local_peer_id = 0;
    m_peer_count = 0;

    for (int i = 0; i < COMLYNX_MAX_PEERS - 1; i++)
        m_peers[i] = Peer();

    SetMode(ComLynxModeDisabled);
}

bool ComLynxManager::SendFrame(u8 data, bool parity_bit)
{
    ComLynxMode mode = GetMode();

    if (mode != ComLynxModeHosting && mode != ComLynxModeConnected)
        return false;

    ComLynxFrame frame = {data, parity_bit};

    if (m_outgoing_frames.Push(frame))
    {
        WakeWorker();
        return true;
    }

    IncrementQueueOverflow();

    SetFault("ComLynx transmit queue overflow");

    return false;
}

bool ComLynxManager::ReceiveFrame(ComLynxFrame& frame)
{
    return m_incoming_frames.Pop(frame);
}

void ComLynxManager::SetReceiveEnabled(bool enabled)
{
    std::lock_guard<std::mutex> lock(m_receive_mutex);

    if (enabled == m_receive_enabled)
        return;

    m_receive_enabled = enabled;
    m_incoming_frames.Clear();
}

void ComLynxManager::Synchronize(u64 cycles)
{
    if (!IsCableConnected())
    {
        m_sync_valid = false;
        return;
    }

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (!m_sync_valid || cycles < m_sync_cycles)
    {
        m_sync_valid = true;
        m_sync_cycles = cycles;
        m_sync_time = now;
        return;
    }

    u64 elapsed_cycles = cycles - m_sync_cycles;

    std::chrono::steady_clock::time_point target = m_sync_time +
        std::chrono::seconds(elapsed_cycles / GLYNX_MASTER_CLOCK) +
        std::chrono::nanoseconds((elapsed_cycles % GLYNX_MASTER_CLOCK) * 1000000000ULL / GLYNX_MASTER_CLOCK);

    if (now < target)
    {
        std::this_thread::sleep_until(target);
    }
    else if (now - target > std::chrono::milliseconds(5))
    {
        m_sync_cycles = cycles;
        m_sync_time = now;
    }
}

bool ComLynxManager::IsActive() const
{
    ComLynxMode mode = GetMode();
    return mode == ComLynxModeHosting || mode == ComLynxModeJoining || mode == ComLynxModeConnected;
}

bool ComLynxManager::IsCableConnected() const
{
    ComLynxMode mode = GetMode();
    return mode == ComLynxModeHosting || mode == ComLynxModeConnected;
}

ComLynxMode ComLynxManager::GetMode() const
{
    return (ComLynxMode)m_mode.load();
}

ComLynxStatus ComLynxManager::GetStatus() const
{
    std::lock_guard<std::mutex> lock(m_status_mutex);
    return m_status;
}

void ComLynxManager::WorkerLoop()
{
    while (!m_stop_requested.load())
    {
        ProcessOutgoingFrames();

        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(m_socket, &read_set);
        FD_SET(m_wake_receive_socket, &read_set);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = k_comlynx_select_timeout_ms * 1000;

        int highest_socket = (int)MAX(m_socket, m_wake_receive_socket);
        int ready = select(highest_socket + 1, &read_set, NULL, NULL, &timeout);

        if (ready > 0 && FD_ISSET(m_wake_receive_socket, &read_set))
        {
            DrainWakeSocket();
            ProcessOutgoingFrames();
        }

        if (ready > 0 && FD_ISSET(m_socket, &read_set))
        {
            while (!m_stop_requested.load())
            {
                u8 buffer[COMLYNX_PACKET_MAX_SIZE];
                sockaddr_in source = {};
                glynx_socket_len_t source_size = sizeof(source);

                int received = recvfrom(m_socket, (char*)buffer, sizeof(buffer), 0,
                    (sockaddr*)&source, &source_size);

                if (received <= 0)
                    break;

                ComLynxPacket packet;

                if (!comlynx_packet_decode(buffer, received, &packet))
                    continue;

                IncrementPacketReceived();

                if (GetMode() == ComLynxModeHosting)
                    HostReceive(packet, source);
                else
                    ClientReceive(packet, source);
            }
        }
        else if (ready < 0 && !m_stop_requested.load())
        {
#ifdef _WIN32
            if (WSAGetLastError() != WSAEINTR)
#else
            if (errno != EINTR)
#endif
                SetFault("ComLynx UDP receive failed");
        }

        ProcessTimers();
    }

    SendLeavePackets();
    CloseSocket();
}

void ComLynxManager::HostReceive(const ComLynxPacket& packet, const sockaddr_in& source)
{
    if (packet.type == ComLynxPacketJoinRequest)
    {
        int index = FindJoinPeer(source, packet.peer_token);

        if (index < 0)
        {
            index = AllocatePeer();

            if (index < 0)
            {
                SendJoinReject(source, packet.peer_token, ComLynxRejectSessionFull);
                return;
            }

            Peer& peer = m_peers[index];
            peer.active = true;
            peer.address = source;
            peer.token = packet.peer_token;
            peer.id = (u8)(index + 2);
            peer.receive_sequence_valid = false;
            peer.receive_sequence = 0;
            peer.send_sequence = 0;
            peer.last_seen = std::chrono::steady_clock::now();

            m_peer_count++;

            std::lock_guard<std::mutex> lock(m_status_mutex);

            m_status.peer_count = m_peer_count;
        }

        m_peers[index].last_seen = std::chrono::steady_clock::now();

        SendJoinAccept(m_peers[index]);

        return;
    }

    int index = FindPeer(source, packet.peer_token, packet.sender_id);

    if (index < 0 || packet.session_id != m_session_id)
        return;

    Peer& peer = m_peers[index];

    if (!AcceptSequence(peer, packet.sequence))
        return;

    peer.last_seen = std::chrono::steady_clock::now();

    if (packet.type == ComLynxPacketFrame)
    {
        ComLynxFrame frame = { packet.payload[0], (packet.flags & 0x01) != 0 };
    
        if (QueueIncomingFrame(frame))
            IncrementFrameReceived();

        BroadcastFrame(frame, peer.id);
    }
    else if (packet.type == ComLynxPacketLeave)
        RemovePeer(index);
}

void ComLynxManager::ClientReceive(const ComLynxPacket& packet, const sockaddr_in& source)
{
    if (!IsHostSource(source) || packet.peer_token != m_peer_token)
        return;

    ComLynxMode mode = GetMode();

    if (mode == ComLynxModeJoining)
    {
        if (packet.type == ComLynxPacketJoinReject)
        {
            SetFault("ComLynx host rejected the session");
            return;
        }

        if (packet.type != ComLynxPacketJoinAccept)
            return;

        m_session_id = packet.session_id;
        m_local_peer_id = packet.sender_id;
        m_client_send_sequence = 0;
        m_client_receive_sequence_valid = false;
        m_last_host_packet = std::chrono::steady_clock::now();

        SetMode(ComLynxModeConnected);

        return;
    }

    if (mode != ComLynxModeConnected || packet.session_id != m_session_id ||
        !AcceptClientSequence(packet.sequence))
        return;

    m_last_host_packet = std::chrono::steady_clock::now();

    if (packet.type == ComLynxPacketFrame)
    {
        if (packet.sender_id == m_local_peer_id)
            return;

        ComLynxFrame frame = { packet.payload[0], (packet.flags & 0x01) != 0 };

        if (QueueIncomingFrame(frame))
            IncrementFrameReceived();
    }
    else if (packet.type == ComLynxPacketLeave && packet.sender_id == 1)
        SetFault("ComLynx host closed the session");
}

bool ComLynxManager::QueueIncomingFrame(const ComLynxFrame& frame)
{
    std::lock_guard<std::mutex> lock(m_receive_mutex);

    if (!m_receive_enabled)
        return false;

    if (m_incoming_frames.Push(frame))
        return true;

    IncrementQueueOverflow();
    SetFault("ComLynx receive queue overflow");

    return false;
}

void ComLynxManager::ProcessOutgoingFrames()
{
    ComLynxFrame frame;

    while (m_outgoing_frames.Pop(frame))
    {
        ComLynxMode mode = GetMode();

        if (mode == ComLynxModeHosting)
            BroadcastFrame(frame, 1);
        else if (mode == ComLynxModeConnected)
        {
            ComLynxPacket packet = {};
            packet.type = ComLynxPacketFrame;
            packet.session_id = m_session_id;
            packet.peer_token = m_peer_token;
            packet.sequence = ++m_client_send_sequence;
            packet.sender_id = m_local_peer_id;
            packet.flags = frame.parity_bit ? 0x01 : 0;
            packet.payload_size = 1;
            packet.payload[0] = frame.data;

            if (SendPacket(packet, m_host_address))
                IncrementFrameSent();
        }
    }
}

void ComLynxManager::ProcessTimers()
{
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    ComLynxMode mode = GetMode();

    if (mode == ComLynxModeJoining)
    {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_join_started).count() >= k_comlynx_join_timeout_ms)
        {
            SetFault("Timed out joining ComLynx host");
            return;
        }

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_join_request).count() >= k_comlynx_join_retry_ms)
        {
            SendJoinRequest();
            m_last_join_request = now;
        }
    }
    else if (mode == ComLynxModeConnected)
    {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_host_packet).count() >= k_comlynx_peer_timeout_ms)
        {
            SetFault("ComLynx host timed out");
            return;
        }

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_heartbeat).count() >= k_comlynx_heartbeat_ms)
        {
            SendClientHeartbeat();
            m_last_heartbeat = now;
        }
    }
    else if (mode == ComLynxModeHosting)
    {
        for (int i = 0; i < COMLYNX_MAX_PEERS - 1; i++)
        {
            if (m_peers[i].active &&
                std::chrono::duration_cast<std::chrono::milliseconds>(now - m_peers[i].last_seen).count() >= k_comlynx_peer_timeout_ms)
                RemovePeer(i);
        }

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_heartbeat).count() >= k_comlynx_heartbeat_ms)
        {
            for (int i = 0; i < COMLYNX_MAX_PEERS - 1; i++)
            {
                if (m_peers[i].active)
                    SendHeartbeat(m_peers[i]);
            }

            m_last_heartbeat = now;
        }
    }
}

void ComLynxManager::SendJoinRequest()
{
    ComLynxPacket packet = {};
    packet.type = ComLynxPacketJoinRequest;
    packet.peer_token = m_peer_token;

    SendPacket(packet, m_host_address);
}

void ComLynxManager::SendJoinAccept(Peer& peer)
{
    ComLynxPacket packet = {};
    packet.type = ComLynxPacketJoinAccept;
    packet.session_id = m_session_id;
    packet.peer_token = peer.token;
    packet.sender_id = peer.id;

    SendPacket(packet, peer.address);
}

void ComLynxManager::SendJoinReject(const sockaddr_in& destination, u64 token, u8 reason)
{
    ComLynxPacket packet = {};
    packet.type = ComLynxPacketJoinReject;
    packet.session_id = m_session_id;
    packet.peer_token = token;
    packet.sender_id = 1;
    packet.payload_size = 1;
    packet.payload[0] = reason;

    SendPacket(packet, destination);
}

void ComLynxManager::SendHeartbeat(Peer& peer)
{
    ComLynxPacket packet = {};
    packet.type = ComLynxPacketHeartbeat;
    packet.session_id = m_session_id;
    packet.peer_token = peer.token;
    packet.sequence = ++peer.send_sequence;
    packet.sender_id = 1;

    SendPacket(packet, peer.address);
}

void ComLynxManager::SendClientHeartbeat()
{
    ComLynxPacket packet = {};
    packet.type = ComLynxPacketHeartbeat;
    packet.session_id = m_session_id;
    packet.peer_token = m_peer_token;
    packet.sequence = ++m_client_send_sequence;
    packet.sender_id = m_local_peer_id;

    SendPacket(packet, m_host_address);
}

void ComLynxManager::SendLeavePackets()
{
    ComLynxMode mode = GetMode();
    if (mode == ComLynxModeHosting)
    {
        for (int i = 0; i < COMLYNX_MAX_PEERS - 1; i++)
        {
            if (!m_peers[i].active)
                continue;

            ComLynxPacket packet = {};
            packet.type = ComLynxPacketLeave;
            packet.session_id = m_session_id;
            packet.peer_token = m_peers[i].token;
            packet.sequence = ++m_peers[i].send_sequence;
            packet.sender_id = 1;

            SendPacket(packet, m_peers[i].address);
        }
    }
    else if (mode == ComLynxModeConnected)
    {
        ComLynxPacket packet = {};
        packet.type = ComLynxPacketLeave;
        packet.session_id = m_session_id;
        packet.peer_token = m_peer_token;
        packet.sequence = ++m_client_send_sequence;
        packet.sender_id = m_local_peer_id;

        SendPacket(packet, m_host_address);
    }
}

void ComLynxManager::BroadcastFrame(const ComLynxFrame& frame, u8 sender_id)
{
    for (int i = 0; i < COMLYNX_MAX_PEERS - 1; i++)
    {
        Peer& peer = m_peers[i];
        if (!peer.active || peer.id == sender_id)
            continue;

        ComLynxPacket packet = {};
        packet.type = ComLynxPacketFrame;
        packet.session_id = m_session_id;
        packet.peer_token = peer.token;
        packet.sequence = ++peer.send_sequence;
        packet.sender_id = sender_id;
        packet.flags = frame.parity_bit ? 0x01 : 0;
        packet.payload_size = 1;
        packet.payload[0] = frame.data;

        if (SendPacket(packet, peer.address))
            IncrementFrameSent();
    }
}

bool ComLynxManager::SendPacket(const ComLynxPacket& packet, const sockaddr_in& destination)
{
    u8 buffer[COMLYNX_PACKET_MAX_SIZE];
    int size = 0;
    if (!comlynx_packet_encode(packet, buffer, sizeof(buffer), &size))
        return false;

    int sent = sendto(m_socket, (const char*)buffer, size, 0, (const sockaddr*)&destination, sizeof(destination));

    if (sent != size)
        return false;

    IncrementPacketSent();

    return true;
}

bool ComLynxManager::AcceptSequence(Peer& peer, u32 sequence)
{
    if (!peer.receive_sequence_valid)
    {
        peer.receive_sequence_valid = true;
        peer.receive_sequence = sequence;
        return true;
    }

    s32 difference = (s32)(sequence - peer.receive_sequence);

    if (difference <= 0)
    {
        IncrementDuplicate();
        return false;
    }

    if (difference > 1)
        IncrementSequenceGaps((u32)(difference - 1));

    peer.receive_sequence = sequence;

    return true;
}

bool ComLynxManager::AcceptClientSequence(u32 sequence)
{
    if (!m_client_receive_sequence_valid)
    {
        m_client_receive_sequence_valid = true;
        m_client_receive_sequence = sequence;
        return true;
    }

    s32 difference = (s32)(sequence - m_client_receive_sequence);

    if (difference <= 0)
    {
        IncrementDuplicate();
        return false;
    }

    if (difference > 1)
        IncrementSequenceGaps((u32)(difference - 1));

    m_client_receive_sequence = sequence;

    return true;
}

int ComLynxManager::FindPeer(const sockaddr_in& source, u64 token, u8 id) const
{
    for (int i = 0; i < COMLYNX_MAX_PEERS - 1; i++)
    {
        const Peer& peer = m_peers[i];
        if (peer.active && peer.token == token && peer.id == id && SameAddress(peer.address, source))
            return i;
    }

    return -1;
}

int ComLynxManager::FindJoinPeer(const sockaddr_in& source, u64 token) const
{
    for (int i = 0; i < COMLYNX_MAX_PEERS - 1; i++)
    {
        const Peer& peer = m_peers[i];
        if (peer.active && peer.token == token && SameAddress(peer.address, source))
            return i;
    }

    return -1;
}

int ComLynxManager::AllocatePeer() const
{
    for (int i = 0; i < COMLYNX_MAX_PEERS - 1; i++)
    {
        if (!m_peers[i].active)
            return i;
    }

    return -1;
}

void ComLynxManager::RemovePeer(int index)
{
    if (index < 0 || index >= COMLYNX_MAX_PEERS - 1 || !m_peers[index].active)
        return;

    m_peers[index] = Peer();
    m_peer_count--;

    std::lock_guard<std::mutex> lock(m_status_mutex);

    m_status.peer_count = m_peer_count;
}

bool ComLynxManager::IsHostSource(const sockaddr_in& source) const
{
    return SameAddress(source, m_host_address);
}

bool ComLynxManager::ConfigureSocket(glynx_socket_t socket)
{
#ifdef _WIN32
    u_long nonblocking = 1;
    return ioctlsocket(socket, FIONBIO, &nonblocking) == 0;
#else
    int flags = fcntl(socket, F_GETFL, 0);
    return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

bool ComLynxManager::SetupWakeSockets()
{
    m_wake_receive_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    m_wake_send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (m_wake_receive_socket == GLYNX_INVALID_SOCKET ||
        m_wake_send_socket == GLYNX_INVALID_SOCKET ||
        !ConfigureSocket(m_wake_receive_socket) || !ConfigureSocket(m_wake_send_socket))
        return false;

    memset(&m_wake_address, 0, sizeof(m_wake_address));
    m_wake_address.sin_family = AF_INET;
    m_wake_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    m_wake_address.sin_port = 0;

    if (bind(m_wake_receive_socket, (sockaddr*)&m_wake_address, sizeof(m_wake_address)) < 0)
        return false;

    glynx_socket_len_t address_size = sizeof(m_wake_address);

    return getsockname(m_wake_receive_socket, (sockaddr*)&m_wake_address, &address_size) == 0;
}

void ComLynxManager::WakeWorker()
{
    if (m_wake_send_socket == GLYNX_INVALID_SOCKET || m_wake_address.sin_port == 0)
        return;

    const char wake = 1;

    sendto(m_wake_send_socket, &wake, sizeof(wake), 0,
        (sockaddr*)&m_wake_address, sizeof(m_wake_address));
}

void ComLynxManager::DrainWakeSocket()
{
    char buffer[32];
    while (recvfrom(m_wake_receive_socket, buffer, sizeof(buffer), 0, NULL, NULL) > 0)
    {
    }
}

void ComLynxManager::CloseSocket()
{
    if (m_socket != GLYNX_INVALID_SOCKET)
    {
        GLYNX_SOCKET_CLOSE(m_socket);
        m_socket = GLYNX_INVALID_SOCKET;
    }

    if (m_wake_receive_socket != GLYNX_INVALID_SOCKET)
    {
        GLYNX_SOCKET_CLOSE(m_wake_receive_socket);
        m_wake_receive_socket = GLYNX_INVALID_SOCKET;
    }

    if (m_wake_send_socket != GLYNX_INVALID_SOCKET)
    {
        GLYNX_SOCKET_CLOSE(m_wake_send_socket);
        m_wake_send_socket = GLYNX_INVALID_SOCKET;
    }

    memset(&m_wake_address, 0, sizeof(m_wake_address));

#ifdef _WIN32
    if (m_winsock_started)
    {
        WSACleanup();
        m_winsock_started = false;
    }
#endif
}

void ComLynxManager::SetMode(ComLynxMode mode)
{
    m_mode.store(mode);

    std::lock_guard<std::mutex> lock(m_status_mutex);

    m_status.mode = mode;
    m_status.cable_connected = mode == ComLynxModeHosting || mode == ComLynxModeConnected;
    m_status.local_peer_id = m_local_peer_id;
}

void ComLynxManager::SetFault(const char* message)
{
    m_mode.store(ComLynxModeFault);
    m_stop_requested.store(true);

    std::lock_guard<std::mutex> lock(m_status_mutex);

    m_status.mode = ComLynxModeFault;
    m_status.cable_connected = false;

    strncpy_fit(m_status.last_error, message ? message : "Unknown ComLynx error", sizeof(m_status.last_error));
}

void ComLynxManager::SetEndpoint(const char* address, int port)
{
    std::lock_guard<std::mutex> lock(m_status_mutex);

    snprintf(m_status.endpoint, sizeof(m_status.endpoint), "%s:%d", address, port);
    m_status.port = port;
}

void ComLynxManager::ResetStatus()
{
    std::lock_guard<std::mutex> lock(m_status_mutex);

    memset(&m_status, 0, sizeof(m_status));
    m_status.mode = ComLynxModeDisabled;
}

void ComLynxManager::IncrementPacketSent()
{
    std::lock_guard<std::mutex> lock(m_status_mutex);

    m_status.packets_sent++;
}

void ComLynxManager::IncrementPacketReceived()
{
    std::lock_guard<std::mutex> lock(m_status_mutex);

    m_status.packets_received++;
}

void ComLynxManager::IncrementFrameSent()
{
    std::lock_guard<std::mutex> lock(m_status_mutex);

    m_status.frames_sent++;
}

void ComLynxManager::IncrementFrameReceived()
{
    std::lock_guard<std::mutex> lock(m_status_mutex);

    m_status.frames_received++;
}

void ComLynxManager::IncrementDuplicate()
{
    std::lock_guard<std::mutex> lock(m_status_mutex);

    m_status.duplicate_packets++;
}

void ComLynxManager::IncrementSequenceGaps(u32 count)
{
    std::lock_guard<std::mutex> lock(m_status_mutex);

    m_status.sequence_gaps += count;
}

void ComLynxManager::IncrementQueueOverflow()
{
    std::lock_guard<std::mutex> lock(m_status_mutex);

    m_status.queue_overflows++;
}

bool ComLynxManager::SameAddress(const sockaddr_in& left, const sockaddr_in& right)
{
    return left.sin_family == right.sin_family && left.sin_port == right.sin_port &&
        left.sin_addr.s_addr == right.sin_addr.s_addr;
}

u64 ComLynxManager::MakeToken()
{
    static std::atomic<u64> counter(1);

    u64 value = (u64)std::chrono::high_resolution_clock::now().time_since_epoch().count();

    value ^= counter.fetch_add(1) * 0x9E3779B97F4A7C15ULL;
    value ^= value >> 30;
    value *= 0xBF58476D1CE4E5B9ULL;
    value ^= value >> 27;
    value *= 0x94D049BB133111EBULL;
    value ^= value >> 31;

    return value ? value : 1;
}