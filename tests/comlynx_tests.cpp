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

#include <chrono>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <thread>
#include "defines.h"
#include "comlynx/comlynx_manager.h"

bool g_mcp_stdio_mode = false;

static int failures = 0;

static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        fprintf(stderr, "FAILED: %s\n", message);
        failures++;
    }
}

static bool WaitForMode(ComLynxManager& manager, ComLynxMode mode, int timeout_ms)
{
    std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (manager.GetMode() == mode)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}

static bool WaitForFrame(ComLynxManager& manager, ComLynxFrame& frame, int timeout_ms)
{
    std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (manager.ReceiveFrame(frame))
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}

static bool WaitForPeerCount(ComLynxManager& manager, int peer_count, int timeout_ms)
{
    std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (manager.GetStatus().peer_count == peer_count)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}

static bool WaitForPacketsReceived(ComLynxManager& manager, u64 packets, int timeout_ms)
{
    std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (manager.GetStatus().packets_received >= packets)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}

static bool WaitForFramesQueued(ComLynxManager& manager, u64 frames, int timeout_ms)
{
    std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (manager.GetStatus().frames_queued >= frames)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}

static void TestProtocol()
{
    ComLynxPacket packet = {};
    packet.type = ComLynxPacketFrame;
    packet.session_id = 0x0102030405060708ULL;
    packet.peer_token = 0x1112131415161718ULL;
    packet.sequence = 0x21222324;
    packet.sender_id = 3;
    packet.flags = 1;
    packet.payload_size = 1;
    packet.payload[0] = 0xA5;

    u8 buffer[COMLYNX_PACKET_MAX_SIZE] = {};
    int size = 0;
    Check(comlynx_packet_encode(packet, buffer, sizeof(buffer), &size), "encode frame packet");
    Check(size == 33, "encoded frame size");
    Check(memcmp(buffer, "GLCX", 4) == 0, "protocol magic");
    Check(buffer[8] == 0x01 && buffer[15] == 0x08, "session big endian");
    Check(buffer[24] == 0x21 && buffer[27] == 0x24, "sequence big endian");

    ComLynxPacket decoded = {};
    Check(comlynx_packet_decode(buffer, size, &decoded), "decode frame packet");
    Check(decoded.session_id == packet.session_id, "round-trip session");
    Check(decoded.peer_token == packet.peer_token, "round-trip token");
    Check(decoded.sequence == packet.sequence, "round-trip sequence");
    Check(decoded.payload[0] == 0xA5 && decoded.flags == 1, "round-trip frame");

    for (int i = 0; i < size; i++)
        Check(!comlynx_packet_decode(buffer, i, &decoded), "reject truncated packet");

    u8 malformed[COMLYNX_PACKET_MAX_SIZE];
    memcpy(malformed, buffer, size);
    malformed[0] = 'X';
    Check(!comlynx_packet_decode(malformed, size, &decoded), "reject bad magic");
    memcpy(malformed, buffer, size);
    malformed[4]++;
    Check(!comlynx_packet_decode(malformed, size, &decoded), "reject bad version");
    memcpy(malformed, buffer, size);
    malformed[5] = 0xFF;
    Check(!comlynx_packet_decode(malformed, size, &decoded), "reject unknown packet type");
    memcpy(malformed, buffer, size);
    malformed[6] = 0;
    malformed[7] = 2;
    Check(!comlynx_packet_decode(malformed, size, &decoded), "reject invalid payload size");
    memcpy(malformed, buffer, size);
    malformed[30] = 1;
    Check(!comlynx_packet_decode(malformed, size, &decoded), "reject nonzero reserved bytes");
    Check(!comlynx_packet_decode(buffer, size + 1, &decoded), "reject trailing byte");
}

static void TestQueue()
{
    ComLynxQueue<int, 4> queue;
    Check(queue.Push(1), "queue push 1");
    Check(queue.Push(2), "queue push 2");
    Check(queue.Push(3), "queue push 3");
    Check(queue.Push(4), "queue push 4");
    Check(!queue.Push(5), "queue full");
    Check(queue.Size() == 4, "queue full size");

    int value = 0;
    for (int expected = 1; expected <= 4; expected++)
    {
        Check(queue.Pop(value), "queue pop");
        Check(value == expected, "queue order");
    }
    Check(!queue.Pop(value), "queue empty");

    Check(queue.Push(6), "queue wrap push");
    Check(queue.Pop(value) && value == 6, "queue wrap pop");
}

static void TestConcurrentQueue()
{
    const int item_count = 100000;
    ComLynxQueue<int, 64> queue;
    std::atomic<bool> stop(false);

    std::thread producer([&queue, &stop]()
    {
        for (int value = 0; value < item_count && !stop.load(); value++)
        {
            while (!queue.Push(value) && !stop.load())
                std::this_thread::yield();
        }
    });

    int expected = 0;
    std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(5);
    while (expected < item_count && std::chrono::steady_clock::now() < deadline)
    {
        int value = 0;
        if (queue.Pop(value))
        {
            Check(value == expected, "concurrent queue order");
            expected++;
        }
        else
            std::this_thread::yield();
    }

    stop.store(true);
    producer.join();
    Check(expected == item_count, "concurrent queue completed before deadline");
}

static void TestLoopback()
{
    ComLynxManager host;
    ComLynxManager client_a;
    ComLynxManager client_b;

    Check(host.Host("127.0.0.1", 0), "start host");
    int port = host.GetStatus().port;
    Check(port > 0, "host assigned port");
    Check(client_a.Join("127.0.0.1", port), "start client A");
    Check(client_b.Join("127.0.0.1", port), "start client B");
    Check(WaitForMode(client_a, ComLynxModeConnected, 2000), "client A connected");
    Check(WaitForMode(client_b, ComLynxModeConnected, 2000), "client B connected");
    host.SetReceiveEnabled(true);
    client_a.SetReceiveEnabled(true);
    client_b.SetReceiveEnabled(true);

    Check(client_a.SendFrame(0x42, true), "client A send");
    ComLynxFrame frame = {};
    Check(WaitForFrame(host, frame, 1000), "host receive client A");
    Check(frame.data == 0x42 && frame.parity_bit, "host frame contents");
    Check(host.GetStatus().frames_received_network == 1, "host network frame count");
    Check(host.GetStatus().frames_queued == 1, "host queued frame count");
    Check(host.GetStatus().frames_consumed == 1, "host consumed frame count");
    Check(WaitForFrame(client_b, frame, 1000), "client B receive client A");
    Check(frame.data == 0x42 && frame.parity_bit, "client B frame contents");
    Check(client_b.GetStatus().frames_received_network == 1, "client network frame count");
    Check(client_b.GetStatus().frames_queued == 1, "client queued frame count");
    Check(client_b.GetStatus().frames_consumed == 1, "client consumed frame count");
    Check(client_b.GetStatus().rx_bursts >= 1, "client receive burst count");
    Check(client_b.GetStatus().rx_burst_max >= 1, "client receive burst maximum");
    Check(!client_a.ReceiveFrame(frame), "sender receives no network echo");

    Check(host.SendFrame(0x99, false), "host send");
    Check(WaitForFrame(client_a, frame, 1000), "client A receive host");
    Check(frame.data == 0x99 && !frame.parity_bit, "client A host frame contents");
    Check(WaitForFrame(client_b, frame, 1000), "client B receive host");
    Check(frame.data == 0x99 && !frame.parity_bit, "client B host frame contents");

    std::chrono::steady_clock::time_point relay_start = std::chrono::steady_clock::now();
    for (int value = 0; value < 256; value++)
    {
        Check(client_a.SendFrame((u8)value, (value & 1) != 0), "ordered frame send");
        Check(WaitForFrame(host, frame, 1000), "host ordered frame receive");
        Check(frame.data == (u8)value && frame.parity_bit == ((value & 1) != 0), "host frame order");
        Check(WaitForFrame(client_b, frame, 1000), "client ordered frame receive");
        Check(frame.data == (u8)value && frame.parity_bit == ((value & 1) != 0), "client frame order");
    }
    std::chrono::milliseconds relay_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - relay_start);
    Check(relay_time.count() < 2000, "ordered relay avoids polling latency");

    client_a.Stop();
    Check(WaitForPeerCount(host, 1, 1000), "host removes leaving client");
    client_b.Stop();
    host.Stop();
    Check(host.GetMode() == ComLynxModeDisabled, "host stopped");
}

static void TestReceiveBacklogRecovery()
{
    const int frame_count = 2048;
    const int batch_size = 128;
    ComLynxManager host;
    ComLynxManager client;

    Check(host.Host("127.0.0.1", 0), "start backlog host");
    Check(client.Join("127.0.0.1", host.GetStatus().port), "start backlog client");
    Check(WaitForMode(client, ComLynxModeConnected, 2000), "backlog client connected");
    client.SetReceiveEnabled(true);

    for (int start = 0; start < frame_count; start += batch_size)
    {
        for (int i = start; i < start + batch_size; i++)
            Check(host.SendFrame((u8)i, (i & 1) != 0), "send backlog frame");

        Check(WaitForFramesQueued(client, start + batch_size, 2000), "queue backlog frame batch");
    }

    ComLynxStatus status = client.GetStatus();
    Check(client.GetMode() == ComLynxModeConnected, "backlog client remains connected");
    Check(status.queue_overflows == 0, "backlog does not overflow receive queue");
    Check(status.max_incoming_queue_depth >= frame_count, "backlog exceeds old receive queue capacity");

    ComLynxFrame frame = {};
    for (int i = 0; i < frame_count; i++)
    {
        Check(client.ReceiveFrame(frame), "drain backlog frame");
        Check(frame.data == (u8)i && frame.parity_bit == ((i & 1) != 0), "backlog frame order");
    }
    Check(!client.ReceiveFrame(frame), "backlog queue drained");

    client.Stop();
    host.Stop();
}

static void TestSixPlayerRelayBacklog()
{
    const int client_count = 5;
    const int frames_per_client = 320;
    const int batch_size = 32;
    ComLynxManager host;
    ComLynxManager* clients = new ComLynxManager[client_count];

    Check(host.Host("127.0.0.1", 0), "start six-player host");
    host.SetReceiveEnabled(true);

    for (int client = 0; client < client_count; client++)
    {
        Check(clients[client].Join("127.0.0.1", host.GetStatus().port), "start six-player client");
        Check(WaitForMode(clients[client], ComLynxModeConnected, 2000), "six-player client connected");
        clients[client].SetReceiveEnabled(true);
    }
    Check(WaitForPeerCount(host, client_count, 2000), "six-player host peer count");

    for (int start = 0; start < frames_per_client; start += batch_size)
    {
        for (int frame = start; frame < start + batch_size; frame++)
        {
            for (int client = 0; client < client_count; client++)
                Check(clients[client].SendFrame((u8)(frame + client), (client & 1) != 0),
                    "send six-player frame");
        }

        int frames_sent_per_client = start + batch_size;
        Check(WaitForFramesQueued(host, frames_sent_per_client * client_count, 5000),
            "queue six-player host batch");
        for (int client = 0; client < client_count; client++)
        {
            Check(WaitForFramesQueued(clients[client], frames_sent_per_client * (client_count - 1), 5000),
                "queue six-player client batch");
        }
    }

    Check(host.GetMode() == ComLynxModeHosting, "six-player host remains connected");
    Check(host.GetStatus().queue_overflows == 0, "six-player host queue does not overflow");
    Check(host.GetStatus().max_incoming_queue_depth > 1024, "six-player host exceeds old queue capacity");

    ComLynxFrame frame = {};
    for (int i = 0; i < frames_per_client * client_count; i++)
        Check(host.ReceiveFrame(frame), "drain six-player host frame");

    for (int client = 0; client < client_count; client++)
    {
        Check(clients[client].GetMode() == ComLynxModeConnected, "six-player client remains connected");
        Check(clients[client].GetStatus().queue_overflows == 0, "six-player client queue does not overflow");
        Check(clients[client].GetStatus().max_incoming_queue_depth > 1024,
            "six-player client exceeds old queue capacity");

        for (int i = 0; i < frames_per_client * (client_count - 1); i++)
            Check(clients[client].ReceiveFrame(frame), "drain six-player client frame");
    }

    for (int client = 0; client < client_count; client++)
        clients[client].Stop();
    host.Stop();
    delete[] clients;
}

static void TestSessionCapacity()
{
    ComLynxManager host;
    ComLynxManager* clients = new ComLynxManager[COMLYNX_MAX_PEERS];
    Check(host.Host("127.0.0.1", 0), "capacity host start");
    int port = host.GetStatus().port;

    for (int i = 0; i < COMLYNX_MAX_PEERS - 1; i++)
    {
        Check(clients[i].Join("127.0.0.1", port), "capacity client start");
        Check(WaitForMode(clients[i], ComLynxModeConnected, 2000), "capacity client connected");
    }
    Check(WaitForPeerCount(host, COMLYNX_MAX_PEERS - 1, 1000), "host reaches session capacity");

    Check(clients[COMLYNX_MAX_PEERS - 1].Join("127.0.0.1", port), "overflow client start");
    Check(WaitForMode(clients[COMLYNX_MAX_PEERS - 1], ComLynxModeFault, 2000), "full session rejected");

    host.Stop();
    for (int i = 0; i < COMLYNX_MAX_PEERS; i++)
        clients[i].Stop();
    delete[] clients;
}

static void TestSynchronization()
{
    ComLynxManager manager;
    Check(manager.Host("127.0.0.1", 0), "start synchronization host");

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    manager.Synchronize(0);
    manager.Synchronize(GLYNX_MASTER_CLOCK / 100);
    std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    Check(elapsed.count() >= 8, "active session paces emulated cycles");
    manager.Stop();
}

static void TestSynchronizationRecovery()
{
    ComLynxManager manager;
    Check(manager.Host("127.0.0.1", 0), "start synchronization recovery host");

    manager.Synchronize(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    manager.Synchronize(GLYNX_MASTER_CLOCK / 100);

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    manager.Synchronize(GLYNX_MASTER_CLOCK / 50);
    std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    Check(elapsed.count() >= 8, "late synchronization rebases without catch-up burst");
    manager.Stop();
}

static void TestReceiveReadiness()
{
    ComLynxManager host;
    ComLynxManager client;

    Check(host.Host("127.0.0.1", 0), "start readiness host");
    Check(client.Join("127.0.0.1", host.GetStatus().port), "start readiness client");
    Check(WaitForMode(client, ComLynxModeConnected, 2000), "readiness client connected");

    u64 packets = client.GetStatus().packets_received;
    u64 network_frames = client.GetStatus().frames_received_network;
    Check(host.SendFrame(0x11, false), "send frame before client ready");
    Check(WaitForPacketsReceived(client, packets + 1, 1000), "client receives startup packet");

    ComLynxFrame frame = {};
    Check(!client.ReceiveFrame(frame), "client discards frame before UART ready");
    Check(client.GetStatus().frames_received_network == network_frames + 1, "disabled frame reaches network metric");
    Check(client.GetStatus().frames_queued == 0, "disabled frame is not queued");
    Check(client.GetStatus().frames_dropped_disabled == 1, "disabled frame drop count");
    Check(client.GetMode() == ComLynxModeConnected, "startup traffic keeps client connected");
    Check(client.GetStatus().queue_overflows == 0, "startup traffic does not overflow queue");

    client.SetReceiveEnabled(true);
    Check(host.SendFrame(0x22, true), "send frame after client ready");
    Check(WaitForFrame(client, frame, 1000), "client receives frame after UART ready");
    Check(frame.data == 0x22 && frame.parity_bit, "ready client frame contents");

    u64 queued_before_clear = client.GetStatus().frames_queued;
    Check(host.SendFrame(0x33, false), "send frame before receive clear");
    Check(WaitForFramesQueued(client, queued_before_clear + 1, 1000), "queue frame before receive clear");
    client.SetReceiveEnabled(false);
    Check(client.GetStatus().frames_dropped_clear >= 1, "receive clear drop count");

    client.Stop();
    host.Stop();
}

static void TestResetMetrics()
{
    ComLynxManager host;
    ComLynxManager client;

    Check(host.Host("127.0.0.1", 0), "start metrics reset host");
    Check(client.Join("127.0.0.1", host.GetStatus().port), "start metrics reset client");
    Check(WaitForMode(client, ComLynxModeConnected, 2000), "metrics reset client connected");
    client.SetReceiveEnabled(true);

    Check(host.SendFrame(0x5A, true), "send frame for metrics reset");
    Check(WaitForFramesQueued(client, 1, 1000), "queue frame for metrics reset");

    ComLynxStatus before = client.GetStatus();
    Check(before.packets_received > 0, "metrics reset has received packets");
    Check(before.frames_received_network > 0, "metrics reset has received frames");
    Check(before.frames_queued > 0, "metrics reset has queued frames");
    Check(before.rx_bursts > 0, "metrics reset has receive bursts");

    ComLynxMode mode_before = client.GetMode();
    bool cable_before = client.IsCableConnected();
    client.ResetMetrics();

    ComLynxStatus after = client.GetStatus();
    Check(client.GetMode() == mode_before, "metrics reset preserves mode");
    Check(client.IsCableConnected() == cable_before, "metrics reset preserves cable state");
    Check(after.packets_sent == 0 && after.packets_received == 0, "metrics reset packet counters");
    Check(after.datagrams_sent == 0 && after.datagrams_received == 0, "metrics reset datagram counters");
    Check(after.frames_generated == 0 && after.frames_sent == 0, "metrics reset transmit frame counters");
    Check(after.frames_received_network == 0 && after.frames_queued == 0 && after.frames_consumed == 0,
        "metrics reset receive frame counters");
    Check(after.frames_dropped_disabled == 0 && after.frames_dropped_clear == 0,
        "metrics reset drop counters");
    Check(after.send_eagain == 0 && after.send_errors == 0, "metrics reset send error counters");
    Check(after.duplicate_packets == 0 && after.out_of_order_packets == 0 && after.sequence_gaps == 0,
        "metrics reset sequence counters");
    Check(after.queue_overflows == 0, "metrics reset overflow counter");
    Check(after.max_outgoing_queue_depth == 0 && after.max_incoming_queue_depth == 0 &&
        after.max_pending_packet_depth == 0, "metrics reset queue high water marks");
    Check(after.frame_rx_interval_min_us == 0 && after.frame_rx_interval_avg_us == 0 &&
        after.frame_rx_interval_max_us == 0 && after.frame_rx_interval_variation_us == 0,
        "metrics reset interval statistics");
    Check(after.rx_bursts == 0 && after.rx_burst_max == 0 && after.rx_burst_total_packets == 0,
        "metrics reset burst statistics");

    ComLynxFrame frame = {};
    Check(client.ReceiveFrame(frame), "metrics reset preserves incoming queue");
    Check(frame.data == 0x5A && frame.parity_bit, "metrics reset preserves queued frame contents");

    client.Stop();
    host.Stop();
}

static void TestRestart()
{
    ComLynxManager manager;
    for (int i = 0; i < 100; i++)
    {
        Check(manager.Host("127.0.0.1", 0), "restart host bind");
        Check(manager.GetStatus().port > 0, "restart assigned port");
        manager.Stop();
        Check(manager.GetMode() == ComLynxModeDisabled, "restart host stop");
    }
}

int main()
{
    TestProtocol();
    TestQueue();
    TestConcurrentQueue();
    TestLoopback();
    TestReceiveBacklogRecovery();
    TestSixPlayerRelayBacklog();
    TestSessionCapacity();
    TestSynchronization();
    TestSynchronizationRecovery();
    TestReceiveReadiness();
    TestResetMetrics();
    TestRestart();

    if (failures == 0)
        printf("ComLynx tests passed\n");
    return failures == 0 ? 0 : 1;
}