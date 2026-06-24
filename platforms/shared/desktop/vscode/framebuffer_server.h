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

#ifndef FRAMEBUFFER_SERVER_H
#define FRAMEBUFFER_SERVER_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "common.h"
#include "socket_types.h"

#define FB_MAX_DIMENSION GLYNX_SCREEN_WIDTH
#define FB_MAX_FRAME_SIZE (FB_MAX_DIMENSION * FB_MAX_DIMENSION * 4)

class FramebufferServer
{
public:
    FramebufferServer(int port = 6503);
    ~FramebufferServer();

    bool Start();
    void Stop();
    bool IsRunning() const;
    int GetPort() const;

    // Called from emu thread after each frame
    void PushFrame(const u8* framebuffer, int width, int height, int stride_pixels);

private:
    void AcceptLoop();
    void ClientLoop(glynx_socket_t client);
    bool ConfigureClientSocket(glynx_socket_t client);
    bool SendAll(glynx_socket_t client, const u8* data, int size);

    int m_port;
    glynx_socket_t m_server_socket;
    glynx_socket_t m_client_socket;
    std::mutex m_client_mutex;
    std::thread m_accept_thread;
    std::thread m_client_thread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_client_connected;

    // Double-buffered frame data
    u8* m_frame_buffer;
    int m_frame_width;
    int m_frame_height;
    int m_frame_size;
    std::mutex m_frame_mutex;
    std::condition_variable m_frame_cv;
    std::atomic<bool> m_frame_ready;
};

#endif /* FRAMEBUFFER_SERVER_H */
