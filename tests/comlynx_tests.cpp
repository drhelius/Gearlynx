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

#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <chrono>
#include <ctime>
#include <thread>
#if !defined(_WIN32)
#include <unistd.h>
#endif
#include "comlynx/comlynx_manager.h"
#include "comlynx/comlynx_wire.h"

bool g_mcp_stdio_mode = false;

static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static u8 TestSession()
{
#if defined(_WIN32)
    return 253;
#else
    return (u8)(200 + (getpid() % 50));
#endif
}

int main()
{
    ComLynxSharedFrame shared_frame;
    Check(comlynx_shared_frame_atomics_lock_free(shared_frame),
        "shared frame atomics are lock-free");

    std::atomic<bool> frame_test_done(false);
    std::atomic<bool> frame_test_failed(false);
    std::atomic<u32> frame_test_reads(0);

    std::thread frame_reader([&shared_frame, &frame_test_done,
        &frame_test_failed, &frame_test_reads]() {
        while (!frame_test_done.load(std::memory_order_acquire))
        {
            ComLynxLocalFrame frame;
            if (!comlynx_read_shared_frame(shared_frame, 7, frame))
                continue;

            u32 expected_bit_cycles = ((u32)frame.start_cycle & 0x03FF) + 1;
            u16 expected_bits = (u16)((frame.start_cycle ^ expected_bit_cycles) & 0x07FF);

            if (frame.bit_cycles != expected_bit_cycles || frame.bits != expected_bits)
                frame_test_failed.store(true, std::memory_order_release);

            frame_test_reads.fetch_add(1, std::memory_order_relaxed);
        }
    });

    for (u32 i = 1; i <= 200000; i++)
    {
        u32 bit_cycles = (i & 0x03FF) + 1;
        u16 bits = (u16)((i ^ bit_cycles) & 0x07FF);
        comlynx_publish_shared_frame(shared_frame, 7, i, bit_cycles, bits);

        if ((i & 0xFF) == 0)
            std::this_thread::yield();
    }

    frame_test_done.store(true, std::memory_order_release);
    frame_reader.join();

    Check(frame_test_reads.load(std::memory_order_acquire) > 0,
        "concurrent shared frame reader accepts snapshots");
    Check(!frame_test_failed.load(std::memory_order_acquire),
        "concurrent shared frame snapshots are consistent");

    shared_frame.sequence.store(1, std::memory_order_relaxed);
    comlynx_publish_shared_frame(shared_frame, 9, 1234, 16, 0x0555);
    ComLynxLocalFrame recovered_frame;
    Check(comlynx_read_shared_frame(shared_frame, 9, recovered_frame),
        "shared frame publication recovers an abandoned odd sequence");
    Check(recovered_frame.start_cycle == 1234 && recovered_frame.bit_cycles == 16 &&
        recovered_frame.bits == 0x0555, "recovered shared frame payload is complete");

    Check(comlynx_heartbeat_age(100, 101) == 0,
        "newer concurrent heartbeat has zero age");
    Check(comlynx_heartbeat_age(151, 100) == 51,
        "older heartbeat reports elapsed age");
    Check(comlynx_lease_is_unchanged_and_stale(COMLYNX_DETACH_US + 1, 0, 3, 0, 3),
        "unchanged expired lease remains stale");
    Check(!comlynx_lease_is_unchanged_and_stale(COMLYNX_DETACH_US, 0, 3, 0, 3),
        "lease remains valid through its deadline");
    Check(!comlynx_lease_is_unchanged_and_stale(COMLYNX_DETACH_US + 1, 0, 3, 1, 3),
        "refreshed heartbeat cancels eviction");
    Check(!comlynx_lease_is_unchanged_and_stale(COMLYNX_DETACH_US + 1, 0, 3, 0, 4),
        "changed generation cancels eviction");

    u8 session = TestSession();
    ComLynxManager first;
    ComLynxManager second;

    Check(first.Connect(session, 0), "connect first peer");
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    Check(second.Connect(session, 0), "connect second peer");
    Check(first.GetStatus().peer_count == 2, "both peers are visible");
    Check(first.GetStatus().local_peer_id != second.GetStatus().local_peer_id,
        "peers claim distinct slots");
    Check(first.IsPacingPeer(), "first peer owns session pacing");
    Check(!second.IsPacingPeer(), "second peer follows session pacing");

    // The second peer anchors 256 cycles ahead of the first. These local
    // cycle values therefore refer to the same shared bus cycle.
    const u64 first_cycle = 400;
    const u64 second_cycle = 144;
    const u32 bit_cycles = 16;

    first.PublishFrame(first_cycle, bit_cycles, 0x07FE);
    Check(first.SampleLine(first_cycle), "manager excludes the local transmitter");
    Check(!second.SampleLine(second_cycle), "peer samples the shared start bit");
    Check(first.SampleLine(first_cycle + bit_cycles), "released data bit is high");

    second.PublishFrame(second_cycle, bit_cycles, 0x07FC);
    Check(!first.SampleLine(first_cycle + bit_cycles), "low dominates conflicting data");

    first.PublishFrame(first_cycle + 256, bit_cycles, 0x06AA);
    second.PublishFrame(second_cycle + 256, bit_cycles, 0x06AA);
    for (u64 offset = 0; offset < bit_cycles * COMLYNX_FRAME_BITS; offset++)
    {
        bool expected = ((0x06AA >> (offset / bit_cycles)) & 1) != 0;
        Check(first.SampleLine(first_cycle + 256 + offset) == expected,
            "identical simultaneous frames preserve the waveform");
    }

    second.SetBreak(true, second_cycle + 512);
    Check(!first.SampleLine(first_cycle + 512), "break drives the shared line low");
    second.SetBreak(false, second_cycle + 513);
    Check(first.SampleLine(first_cycle + 513), "break release restores idle high");

    first.Stop();
    ComLynxStatus remaining_status = second.GetStatus();
    Check(remaining_status.peer_count == 1, "stopped peer releases its slot");
    Check(remaining_status.local_peer_id == 1, "peer display index closes slot gaps");
    Check(second.IsPacingPeer(), "follower takes over session pacing");
    Check(first.Connect(session, 1000), "released slot can be reused");

    first.Stop();
    second.Stop();

    for (int i = 0; i < 100; i++)
    {
        ComLynxManager manager;
        Check(manager.Connect(session, i), "repeated map and claim");
        manager.Stop();
    }

    ComLynxManager barrier_first;
    ComLynxManager barrier_second;
    ComLynxManager barrier_third;
    Check(barrier_first.Connect(session, 0), "connect first barrier peer");
    Check(barrier_second.Connect(session, 0), "connect second barrier peer");
    Check(barrier_third.Connect(session, 0), "connect third barrier peer");

    std::chrono::steady_clock::time_point wall_start = std::chrono::steady_clock::now();
    clock_t cpu_start = clock();
    barrier_first.Synchronize(1024, COMLYNX_MAX_PROMISE_CYCLES);
    u64 wall_us = (u64)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - wall_start).count();
    u64 cpu_us = (u64)(clock() - cpu_start) * 1000000ULL / CLOCKS_PER_SEC;

    ComLynxStatus barrier_status = barrier_first.GetStatus();
    Check(barrier_status.peer_count == 1, "stale barrier peers are detached");
    Check(barrier_status.peer_detaches == 2, "both stale barrier peers are counted");
    Check(wall_us < 1500000, "stale barrier wait is bounded");
    Check(cpu_us * 2 < wall_us, "stale barrier wait does not spin the CPU");

    barrier_third.Stop();
    barrier_second.Stop();
    barrier_first.Stop();

    ComLynxManager turbo_barrier_first;
    ComLynxManager turbo_barrier_second;
    ComLynxManager turbo_barrier_third;
    Check(turbo_barrier_first.Connect(session, 0), "connect first turbo barrier peer");
    Check(turbo_barrier_second.Connect(session, 0), "connect second turbo barrier peer");
    Check(turbo_barrier_third.Connect(session, 0), "connect third turbo barrier peer");

    wall_start = std::chrono::steady_clock::now();
    cpu_start = clock();
    turbo_barrier_first.SynchronizeTurbo(1024);
    wall_us = (u64)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - wall_start).count();
    cpu_us = (u64)(clock() - cpu_start) * 1000000ULL / CLOCKS_PER_SEC;

    barrier_status = turbo_barrier_first.GetStatus();
    Check(barrier_status.peer_count == 1, "stale turbo barrier peers are detached");
    Check(barrier_status.peer_detaches == 2, "both stale turbo barrier peers are counted");
    Check(wall_us < 1500000, "stale turbo barrier wait is bounded");
    Check(cpu_us * 2 < wall_us, "stale turbo barrier wait does not spin the CPU");

    turbo_barrier_third.Stop();
    turbo_barrier_second.Stop();
    turbo_barrier_first.Stop();

    ComLynxManager live_first;
    ComLynxManager live_second;
    Check(live_first.Connect(session, 0), "connect first live lease peer");
    Check(live_second.Connect(session, 0), "connect second live lease peer");

    std::thread live_peer([&live_second]() {
        for (int i = 0; i < 60; i++)
        {
            live_second.SampleLine((u64)i);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        live_second.Stop();
    });

    wall_start = std::chrono::steady_clock::now();
    live_first.Synchronize(1024, COMLYNX_MAX_PROMISE_CYCLES);
    wall_us = (u64)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - wall_start).count();
    live_peer.join();

    Check(live_first.GetStatus().peer_detaches == 0,
        "refreshed live peer is not detached after the lease duration");
    Check(wall_us >= COMLYNX_DETACH_US,
        "live peer remains attached beyond the lease duration");
    Check(wall_us < 1500000, "orderly live peer stop releases the barrier");
    live_first.Stop();

    printf("ComLynx SHM tests passed\n");
    return 0;
}
