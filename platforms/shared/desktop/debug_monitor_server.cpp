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
#include "emu.h"
#include "config.h"
#include <sstream>
#include <cstring>

static const char* k_stop_reason_names[] = {
    "none", "entry", "breakpoint", "step", "pause", "run_to"
};

DebugMonitorServer::DebugMonitorServer(int port)
{
    m_core = NULL;
    m_debug_adapter = NULL;
    m_server_socket = DM_INVALID_SOCKET;
    m_client_socket = DM_INVALID_SOCKET;
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

void DebugMonitorServer::Start()
{
    if (m_running.load())
        return;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_socket == DM_INVALID_SOCKET)
    {
        Error("[DebugMonitor] Failed to create socket");
        return;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(m_port);

    if (bind(m_server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        Error("[DebugMonitor] Failed to bind to port %d", m_port);
        DM_SOCKET_CLOSE(m_server_socket);
        m_server_socket = DM_INVALID_SOCKET;
        return;
    }

    if (listen(m_server_socket, 1) < 0)
    {
        Error("[DebugMonitor] Failed to listen on socket");
        DM_SOCKET_CLOSE(m_server_socket);
        m_server_socket = DM_INVALID_SOCKET;
        return;
    }

    m_in_queue.Clear();
    m_out_queue.Reset();
    m_running.store(true);

    Log("[DebugMonitor] Listening on 127.0.0.1:%d", m_port);

    m_accept_thread = std::thread(&DebugMonitorServer::AcceptLoop, this);
    m_send_thread = std::thread(&DebugMonitorServer::SendLoop, this);
}

void DebugMonitorServer::Stop()
{
    if (!m_running.load())
        return;

    m_running.store(false);
    m_out_queue.Stop();

    {
        std::lock_guard<std::mutex> lock(m_client_mutex);
        if (m_client_socket != DM_INVALID_SOCKET)
        {
            DM_SOCKET_CLOSE(m_client_socket);
            m_client_socket = DM_INVALID_SOCKET;
        }
    }

    if (m_server_socket != DM_INVALID_SOCKET)
    {
        DM_SOCKET_CLOSE(m_server_socket);
        m_server_socket = DM_INVALID_SOCKET;
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

// ---- Network threads ----

void DebugMonitorServer::AcceptLoop()
{
    while (m_running.load())
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        dm_socket_t client = accept(m_server_socket, (struct sockaddr*)&client_addr, &client_len);

        if (client == DM_INVALID_SOCKET)
        {
            if (!m_running.load())
                break;
            Debug("[DebugMonitor] accept() failed, retrying");
            continue;
        }

        // Single client at a time -- close previous if any
        {
            std::lock_guard<std::mutex> lock(m_client_mutex);
            if (m_client_socket != DM_INVALID_SOCKET)
            {
                Log("[DebugMonitor] Closing previous client connection");
                DM_SOCKET_CLOSE(m_client_socket);
                m_client_socket = DM_INVALID_SOCKET;
                m_client_connected.store(false);
            }

            // Wait for old recv thread to finish
            if (m_recv_thread.joinable())
                m_recv_thread.join();

            int tcp_opt = 1;
#ifdef _WIN32
            setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (const char*)&tcp_opt, sizeof(tcp_opt));
#else
            setsockopt(client, IPPROTO_TCP, TCP_NODELAY, &tcp_opt, sizeof(tcp_opt));
#endif

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
    dm_socket_t my_socket;

    {
        std::lock_guard<std::mutex> lock(m_client_mutex);
        my_connection_id = m_connection_id;
        my_socket = m_client_socket;
    }

    while (m_running.load() && m_client_connected.load())
    {
        std::string json_str;
        if (!RecvMessage(my_socket, json_str))
        {
            Log("[DebugMonitor] Client disconnected (id=%u)", my_connection_id);
            break;
        }

        json msg;
        try
        {
            msg = json::parse(json_str);
        }
        catch (const json::parse_error& e)
        {
            Error("[DebugMonitor] JSON parse error: %s", e.what());
            continue;
        }

        int64_t id = msg.value("id", (int64_t)0);
        std::string cmd = msg.value("cmd", "");

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
            if (m_client_socket != DM_INVALID_SOCKET)
            {
                DM_SOCKET_CLOSE(m_client_socket);
                m_client_socket = DM_INVALID_SOCKET;
            }
            m_client_connected.store(false);
        }
    }
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
            Debug("[DebugMonitor] Dropped outbound message (stale or disconnected)");

        delete msg;
    }
}

// ---- Content-Length framed message I/O ----

bool DebugMonitorServer::RecvMessage(dm_socket_t sock, std::string& out_json)
{
    // Read headers until \r\n\r\n
    std::string header_buf;
    char c;
    int consecutive_newlines = 0;

    while (true)
    {
        int r = ::recv(sock, &c, 1, 0);
        if (r <= 0)
            return false;

        header_buf += c;

        if (header_buf.size() > 8192)
            return false;

        if (c == '\n')
            consecutive_newlines++;
        else if (c != '\r')
            consecutive_newlines = 0;

        if (consecutive_newlines >= 2)
            break;
    }

    // Parse Content-Length
    int content_length = 0;
    size_t cl_pos = header_buf.find("Content-Length:");
    if (cl_pos == std::string::npos)
        cl_pos = header_buf.find("content-length:");

    if (cl_pos != std::string::npos)
    {
        const char* p = header_buf.c_str() + cl_pos + 15;
        while (*p == ' ' || *p == '\t') p++;
        content_length = atoi(p);
    }

    if (content_length <= 0)
        return false;

    // Read body
    out_json.resize(content_length);
    int total_read = 0;
    while (total_read < content_length)
    {
        int r = ::recv(sock, &out_json[total_read], content_length - total_read, 0);
        if (r <= 0)
            return false;
        total_read += r;
    }

    return true;
}

bool DebugMonitorServer::SendMessage(dm_socket_t sock, const std::string& json_str)
{
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

    const char* reason_str = (reason >= 0 && reason <= DM_STOP_RUN_TO)
        ? k_stop_reason_names[reason] : "unknown";

    EnqueueEvent("stopped", {
        {"reason", reason_str},
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

// ---- Command dispatch ----

json DebugMonitorServer::ExecuteCommand(const std::string& cmd, const json& params)
{
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
    if (cmd == "sprite_count")      return HandleSpriteCount();
    if (cmd == "sprite_info")       return HandleSpriteInfo(params);
    if (cmd == "sprite_image")      return HandleSpriteImage(params);
    if (cmd == "palette")           return HandlePalette();
    if (cmd == "screenshot")        return HandleScreenshot();
    if (cmd == "screenshot_raw")    return HandleScreenshotRaw();
    if (cmd == "controller_button") return HandleControllerButton(params);
    if (cmd == "trace_log_set")     return HandleTraceLogSet(params);
    if (cmd == "trace_log_get")     return HandleTraceLogGet(params);

    return {{"error", "unknown command: " + cmd}};
}

// ---- Command handlers ----

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
    if (params.contains("name") && params.contains("value"))
    {
        std::string name = params["name"];
        u32 value = params["value"];
        m_debug_adapter->SetRegister(name, value);
        return {{"ok", true}};
    }
    return {{"error", "missing name or value"}};
}

json DebugMonitorServer::HandleMemoryGet(const json& params)
{
    int area = params.value("area", 0);
    u32 offset = params.value("offset", (u32)0);
    int size = params.value("size", 256);

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
    int area = params.value("area", 0);
    u32 offset = params.value("offset", (u32)0);
    std::string hex = params.value("hex", "");

    if (hex.size() % 2 != 0)
        return {{"error", "hex string must have even length"}};

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
    u16 address = params.value("address", (u16)0);
    std::string type = params.value("type", "exec");

    bool read = (type == "read" || type == "all");
    bool write = (type == "write" || type == "all");
    bool execute = (type == "exec" || type == "all");

    m_debug_adapter->SetBreakpoint(address, read, write, execute);
    return {{"ok", true}, {"address", address}};
}

json DebugMonitorServer::HandleBreakpointDelete(const json& params)
{
    u16 address = params.value("address", (u16)0);
    u16 end_address = params.value("end_address", (u16)0);
    m_debug_adapter->ClearBreakpointByAddress(address, end_address);
    return {{"ok", true}};
}

json DebugMonitorServer::HandleBreakpointList()
{
    std::vector<BreakpointInfo> bps = m_debug_adapter->ListBreakpoints();
    json arr = json::array();
    for (const auto& bp : bps)
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
    emu_debug_continue();
    return {{"ok", true}};
}

json DebugMonitorServer::HandlePause()
{
    emu_debug_break();
    return {{"ok", true}};
}

json DebugMonitorServer::HandleStepIn()
{
    emu_debug_step_into();
    return {{"ok", true}};
}

json DebugMonitorServer::HandleStepOver()
{
    emu_debug_step_over();
    return {{"ok", true}};
}

json DebugMonitorServer::HandleStepOut()
{
    emu_debug_step_out();
    return {{"ok", true}};
}

json DebugMonitorServer::HandleStepFrame()
{
    emu_debug_step_frame();
    return {{"ok", true}};
}

json DebugMonitorServer::HandleReset()
{
    emu_reset();
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
        {"stop_reason", (m_stop_reason >= 0 && m_stop_reason <= DM_STOP_RUN_TO)
            ? k_stop_reason_names[m_stop_reason] : "none"}
    };
}

json DebugMonitorServer::HandleDisassemblyGet(const json& params)
{
    u16 start = params.value("start", (u16)0);
    u16 end = params.value("end", (u16)0);

    std::vector<DisasmLine> lines = m_debug_adapter->GetDisassembly(start, end, true);
    json arr = json::array();
    for (const auto& line : lines)
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
    std::string path = params.value("path", "");
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
    for (const auto& a : areas)
    {
        arr.push_back({
            {"id", a.id},
            {"name", a.name},
            {"size", a.size},
            {"cpu_offset", a.cpu_offset}
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

json DebugMonitorServer::HandleSpriteCount()
{
    return {{"count", emu_debug_scb_count}};
}

json DebugMonitorServer::HandleSpriteInfo(const json& params)
{
    int index = params.value("index", -1);
    return m_debug_adapter->GetSprite(index, "info");
}

json DebugMonitorServer::HandleSpriteImage(const json& params)
{
    int index = params.value("index", -1);
    return m_debug_adapter->GetSprite(index, "image");
}

json DebugMonitorServer::HandlePalette()
{
    Mikey* mikey = m_core->GetMikey();
    json colors = json::array();

    for (int i = 0; i < 16; i++)
    {
        u16 color_base = 0xFDA0 + (i * 2);
        u8 green_reg = mikey->Read<true>(color_base);
        u8 bluered_reg = mikey->Read<true>(color_base + 1);

        int g = (green_reg & 0x0F) * 17;
        int b = ((bluered_reg >> 4) & 0x0F) * 17;
        int r = (bluered_reg & 0x0F) * 17;

        colors.push_back({
            {"index", i},
            {"r", r}, {"g", g}, {"b", b},
            {"green_reg", green_reg},
            {"bluered_reg", bluered_reg}
        });
    }

    return {{"colors", colors}};
}

json DebugMonitorServer::HandleScreenshot()
{
    return m_debug_adapter->GetScreenshot();
}

json DebugMonitorServer::HandleScreenshotRaw()
{
    if (!m_core || !m_core->GetMedia()->IsReady())
        return {{"error", "No media loaded"}};

    GLYNX_Runtime_Info runtime;
    m_core->GetRuntimeInfo(runtime);

    int w = runtime.screen_width;
    int h = runtime.screen_height;
    int stride = w * 4;
    int size = stride * h;

    // Base64 encode the raw RGBA data (only the visible region)
    // Using a simple base64 encoder inline
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    encoded.reserve((size * 4) / 3 + 4);

    // emu_frame_buffer is 256*4 stride, we need w*4 per row
    std::vector<u8> pixels(size);
    for (int y = 0; y < h; y++)
        memcpy(&pixels[y * stride], &emu_frame_buffer[y * 256 * 4], stride);

    const u8* data = pixels.data();
    int i = 0;
    while (i < size)
    {
        u32 a = (i < size) ? data[i++] : 0;
        u32 b_val = (i < size) ? data[i++] : 0;
        u32 c = (i < size) ? data[i++] : 0;
        u32 triple = (a << 16) | (b_val << 8) | c;
        encoded += b64[(triple >> 18) & 0x3F];
        encoded += b64[(triple >> 12) & 0x3F];
        encoded += (i > size + 1) ? '=' : b64[(triple >> 6) & 0x3F];
        encoded += (i > size) ? '=' : b64[triple & 0x3F];
    }

    return {
        {"data", encoded},
        {"width", w},
        {"height", h},
        {"format", "rgba"}
    };
}

json DebugMonitorServer::HandleControllerButton(const json& params)
{
    std::string button = params.value("button", "");
    std::string action = params.value("action", "");

    if (button.empty() || action.empty())
        return {{"error", "missing button or action"}};

    return m_debug_adapter->ControllerButton(button, action);
}

json DebugMonitorServer::HandleTraceLogSet(const json& params)
{
    bool enabled = params.value("enabled", true);
    u32 flags = params.value("flags", (u32)0xFF);
    return m_debug_adapter->SetTraceLog(enabled, flags);
}

json DebugMonitorServer::HandleTraceLogGet(const json& params)
{
    int start = params.value("start", -1);
    int count = params.value("count", 200);
    return m_debug_adapter->GetTraceLog(start, count);
}
