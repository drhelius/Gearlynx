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

#ifndef DEBUG_MONITOR_SERVER_H
#define DEBUG_MONITOR_SERVER_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include "json.hpp"
#include "gearlynx.h"
#include "mcp/mcp_debug_adapter.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET dm_socket_t;
    #define DM_INVALID_SOCKET INVALID_SOCKET
    #define DM_SOCKET_CLOSE(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    typedef int dm_socket_t;
    #define DM_INVALID_SOCKET -1
    #define DM_SOCKET_CLOSE(s) ::close(s)
#endif

using json = nlohmann::json;

enum DebugMonitorStopReason
{
    DM_STOP_NONE = 0,
    DM_STOP_ENTRY,
    DM_STOP_BREAKPOINT,
    DM_STOP_STEP,
    DM_STOP_PAUSE,
    DM_STOP_RUN_TO
};

enum DebugMonitorRunState
{
    DM_STATE_STOPPED = 0,
    DM_STATE_RUNNING
};

struct DebugMonitorCommand
{
    int64_t id;
    std::string cmd;
    json params;
};

struct DebugMonitorMessage
{
    json data;
    u32 connection_id;
};

class DebugMonitorOutQueue
{
public:
    DebugMonitorOutQueue() : m_running(true) {}

    void Push(DebugMonitorMessage* msg)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(msg);
        m_cv.notify_one();
    }

    DebugMonitorMessage* WaitAndPop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running; });
        if (m_queue.empty())
            return NULL;
        DebugMonitorMessage* msg = m_queue.front();
        m_queue.pop();
        return msg;
    }

    void Stop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
        m_cv.notify_all();
    }

    void Reset()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty())
        {
            delete m_queue.front();
            m_queue.pop();
        }
        m_running = true;
    }

private:
    std::queue<DebugMonitorMessage*> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_running;
};

class DebugMonitorInQueue
{
public:
    void Push(DebugMonitorCommand* cmd)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(cmd);
    }

    DebugMonitorCommand* Pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return NULL;
        DebugMonitorCommand* cmd = m_queue.front();
        m_queue.pop();
        return cmd;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty())
        {
            delete m_queue.front();
            m_queue.pop();
        }
    }

private:
    std::queue<DebugMonitorCommand*> m_queue;
    std::mutex m_mutex;
};

class DebugMonitorServer
{
public:
    DebugMonitorServer(int port = 6502);
    ~DebugMonitorServer();

    void Init(GearlynxCore* core);
    void Start();
    void Stop();
    bool IsRunning() const;
    int GetPort() const;

    // Called from emu thread each frame
    void PumpCommands();

    // Called from emu thread to notify state changes
    void NotifyStopped(DebugMonitorStopReason reason, u16 pc);
    void NotifyResumed();
    void NotifyTerminated();

private:
    void AcceptLoop();
    void RecvLoop();
    void SendLoop();

    bool RecvMessage(dm_socket_t sock, std::string& out_json);
    bool SendMessage(dm_socket_t sock, const std::string& json_str);

    json ExecuteCommand(const std::string& cmd, const json& params);

    // Command handlers
    json HandleRegistersGet();
    json HandleRegistersSet(const json& params);
    json HandleMemoryGet(const json& params);
    json HandleMemorySet(const json& params);
    json HandleBreakpointSet(const json& params);
    json HandleBreakpointDelete(const json& params);
    json HandleBreakpointList();
    json HandleContinue();
    json HandlePause();
    json HandleStepIn();
    json HandleStepOver();
    json HandleStepOut();
    json HandleStepFrame();
    json HandleReset();
    json HandleStatus();
    json HandleDisassemblyGet(const json& params);
    json HandleLoadRom(const json& params);
    json HandleCallStack();
    json HandleMemoryAreas();
    json HandleHardwareStatus();
    json HandleSpriteCount();
    json HandleSpriteInfo(const json& params);
    json HandleSpriteImage(const json& params);
    json HandlePalette();
    json HandleScreenshot();
    json HandleScreenshotRaw();
    json HandleControllerButton(const json& params);
    json HandleTraceLogSet(const json& params);
    json HandleTraceLogGet(const json& params);

    void EnqueueResponse(int64_t id, bool success, const json& data);
    void EnqueueEvent(const std::string& event, const json& data = json::object());

    GearlynxCore* m_core;
    DebugAdapter* m_debug_adapter;

    DebugMonitorInQueue m_in_queue;
    DebugMonitorOutQueue m_out_queue;

    dm_socket_t m_server_socket;
    dm_socket_t m_client_socket;
    std::mutex m_client_mutex;
    std::atomic<u32> m_connection_id;

    std::thread m_accept_thread;
    std::thread m_recv_thread;
    std::thread m_send_thread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_client_connected;

    int m_port;

    // Stop state (written on emu thread, read for event generation)
    DebugMonitorRunState m_run_state;
    DebugMonitorStopReason m_stop_reason;
    u16 m_stop_pc;
    u32 m_event_seq;
};

#endif /* DEBUG_MONITOR_SERVER_H */
