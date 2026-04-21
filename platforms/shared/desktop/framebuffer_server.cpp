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

#include "framebuffer_server.h"
#include "log.h"
#include <cstring>

FramebufferServer::FramebufferServer(int port)
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

FramebufferServer::~FramebufferServer()
{
    Stop();
    delete[] m_frame_buffer;
}

void FramebufferServer::Start()
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
        Error("[FramebufferStream] Failed to create socket");
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
        Error("[FramebufferStream] Failed to bind to port %d", m_port);
        FB_SOCKET_CLOSE(m_server_socket);
        m_server_socket = FB_INVALID_SOCKET;
        return;
    }

    if (listen(m_server_socket, 1) < 0)
    {
        Error("[FramebufferStream] Failed to listen");
        FB_SOCKET_CLOSE(m_server_socket);
        m_server_socket = FB_INVALID_SOCKET;
        return;
    }

    m_running.store(true);
    Log("[FramebufferStream] Listening on 127.0.0.1:%d", m_port);

    m_accept_thread = std::thread(&FramebufferServer::AcceptLoop, this);
}

void FramebufferServer::Stop()
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

    Log("[FramebufferStream] Stopped");
}

bool FramebufferServer::IsRunning() const
{
    return m_running.load();
}

int FramebufferServer::GetPort() const
{
    return m_port;
}

void FramebufferServer::PushFrame(const u8* framebuffer, int width, int height, int stride_pixels)
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

        int src_stride = stride_pixels * 4;
        for (int y = 0; y < height; y++)
            memcpy(&m_frame_buffer[y * row_bytes], &framebuffer[y * src_stride], row_bytes);
    }

    m_frame_ready.store(true);
}

void FramebufferServer::AcceptLoop()
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

        Log("[FramebufferStream] Client connected");
        m_client_thread = std::thread(&FramebufferServer::ClientLoop, this, client);
    }
}

void FramebufferServer::ClientLoop(fb_socket_t client)
{
    {
        std::lock_guard<std::mutex> lock(m_client_mutex);
        m_client_socket = client;
        m_client_connected.store(true);
    }

    // Raw TCP binary stream: 8-byte header + RGBA pixels per frame
    // Header: width (u16 LE), height (u16 LE), size (u32 LE)
    while (m_running.load() && m_client_connected.load())
    {
        if (!m_frame_ready.load())
        {
#ifdef _WIN32
            Sleep(1);
#else
            usleep(1000);
#endif
            continue;
        }

        m_frame_ready.store(false);

        int w, h, size;
        {
            std::lock_guard<std::mutex> lock(m_frame_mutex);
            w = m_frame_width;
            h = m_frame_height;
            size = m_frame_size;
        }

        u8 header[8];
        header[0] = (u8)(w & 0xFF);
        header[1] = (u8)((w >> 8) & 0xFF);
        header[2] = (u8)(h & 0xFF);
        header[3] = (u8)((h >> 8) & 0xFF);
        header[4] = (u8)(size & 0xFF);
        header[5] = (u8)((size >> 8) & 0xFF);
        header[6] = (u8)((size >> 16) & 0xFF);
        header[7] = (u8)((size >> 24) & 0xFF);

        int sent = ::send(client, (const char*)header, 8, 0);
        if (sent != 8)
        {
            Log("[FramebufferStream] Send header failed, client disconnected");
            break;
        }

        {
            std::lock_guard<std::mutex> lock(m_frame_mutex);
            int total_sent = 0;
            while (total_sent < size)
            {
                int remain = size - total_sent;
                int chunk = remain < 65536 ? remain : 65536;
                sent = ::send(client, (const char*)(m_frame_buffer + total_sent), chunk, 0);
                if (sent <= 0) break;
                total_sent += sent;
            }
            if (total_sent < size)
            {
                Log("[FramebufferStream] Send pixels failed, client disconnected");
                break;
            }
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

