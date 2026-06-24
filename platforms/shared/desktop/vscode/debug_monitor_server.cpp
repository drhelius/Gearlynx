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

#include "debug_monitor_server.h"
#include "common.h"
#include "../emu.h"
#include "../config.h"
#include "../rewind.h"
#include <cstring>

DebugMonitorServer::DebugMonitorServer(int port)
{
    m_core = NULL;
    m_debug_adapter = NULL;
    m_server_socket = GLYNX_INVALID_SOCKET;
    m_client_socket = GLYNX_INVALID_SOCKET;
    m_connection_id.store(0);
    m_running.store(false);
    m_client_connected.store(false);
    m_port = port;
    m_run_state = DM_STATE_STOPPED;
    m_stop_reason = DM_STOP_NONE;
    m_stop_pc = 0;
    m_event_seq = 0;
}

DebugMonitorServer::~DebugMonitorServer()
{
    Stop();
    SafeDelete(m_debug_adapter);
}

void DebugMonitorServer::Init(GearlynxCore* core)
{
    m_core = core;
    m_debug_adapter = new DebugAdapter(core);
}

bool DebugMonitorServer::Start()
{
    if (m_running.load())
        return true;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        Error("[DebugMonitor] Failed to initialize Winsock");
        return false;
    }
#endif

    m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_socket == GLYNX_INVALID_SOCKET)
    {
        Error("[DebugMonitor] Failed to create socket");
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    int opt = 1;
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(m_port);

    if (bind(m_server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        Error("[DebugMonitor] Failed to bind to port %d", m_port);
        GLYNX_SOCKET_CLOSE(m_server_socket);
        m_server_socket = GLYNX_INVALID_SOCKET;
    #ifdef _WIN32
        WSACleanup();
    #endif
        return false;
    }

    if (listen(m_server_socket, 1) < 0)
    {
        Error("[DebugMonitor] Failed to listen on socket");
        GLYNX_SOCKET_CLOSE(m_server_socket);
        m_server_socket = GLYNX_INVALID_SOCKET;
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    m_in_queue.Clear();
    m_out_queue.Reset();
    m_recv_buffer.clear();
    m_running.store(true);

    Log("[DebugMonitor] Listening on %s:%d", GetAddress(), m_port);

    m_accept_thread = std::thread(&DebugMonitorServer::AcceptLoop, this);
    m_send_thread = std::thread(&DebugMonitorServer::SendLoop, this);

    return true;
}

void DebugMonitorServer::Stop()
{
    if (!m_running.load())
        return;

    m_running.store(false);
    m_out_queue.Stop();

    {
        std::lock_guard<std::mutex> lock(m_client_mutex);
        if (m_client_socket != GLYNX_INVALID_SOCKET)
        {
            GLYNX_SOCKET_CLOSE(m_client_socket);
            m_client_socket = GLYNX_INVALID_SOCKET;
        }
    }

    if (m_server_socket != GLYNX_INVALID_SOCKET)
    {
        GLYNX_SOCKET_CLOSE(m_server_socket);
        m_server_socket = GLYNX_INVALID_SOCKET;
    }

    if (m_accept_thread.joinable())
        m_accept_thread.join();
    if (m_recv_thread.joinable())
        m_recv_thread.join();
    if (m_send_thread.joinable())
        m_send_thread.join();

    m_in_queue.Clear();
    m_out_queue.Reset();
    m_client_connected.store(false);

#ifdef _WIN32
    WSACleanup();
#endif

    Log("[DebugMonitor] Stopped");
}

bool DebugMonitorServer::IsRunning() const
{
    return m_running.load();
}

int DebugMonitorServer::GetPort() const
{
    return m_port;
}

const char* DebugMonitorServer::GetAddress() const
{
    return DM_BIND_ADDRESS;
}

// ---- Network threads ----

void DebugMonitorServer::AcceptLoop()
{
    while (m_running.load())
    {
        struct sockaddr_in client_addr;
        glynx_socket_len_t client_len = sizeof(client_addr);
        glynx_socket_t client = accept(m_server_socket, (struct sockaddr*)&client_addr, &client_len);

        if (client == GLYNX_INVALID_SOCKET)
        {
            if (!m_running.load())
                break;
            Debug("[DebugMonitor] accept() failed, retrying");
            continue;
        }

        if (!ConfigureClientSocket(client))
        {
            Error("[DebugMonitor] Failed to configure client socket");
            GLYNX_SOCKET_CLOSE(client);
            continue;
        }

        // Close previous client under lock, then join outside to avoid deadlock
        {
            std::lock_guard<std::mutex> lock(m_client_mutex);
            if (m_client_socket != GLYNX_INVALID_SOCKET)
            {
                Log("[DebugMonitor] Closing previous client connection");
                GLYNX_SOCKET_CLOSE(m_client_socket);
                m_client_socket = GLYNX_INVALID_SOCKET;
                m_client_connected.store(false);
            }
        }

        if (m_recv_thread.joinable())
            m_recv_thread.join();

        {
            std::lock_guard<std::mutex> lock(m_client_mutex);
            m_client_socket = client;
            m_connection_id++;
            m_client_connected.store(true);
        }

        Log("[DebugMonitor] Client connected (id=%u)", m_connection_id.load());
        m_recv_thread = std::thread(&DebugMonitorServer::RecvLoop, this);
    }
}

void DebugMonitorServer::RecvLoop()
{
    u32 my_connection_id;
    glynx_socket_t my_socket;

    {
        std::lock_guard<std::mutex> lock(m_client_mutex);
        my_connection_id = m_connection_id;
        my_socket = m_client_socket;
    }

    m_recv_buffer.clear();

    while (m_running.load() && m_client_connected.load())
    {
        std::string json_str;
        if (!RecvMessage(my_socket, json_str))
        {
            Log("[DebugMonitor] Client disconnected (id=%u)", my_connection_id);
            break;
        }

        json msg = json::parse(json_str, nullptr, false);
        if (msg.is_discarded())
        {
            Error("[DebugMonitor] JSON parse error");
            continue;
        }

        s64 id = 0;
        if (msg.contains("id") && !JsonReadS64(msg, "id", &id))
        {
            EnqueueResponse(0, false, {{"error", "invalid id field"}});
            continue;
        }

        std::string cmd;
        if (!JsonReadString(msg, "cmd", &cmd))
        {
            EnqueueResponse(id, false, {{"error", "missing or invalid cmd field"}});
            continue;
        }

        if (cmd.empty())
        {
            EnqueueResponse(id, false, {{"error", "missing cmd field"}});
            continue;
        }

        DebugMonitorCommand* command = new DebugMonitorCommand();
        command->id = id;
        command->cmd = cmd;
        command->params = msg;
        m_in_queue.Push(command);
    }

    {
        std::lock_guard<std::mutex> lock(m_client_mutex);
        if (m_connection_id == my_connection_id)
        {
            if (m_client_socket != GLYNX_INVALID_SOCKET)
            {
                GLYNX_SOCKET_CLOSE(m_client_socket);
                m_client_socket = GLYNX_INVALID_SOCKET;
            }
            m_client_connected.store(false);
        }
    }

    m_recv_buffer.clear();
}

void DebugMonitorServer::SendLoop()
{
    while (m_running.load())
    {
        DebugMonitorMessage* msg = m_out_queue.WaitAndPop();
        if (!msg)
            continue;

        bool sent = false;
        {
            std::lock_guard<std::mutex> lock(m_client_mutex);
            if (m_client_connected.load() && msg->connection_id == m_connection_id.load())
            {
                std::string json_str = msg->data.dump();
                sent = SendMessage(m_client_socket, json_str);
            }
        }

        if (!sent)
        {
            Debug("[DebugMonitor] Dropped outbound message (stale or disconnected)");
        }

        delete msg;
    }
}

// ---- Content-Length framed message I/O ----

bool DebugMonitorServer::RecvMessage(glynx_socket_t sock, std::string& out_json)
{
    size_t header_end = m_recv_buffer.find("\r\n\r\n");

    while (header_end == std::string::npos)
    {
        if (m_recv_buffer.size() >= DM_MAX_HEADER_SIZE)
            return false;

        char buffer[1024];
        size_t room = DM_MAX_HEADER_SIZE - m_recv_buffer.size();
        int bytes_to_read = (room < sizeof(buffer)) ? (int)room : (int)sizeof(buffer);
        int r = ::recv(sock, buffer, bytes_to_read, 0);
        if (r <= 0)
            return false;

        m_recv_buffer.append(buffer, r);
        header_end = m_recv_buffer.find("\r\n\r\n");
    }

    size_t header_size = header_end + 4;
    std::string header_buf = m_recv_buffer.substr(0, header_size);
    int content_length = 0;
    size_t cl_pos = header_buf.find("Content-Length:");
    if (cl_pos == std::string::npos)
        cl_pos = header_buf.find("content-length:");

    if (cl_pos != std::string::npos)
    {
        const char* p = header_buf.c_str() + cl_pos + 15;
        while (*p == ' ' || *p == '\t') p++;

        while (*p >= '0' && *p <= '9')
        {
            int digit = *p - '0';
            if (content_length > (DM_MAX_MESSAGE_SIZE - digit) / 10)
                return false;
            content_length = (content_length * 10) + digit;
            if (content_length > DM_MAX_MESSAGE_SIZE)
                return false;
            p++;
        }
    }

    if (content_length <= 0 || content_length > DM_MAX_MESSAGE_SIZE)
        return false;

    size_t message_size = header_size + (size_t)content_length;
    while (m_recv_buffer.size() < message_size)
    {
        char buffer[4096];
        size_t remaining = message_size - m_recv_buffer.size();
        int bytes_to_read = (remaining < sizeof(buffer)) ? (int)remaining : (int)sizeof(buffer);
        int r = ::recv(sock, buffer, bytes_to_read, 0);
        if (r <= 0)
            return false;

        m_recv_buffer.append(buffer, r);
    }

    out_json.assign(m_recv_buffer.data() + header_size, content_length);
    m_recv_buffer.erase(0, message_size);

    return true;
}

bool DebugMonitorServer::SendMessage(glynx_socket_t sock, const std::string& json_str)
{
    if (json_str.size() > DM_MAX_MESSAGE_SIZE)
        return false;

    std::string frame = "Content-Length: " + std::to_string(json_str.size()) + "\r\n\r\n" + json_str;

    int total_sent = 0;
    int frame_len = (int)frame.size();
    while (total_sent < frame_len)
    {
        int s = ::send(sock, frame.c_str() + total_sent, frame_len - total_sent, 0);
        if (s <= 0)
            return false;
        total_sent += s;
    }

    return true;
}

// ---- Emu thread: pump commands and generate events ----

void DebugMonitorServer::PumpCommands()
{
    if (!m_running.load())
        return;

    DebugMonitorCommand* cmd = NULL;
    while ((cmd = m_in_queue.Pop()) != NULL)
    {
        json result = ExecuteCommand(cmd->cmd, cmd->params);

        bool success = !result.contains("error");
        EnqueueResponse(cmd->id, success, result);

        delete cmd;
    }
}

void DebugMonitorServer::NotifyStopped(DebugMonitorStopReason reason, u16 pc)
{
    if (!m_running.load() || !m_client_connected.load())
        return;

    if (m_run_state == DM_STATE_STOPPED && m_stop_reason == reason && m_stop_pc == pc)
        return;

    m_run_state = DM_STATE_STOPPED;
    m_stop_reason = reason;
    m_stop_pc = pc;
    m_event_seq++;

    EnqueueEvent("stopped", {
        {"reason", GetStopReasonName(reason)},
        {"pc", pc},
        {"seq", m_event_seq}
    });
}

void DebugMonitorServer::NotifyResumed()
{
    if (!m_running.load() || !m_client_connected.load())
        return;

    if (m_run_state == DM_STATE_RUNNING)
        return;

    m_run_state = DM_STATE_RUNNING;
    m_event_seq++;

    EnqueueEvent("resumed", {{"seq", m_event_seq}});
}

void DebugMonitorServer::NotifyTerminated()
{
    if (!m_running.load() || !m_client_connected.load())
        return;

    m_event_seq++;
    EnqueueEvent("terminated", {{"seq", m_event_seq}});
}

void DebugMonitorServer::EnqueueResponse(int64_t id, bool success, const json& data)
{
    DebugMonitorMessage* msg = new DebugMonitorMessage();
    msg->data = {{"id", id}, {"success", success}, {"data", data}};
    msg->connection_id = m_connection_id.load();
    m_out_queue.Push(msg);
}

void DebugMonitorServer::EnqueueEvent(const std::string& event, const json& data)
{
    DebugMonitorMessage* msg = new DebugMonitorMessage();
    msg->data = {{"id", 0}, {"event", event}};
    if (!data.empty())
        msg->data["data"] = data;
    msg->connection_id = m_connection_id.load();
    m_out_queue.Push(msg);
}

// ---- Helpers ----

bool DebugMonitorServer::ConfigureClientSocket(glynx_socket_t client)
{
    int tcp_opt = 1;
    if (setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (const char*)&tcp_opt, sizeof(tcp_opt)) < 0)
        return false;

#ifdef _WIN32
    DWORD timeout = 10000;
    if (setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) < 0)
        return false;
#else
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    if (setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        return false;
#endif

    return true;
}

bool DebugMonitorServer::JsonReadU32(const json& params, const char* name, u32* value) const
{
    if (!IsValidPointer(value) || !params.contains(name))
        return false;

    const json& item = params[name];
    u64 raw_value = 0;

    if (item.is_number_unsigned())
    {
        raw_value = item.get<u64>();
    }
    else if (item.is_number_integer())
    {
        s64 signed_value = item.get<s64>();
        if (signed_value < 0)
            return false;
        raw_value = (u64)signed_value;
    }
    else
    {
        return false;
    }

    if (raw_value > 0xFFFFFFFFULL)
        return false;

    *value = (u32)raw_value;
    return true;
}

bool DebugMonitorServer::JsonReadS64(const json& params, const char* name, s64* value) const
{
    if (!IsValidPointer(value) || !params.contains(name))
        return false;

    const json& item = params[name];

    if (item.is_number_integer())
    {
        *value = item.get<s64>();
    }
    else if (item.is_number_unsigned())
    {
        u64 unsigned_value = item.get<u64>();
        if (unsigned_value > 0x7FFFFFFFFFFFFFFFULL)
            return false;
        *value = (s64)unsigned_value;
    }
    else
    {
        return false;
    }

    return true;
}

bool DebugMonitorServer::JsonReadS32Range(const json& params, const char* name, s32 min_value, s32 max_value, s32* value) const
{
    if (!IsValidPointer(value) || !params.contains(name))
        return false;

    const json& item = params[name];
    s64 raw_value = 0;

    if (item.is_number_integer())
    {
        raw_value = item.get<s64>();
    }
    else if (item.is_number_unsigned())
    {
        u64 unsigned_value = item.get<u64>();
        if (unsigned_value > 0x7FFFFFFFULL)
            return false;
        raw_value = (s64)unsigned_value;
    }
    else
    {
        return false;
    }

    if (raw_value < min_value || raw_value > max_value)
        return false;

    *value = (s32)raw_value;
    return true;
}

bool DebugMonitorServer::JsonReadU16(const json& params, const char* name, u16* value) const
{
    u32 raw_value = 0;

    if (!IsValidPointer(value) || !JsonReadU32(params, name, &raw_value) || raw_value > 0xFFFF)
        return false;

    *value = (u16)raw_value;
    return true;
}

bool DebugMonitorServer::JsonReadString(const json& params, const char* name, std::string* value) const
{
    if (!IsValidPointer(value) || !params.contains(name) || !params[name].is_string())
        return false;

    *value = params[name].get<std::string>();
    return true;
}

bool DebugMonitorServer::JsonReadBool(const json& params, const char* name, bool* value) const
{
    if (!IsValidPointer(value) || !params.contains(name) || !params[name].is_boolean())
        return false;

    *value = params[name].get<bool>();
    return true;
}

const char* DebugMonitorServer::GetStopReasonName(DebugMonitorStopReason reason) const
{
    switch (reason)
    {
        case DM_STOP_NONE:
            return "none";
        case DM_STOP_ENTRY:
            return "entry";
        case DM_STOP_BREAKPOINT:
            return "breakpoint";
        case DM_STOP_STEP:
            return "step";
        case DM_STOP_PAUSE:
            return "pause";
        case DM_STOP_RUN_TO:
            return "run_to";
        default:
            return "unknown";
    }
}

// ---- Command dispatch ----

json DebugMonitorServer::ExecuteCommand(const std::string& cmd, const json& params)
{
    if (cmd == "handshake")         return HandleHandshake();
    if (cmd == "registers_get")     return HandleRegistersGet();
    if (cmd == "registers_set")     return HandleRegistersSet(params);
    if (cmd == "memory_get")        return HandleMemoryGet(params);
    if (cmd == "memory_set")        return HandleMemorySet(params);
    if (cmd == "breakpoint_set")    return HandleBreakpointSet(params);
    if (cmd == "breakpoint_delete") return HandleBreakpointDelete(params);
    if (cmd == "breakpoint_list")   return HandleBreakpointList();
    if (cmd == "continue")          return HandleContinue();
    if (cmd == "pause")             return HandlePause();
    if (cmd == "step_in")           return HandleStepIn();
    if (cmd == "step_over")         return HandleStepOver();
    if (cmd == "step_out")          return HandleStepOut();
    if (cmd == "step_frame")        return HandleStepFrame();
    if (cmd == "reset")             return HandleReset();
    if (cmd == "status")            return HandleStatus();
    if (cmd == "disassembly_get")   return HandleDisassemblyGet(params);
    if (cmd == "load_rom")          return HandleLoadRom(params);
    if (cmd == "call_stack")        return HandleCallStack();
    if (cmd == "memory_areas")      return HandleMemoryAreas();
    if (cmd == "hardware_status")   return HandleHardwareStatus();
    if (cmd == "controller_button") return HandleControllerButton(params);
    if (cmd == "trace_log_set")     return HandleTraceLogSet(params);
    if (cmd == "trace_log_get")     return HandleTraceLogGet(params);
    if (cmd == "rewind_step_back")  return HandleRewindStepBack();

    return {{"error", "unknown command: " + cmd}};
}

// ---- Command handlers ----

json DebugMonitorServer::HandleHandshake()
{
    return {
        {"protocolVersion", DM_PROTOCOL_VERSION},
        {"emulatorVersion", GLYNX_VERSION}
    };
}

json DebugMonitorServer::HandleRegistersGet()
{
    M6502::M6502_State* state = m_core->GetM6502()->GetState();
    return {
        {"pc", state->PC.GetValue()},
        {"a", state->A.GetValue()},
        {"x", state->X.GetValue()},
        {"y", state->Y.GetValue()},
        {"s", state->S.GetValue()},
        {"p", state->P.GetValue()},
        {"cycles", state->cycles},
        {"halted", state->halted}
    };
}

json DebugMonitorServer::HandleRegistersSet(const json& params)
{
    std::string name;
    u32 value = 0;

    if (!JsonReadString(params, "name", &name) || !JsonReadU32(params, "value", &value))
        return {{"error", "missing or invalid name/value"}};

    if (name != "PC" && name != "A" && name != "X" && name != "Y" && name != "S" && name != "P")
        return {{"error", "invalid register name"}};

    m_debug_adapter->SetRegister(name, value);
    return {{"ok", true}};
}

json DebugMonitorServer::HandleMemoryGet(const json& params)
{
    s32 area = 0;
    u32 offset = 0;
    s32 size = 0;

    if (!JsonReadS32Range(params, "area", 0, MEMORY_EDITOR_MAX - 1, &area))
        return {{"error", "missing or invalid area"}};
    if (!JsonReadU32(params, "offset", &offset))
        return {{"error", "missing or invalid offset"}};
    if (!JsonReadS32Range(params, "size", 1, 0x10000, &size))
        return {{"error", "missing or invalid size"}};

    std::vector<u8> data = m_debug_adapter->ReadMemoryArea(area, offset, size);

    // Encode as hex string for efficiency
    std::string hex;
    hex.reserve(data.size() * 2);
    static const char hex_chars[] = "0123456789abcdef";
    for (u8 b : data)
    {
        hex += hex_chars[b >> 4];
        hex += hex_chars[b & 0x0f];
    }

    return {{"hex", hex}, {"size", (int)data.size()}};
}

json DebugMonitorServer::HandleMemorySet(const json& params)
{
    s32 area = 0;
    u32 offset = 0;
    std::string hex;

    if (!JsonReadS32Range(params, "area", 0, MEMORY_EDITOR_MAX - 1, &area))
        return {{"error", "missing or invalid area"}};
    if (!JsonReadU32(params, "offset", &offset))
        return {{"error", "missing or invalid offset"}};
    if (!JsonReadString(params, "hex", &hex))
        return {{"error", "missing or invalid hex"}};

    if (hex.size() % 2 != 0)
        return {{"error", "hex string must have even length"}};

    if (hex.size() > 0x20000)
        return {{"error", "hex string too large"}};

    std::vector<u8> data;
    data.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
    {
        if (!is_hex_digit(hex[i]) || !is_hex_digit(hex[i + 1]))
            return {{"error", "invalid hex character in data"}};
        u8 b = (u8)((as_hex(hex[i]) << 4) | as_hex(hex[i + 1]));
        data.push_back(b);
    }

    m_debug_adapter->WriteMemoryArea(area, offset, data);
    return {{"ok", true}};
}

json DebugMonitorServer::HandleBreakpointSet(const json& params)
{
    u16 address = 0;
    std::string type;

    if (!JsonReadU16(params, "address", &address))
        return {{"error", "missing or invalid address"}};
    if (!JsonReadString(params, "type", &type))
        return {{"error", "missing or invalid type"}};

    if (type != "read" && type != "write" && type != "exec" && type != "all")
        return {{"error", "invalid breakpoint type"}};

    bool read = (type == "read" || type == "all");
    bool write = (type == "write" || type == "all");
    bool execute = (type == "exec" || type == "all");

    m_debug_adapter->SetBreakpoint(address, read, write, execute);
    return {{"ok", true}, {"address", address}};
}

json DebugMonitorServer::HandleBreakpointDelete(const json& params)
{
    u16 address = 0;
    u16 end_address = 0;

    if (!JsonReadU16(params, "address", &address))
        return {{"error", "missing or invalid address"}};
    if (params.contains("end_address") && !JsonReadU16(params, "end_address", &end_address))
        return {{"error", "invalid end_address"}};

    m_debug_adapter->ClearBreakpointByAddress(address, end_address);
    return {{"ok", true}};
}

json DebugMonitorServer::HandleBreakpointList()
{
    std::vector<BreakpointInfo> bps = m_debug_adapter->ListBreakpoints();
    json arr = json::array();
    for (const BreakpointInfo& bp : bps)
    {
        arr.push_back({
            {"address1", bp.address1},
            {"address2", bp.address2},
            {"enabled", bp.enabled},
            {"read", bp.read},
            {"write", bp.write},
            {"execute", bp.execute},
            {"range", bp.range}
        });
    }
    return {{"breakpoints", arr}};
}

json DebugMonitorServer::HandleContinue()
{
    m_debug_adapter->Resume();
    return {{"ok", true}};
}

json DebugMonitorServer::HandlePause()
{
    m_debug_adapter->Pause();
    return {{"ok", true}};
}

json DebugMonitorServer::HandleStepIn()
{
    m_debug_adapter->StepInto();
    return {{"ok", true}};
}

json DebugMonitorServer::HandleStepOver()
{
    m_debug_adapter->StepOver();
    return {{"ok", true}};
}

json DebugMonitorServer::HandleStepOut()
{
    m_debug_adapter->StepOut();
    return {{"ok", true}};
}

json DebugMonitorServer::HandleStepFrame()
{
    m_debug_adapter->StepFrame();
    return {{"ok", true}};
}

json DebugMonitorServer::HandleReset()
{
    m_debug_adapter->Reset();
    return {{"ok", true}};
}

json DebugMonitorServer::HandleStatus()
{
    bool paused = emu_is_paused();
    bool idle = emu_is_debug_idle();
    bool empty = emu_is_empty();

    M6502::M6502_State* state = m_core->GetM6502()->GetState();

    return {
        {"paused", paused},
        {"idle", idle},
        {"empty", empty},
        {"pc", state->PC.GetValue()},
        {"run_state", m_run_state == DM_STATE_RUNNING ? "running" : "stopped"},
        {"stop_reason", GetStopReasonName(m_stop_reason)}
    };
}

json DebugMonitorServer::HandleDisassemblyGet(const json& params)
{
    u16 start = 0;
    u16 end = 0;

    if (!JsonReadU16(params, "start", &start))
        return {{"error", "missing or invalid start"}};
    if (!JsonReadU16(params, "end", &end))
        return {{"error", "missing or invalid end"}};
    if (start > end)
        return {{"error", "start must be <= end"}};

    std::vector<DisasmLine> lines = m_debug_adapter->GetDisassembly(start, end, true);
    json arr = json::array();
    for (const DisasmLine& line : lines)
    {
        arr.push_back({
            {"address", line.address},
            {"name", line.name},
            {"bytes", line.bytes},
            {"size", line.size},
            {"jump", line.jump},
            {"jump_address", line.jump_address},
            {"subroutine", line.subroutine}
        });
    }
    return {{"lines", arr}};
}

json DebugMonitorServer::HandleLoadRom(const json& params)
{
    std::string path;

    if (!JsonReadString(params, "path", &path))
        return {{"error", "missing or invalid path"}};

    if (path.empty())
        return {{"error", "missing path"}};

    bool ok = emu_load_rom(path.c_str());
    return {{"ok", ok}};
}

json DebugMonitorServer::HandleCallStack()
{
    return m_debug_adapter->ListCallStack();
}

json DebugMonitorServer::HandleMemoryAreas()
{
    std::vector<MemoryAreaInfo> areas = m_debug_adapter->ListMemoryAreas();
    json arr = json::array();
    for (const MemoryAreaInfo& area : areas)
    {
        arr.push_back({
            {"id", area.id},
            {"name", area.name},
            {"size", area.size},
            {"cpu_offset", area.cpu_offset}
        });
    }
    return {{"areas", arr}};
}

json DebugMonitorServer::HandleHardwareStatus()
{
    M6502::M6502_State* cpu = m_core->GetM6502()->GetState();
    json timers = m_debug_adapter->GetMikeyTimers();
    json audio = m_debug_adapter->GetMikeyAudio();
    json lcd = m_debug_adapter->GetLcdStatus();
    json cart = m_debug_adapter->GetCartStatus();

    return {
        {"cpu_cycles", cpu->cycles},
        {"total_ticks", cpu->total_ticks},
        {"halted", cpu->halted},
        {"irq_asserted", cpu->irq_asserted},
        {"irq_pending", cpu->irq_pending},
        {"timers", timers},
        {"audio", audio},
        {"lcd", lcd},
        {"cart", cart}
    };
}

json DebugMonitorServer::HandleControllerButton(const json& params)
{
    std::string button;
    std::string action;

    if (!JsonReadString(params, "button", &button) || !JsonReadString(params, "action", &action))
        return {{"error", "missing or invalid button/action"}};

    if (button.empty() || action.empty())
        return {{"error", "missing button or action"}};

    return m_debug_adapter->ControllerButton(button, action);
}

json DebugMonitorServer::HandleTraceLogSet(const json& params)
{
    bool enabled = true;
    u32 flags = 0xFF;
    bool debug_output = false;

    if (params.contains("enabled") && !JsonReadBool(params, "enabled", &enabled))
        return {{"error", "invalid enabled"}};
    if (params.contains("flags") && !JsonReadU32(params, "flags", &flags))
        return {{"error", "invalid flags"}};
    if (params.contains("debug_output") && !JsonReadBool(params, "debug_output", &debug_output))
        return {{"error", "invalid debug_output"}};

    return m_debug_adapter->SetTraceLog(enabled, flags, debug_output);
}

json DebugMonitorServer::HandleTraceLogGet(const json& params)
{
    int start = -1;
    int count = 200;

    if (params.contains("start"))
    {
        s32 start_value = 0;
        if (!JsonReadS32Range(params, "start", -1, 1000000000, &start_value))
            return {{"error", "invalid start"}};
        start = start_value;
    }

    if (params.contains("count"))
    {
        s32 count_value = 0;
        if (!JsonReadS32Range(params, "count", 1, 10000, &count_value))
            return {{"error", "invalid count"}};
        count = count_value;
    }

    return m_debug_adapter->GetTraceLog(start, count);
}

json DebugMonitorServer::HandleRewindStepBack()
{
    if (rewind_get_snapshot_count() < 1)
        return {{"ok", false}, {"error", "No rewind snapshots available"}};

    bool ok = rewind_pop();
    if (ok)
    {
        // Ensure emulator stays paused after rewind
        emu_debug_command = Debug_Command_None;
        u16 pc = m_core->GetM6502()->GetState()->PC.GetValue();

        if (config_debug.dis_look_ahead_count > 0)
            m_core->GetM6502()->DisassembleAhead(config_debug.dis_look_ahead_count);

        return {{"ok", true}, {"pc", pc}};
    }
    return {{"ok", false}, {"error", "Rewind pop failed"}};
}
