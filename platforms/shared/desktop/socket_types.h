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

#ifndef SOCKET_TYPES_H
#define SOCKET_TYPES_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET glynx_socket_t;
    typedef int glynx_socket_len_t;
    #define GLYNX_INVALID_SOCKET INVALID_SOCKET
    #define GLYNX_SOCKET_CLOSE(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int glynx_socket_t;
    typedef socklen_t glynx_socket_len_t;
    #define GLYNX_INVALID_SOCKET -1
    #define GLYNX_SOCKET_CLOSE(s) ::close(s)
#endif

#endif /* SOCKET_TYPES_H */