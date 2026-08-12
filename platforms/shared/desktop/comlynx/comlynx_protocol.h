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

#ifndef COMLYNX_PROTOCOL_H
#define COMLYNX_PROTOCOL_H

#include "comlynx.h"

#define COMLYNX_PROTOCOL_VERSION 1
#define COMLYNX_PACKET_HEADER_SIZE 32
#define COMLYNX_PACKET_MAX_PAYLOAD_SIZE 32
#define COMLYNX_PACKET_MAX_SIZE (COMLYNX_PACKET_HEADER_SIZE + COMLYNX_PACKET_MAX_PAYLOAD_SIZE)

enum ComLynxPacketType
{
    ComLynxPacketJoinRequest = 0x01,
    ComLynxPacketJoinAccept = 0x02,
    ComLynxPacketJoinReject = 0x03,
    ComLynxPacketHeartbeat = 0x04,
    ComLynxPacketLeave = 0x05,
    ComLynxPacketFrame = 0x10
};

enum ComLynxRejectReason
{
    ComLynxRejectInvalidRequest = 1,
    ComLynxRejectSessionFull = 2,
    ComLynxRejectIncompatibleVersion = 3
};

#define COMLYNX_FRAME_FLAG_PARITY 0x01
#define COMLYNX_FRAME_FLAG_BURST_END 0x02

struct ComLynxPacket
{
    u8 type;
    u64 session_id;
    u64 peer_token;
    u32 sequence;
    u8 sender_id;
    u8 flags;
    u16 payload_size;
    u8 payload[COMLYNX_PACKET_MAX_PAYLOAD_SIZE];
};

bool comlynx_packet_encode(const ComLynxPacket& packet, u8* buffer, int buffer_size, int* encoded_size);
bool comlynx_packet_decode(const u8* buffer, int buffer_size, ComLynxPacket* packet);

#endif /* COMLYNX_PROTOCOL_H */