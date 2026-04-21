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

#include "framebuffer_ws_server.h"
#include "log.h"
#include <cstring>
#include <string>
#include <sstream>

// Minimal SHA-1 for WebSocket handshake
// Only used for the Sec-WebSocket-Accept header
static void sha1(const u8* data, size_t len, u8 hash[20])
{
    u32 h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;

    size_t ml = len * 8;
    size_t padded_len = ((len + 8) / 64 + 1) * 64;
    u8* msg = new u8[padded_len];
    memset(msg, 0, padded_len);
    memcpy(msg, data, len);
    msg[len] = 0x80;
    for (int i = 0; i < 8; i++)
        msg[padded_len - 1 - i] = (u8)(ml >> (i * 8));

    for (size_t offset = 0; offset < padded_len; offset += 64)
    {
        u32 w[80];
        for (int i = 0; i < 16; i++)
            w[i] = ((u32)msg[offset + i * 4] << 24) | ((u32)msg[offset + i * 4 + 1] << 16)
                  | ((u32)msg[offset + i * 4 + 2] << 8) | (u32)msg[offset + i * 4 + 3];
        for (int i = 16; i < 80; i++)
        {
            u32 t = w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16];
            w[i] = (t << 1) | (t >> 31);
        }

        u32 a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; i++)
        {
            u32 f, k;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }
            u32 temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = temp;
        }
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }
    delete[] msg;

    u32 h[] = {h0, h1, h2, h3, h4};
    for (int i = 0; i < 5; i++)
    {
        hash[i*4] = (u8)(h[i] >> 24);
        hash[i*4+1] = (u8)(h[i] >> 16);
        hash[i*4+2] = (u8)(h[i] >> 8);
        hash[i*4+3] = (u8)(h[i]);
    }
}

static std::string base64_encode_raw(const u8* data, size_t len)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((len * 4) / 3 + 4);
    size_t i = 0;
    while (i < len)
    {
        u32 a = (i < len) ? data[i++] : 0;
        u32 b_val = (i < len) ? data[i++] : 0;
        u32 c = (i < len) ? data[i++] : 0;
        u32 triple = (a << 16) | (b_val << 8) | c;
        out += b64[(triple >> 18) & 0x3F];
        out += b64[(triple >> 12) & 0x3F];
        out += (i > len + 1) ? '=' : b64[(triple >> 6) & 0x3F];
        out += (i > len) ? '=' : b64[triple & 0x3F];
    }
    return out;
}

FramebufferWsServer::FramebufferWsServer(int port)
{
    m_port = port;
    m_server_socket = FB_INVALID_SOCKET;
    m_client_socket = FB_INVALID_SOCKET;
    m_running.store(false);
    m_client_connected.store(false);
    m_frame_buffer = NULL;
    m_frame_width = 0;
    m_frame_height = 0;
    m_frame_size = 0;
    m_frame_ready.store(false);
}

FramebufferWsServer::~FramebufferWsServer()
{
    Stop();
    delete[] m_frame_buffer;
}

void FramebufferWsServer::Start()
{
    if (m_running.load())
        return;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_socket == FB_INVALID_SOCKET)
    {
        Error("[FramebufferWS] Failed to create socket");
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
        Error("[FramebufferWS] Failed to bind to port %d", m_port);
        FB_SOCKET_CLOSE(m_server_socket);
        m_server_socket = FB_INVALID_SOCKET;
        return;
    }

    if (listen(m_server_socket, 1) < 0)
    {
        Error("[FramebufferWS] Failed to listen");
        FB_SOCKET_CLOSE(m_server_socket);
        m_server_socket = FB_INVALID_SOCKET;
        return;
    }

    m_running.store(true);
    Log("[FramebufferWS] Listening on 127.0.0.1:%d", m_port);

    m_accept_thread = std::thread(&FramebufferWsServer::AcceptLoop, this);
}

void FramebufferWsServer::Stop()
{
    if (!m_running.load())
        return;

    m_running.store(false);
    m_frame_ready.store(false);

    {
        std::lock_guard<std::mutex> lock(m_client_mutex);
        if (m_client_socket != FB_INVALID_SOCKET)
        {
            FB_SOCKET_CLOSE(m_client_socket);
            m_client_socket = FB_INVALID_SOCKET;
        }
    }

    if (m_server_socket != FB_INVALID_SOCKET)
    {
        FB_SOCKET_CLOSE(m_server_socket);
        m_server_socket = FB_INVALID_SOCKET;
    }

    if (m_accept_thread.joinable())
        m_accept_thread.join();
    if (m_client_thread.joinable())
        m_client_thread.join();

    m_client_connected.store(false);

    Log("[FramebufferWS] Stopped");
}

bool FramebufferWsServer::IsRunning() const
{
    return m_running.load();
}

int FramebufferWsServer::GetPort() const
{
    return m_port;
}

void FramebufferWsServer::PushFrame(const u8* framebuffer, int width, int height, int stride_pixels)
{
    if (!m_running.load() || !m_client_connected.load())
        return;

    int row_bytes = width * 4;
    int total = row_bytes * height;

    {
        std::lock_guard<std::mutex> lock(m_frame_mutex);
        if (!m_frame_buffer || m_frame_size < total)
        {
            delete[] m_frame_buffer;
            m_frame_buffer = new u8[total];
        }
        m_frame_width = width;
        m_frame_height = height;
        m_frame_size = total;

        // Copy with stride correction
        int src_stride = stride_pixels * 4;
        for (int y = 0; y < height; y++)
            memcpy(&m_frame_buffer[y * row_bytes], &framebuffer[y * src_stride], row_bytes);
    }

    m_frame_ready.store(true);
}

void FramebufferWsServer::AcceptLoop()
{
    while (m_running.load())
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        fb_socket_t client = accept(m_server_socket, (struct sockaddr*)&client_addr, &client_len);

        if (client == FB_INVALID_SOCKET)
        {
            if (!m_running.load()) break;
            continue;
        }

        // Close previous client
        {
            std::lock_guard<std::mutex> lock(m_client_mutex);
            if (m_client_socket != FB_INVALID_SOCKET)
            {
                FB_SOCKET_CLOSE(m_client_socket);
                m_client_socket = FB_INVALID_SOCKET;
                m_client_connected.store(false);
            }
            if (m_client_thread.joinable())
                m_client_thread.join();
        }

        int tcp_opt = 1;
#ifdef _WIN32
        setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (const char*)&tcp_opt, sizeof(tcp_opt));
#else
        setsockopt(client, IPPROTO_TCP, TCP_NODELAY, &tcp_opt, sizeof(tcp_opt));
#endif

        Log("[FramebufferWS] Client connected");
        m_client_thread = std::thread(&FramebufferWsServer::ClientLoop, this, client);
    }
}

void FramebufferWsServer::ClientLoop(fb_socket_t client)
{
    if (!DoWebSocketHandshake(client))
    {
        FB_SOCKET_CLOSE(client);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_client_mutex);
        m_client_socket = client;
        m_client_connected.store(true);
    }

    Log("[FramebufferWS] WebSocket handshake complete, streaming frames");

    while (m_running.load() && m_client_connected.load())
    {
        if (!m_frame_ready.load())
        {
            // Sleep a bit to avoid busy-waiting
#ifdef _WIN32
            Sleep(1);
#else
            usleep(1000);
#endif
            continue;
        }

        m_frame_ready.store(false);

        // Send frame header (8 bytes: width u16 LE, height u16 LE, reserved u32)
        // followed by raw RGBA pixels
        u8 header[8];
        int w, h, size;
        {
            std::lock_guard<std::mutex> lock(m_frame_mutex);
            w = m_frame_width;
            h = m_frame_height;
            size = m_frame_size;
        }

        header[0] = (u8)(w & 0xFF);
        header[1] = (u8)((w >> 8) & 0xFF);
        header[2] = (u8)(h & 0xFF);
        header[3] = (u8)((h >> 8) & 0xFF);
        header[4] = header[5] = header[6] = header[7] = 0;

        // Build complete message: header + pixels
        int msg_size = 8 + size;
        u8* msg = new u8[msg_size];
        memcpy(msg, header, 8);
        {
            std::lock_guard<std::mutex> lock(m_frame_mutex);
            memcpy(msg + 8, m_frame_buffer, size);
        }

        bool ok = SendWebSocketBinaryFrame(client, msg, msg_size);
        delete[] msg;

        if (!ok)
        {
            Log("[FramebufferWS] Send failed, client disconnected");
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_client_mutex);
        if (m_client_socket == client)
        {
            FB_SOCKET_CLOSE(m_client_socket);
            m_client_socket = FB_INVALID_SOCKET;
            m_client_connected.store(false);
        }
    }
}

bool FramebufferWsServer::DoWebSocketHandshake(fb_socket_t client)
{
    char buf[2048];
    int total = 0;

    while (total < (int)sizeof(buf) - 1)
    {
        int r = ::recv(client, buf + total, 1, 0);
        if (r <= 0) return false;
        total += r;
        buf[total] = '\0';
        if (total >= 4 && strstr(buf + total - 4, "\r\n\r\n"))
            break;
    }

    // Find Sec-WebSocket-Key
    const char* key_header = strstr(buf, "Sec-WebSocket-Key:");
    if (!key_header) key_header = strstr(buf, "sec-websocket-key:");
    if (!key_header) return false;

    key_header += 18;
    while (*key_header == ' ') key_header++;
    const char* key_end = strstr(key_header, "\r\n");
    if (!key_end) return false;

    std::string key(key_header, key_end - key_header);
    key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; // WebSocket magic GUID

    u8 hash[20];
    sha1((const u8*)key.c_str(), key.length(), hash);
    std::string accept_key = base64_encode_raw(hash, 20);

    std::string response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept_key + "\r\n"
        "\r\n";

    int sent = ::send(client, response.c_str(), (int)response.length(), 0);
    return sent == (int)response.length();
}

bool FramebufferWsServer::SendWebSocketBinaryFrame(fb_socket_t client, const u8* data, size_t len)
{
    // WebSocket binary frame header
    u8 header[10];
    int header_len;

    header[0] = 0x82; // FIN + binary opcode

    if (len < 126)
    {
        header[1] = (u8)len;
        header_len = 2;
    }
    else if (len < 65536)
    {
        header[1] = 126;
        header[2] = (u8)((len >> 8) & 0xFF);
        header[3] = (u8)(len & 0xFF);
        header_len = 4;
    }
    else
    {
        header[1] = 127;
        for (int i = 0; i < 8; i++)
            header[2 + i] = (u8)((len >> (56 - i * 8)) & 0xFF);
        header_len = 10;
    }

    // Send header
    int s = ::send(client, (const char*)header, header_len, 0);
    if (s != header_len) return false;

    // Send payload in chunks
    size_t total_sent = 0;
    while (total_sent < len)
    {
        size_t remain = len - total_sent;
        int chunk = (int)(remain < 65536 ? remain : 65536);
        s = ::send(client, (const char*)(data + total_sent), chunk, 0);
        if (s <= 0) return false;
        total_sent += s;
    }

    return true;
}
