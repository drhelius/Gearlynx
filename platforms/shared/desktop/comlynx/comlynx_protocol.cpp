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

#include <cstring>
#include "common.h"
#include "comlynx_protocol.h"

static const u8 k_comlynx_magic[4] = {'G', 'L', 'C', 'X'};

static bool validate_packet(const ComLynxPacket& packet)
{
    if (packet.payload_size > COMLYNX_PACKET_MAX_PAYLOAD_SIZE)
        return false;

    switch (packet.type)
    {
        case ComLynxPacketJoinRequest:
            return packet.session_id == 0 && packet.peer_token != 0 &&
                packet.sequence == 0 && packet.sender_id == 0 &&
                packet.flags == 0 && packet.payload_size == 0;
        case ComLynxPacketJoinAccept:
            return packet.session_id != 0 && packet.peer_token != 0 &&
                packet.sender_id >= 2 && packet.sender_id <= COMLYNX_MAX_PEERS &&
                packet.flags == 0 && packet.payload_size == 0;
        case ComLynxPacketJoinReject:
            return packet.peer_token != 0 && packet.sender_id == 1 &&
                packet.flags == 0 && packet.payload_size == 1;
        case ComLynxPacketHeartbeat:
        case ComLynxPacketLeave:
            return packet.session_id != 0 && packet.peer_token != 0 &&
                packet.sender_id >= 1 && packet.sender_id <= COMLYNX_MAX_PEERS &&
                packet.flags == 0 && packet.payload_size == 0;
        case ComLynxPacketFrame:
            return packet.session_id != 0 && packet.peer_token != 0 &&
                packet.sender_id >= 1 && packet.sender_id <= COMLYNX_MAX_PEERS &&
                (packet.flags & ~(COMLYNX_FRAME_FLAG_PARITY | COMLYNX_FRAME_FLAG_BURST_END)) == 0 &&
                packet.payload_size == 1;
        default:
            return false;
    }
}

bool comlynx_packet_encode(const ComLynxPacket& packet, u8* buffer, int buffer_size, int* encoded_size)
{
    if (!buffer || !encoded_size || !validate_packet(packet))
        return false;

    int size = COMLYNX_PACKET_HEADER_SIZE + packet.payload_size;
    if (buffer_size < size)
        return false;

    memcpy(buffer, k_comlynx_magic, sizeof(k_comlynx_magic));
    buffer[4] = COMLYNX_PROTOCOL_VERSION;
    buffer[5] = packet.type;
    write_u16_be(buffer + 6, packet.payload_size);
    write_u64_be(buffer + 8, packet.session_id);
    write_u64_be(buffer + 16, packet.peer_token);
    write_u32_be(buffer + 24, packet.sequence);
    buffer[28] = packet.sender_id;
    buffer[29] = packet.flags;
    buffer[30] = 0;
    buffer[31] = 0;

    if (packet.payload_size > 0)
        memcpy(buffer + COMLYNX_PACKET_HEADER_SIZE, packet.payload, packet.payload_size);

    *encoded_size = size;
    return true;
}

bool comlynx_packet_decode(const u8* buffer, int buffer_size, ComLynxPacket* packet)
{
    if (!buffer || !packet || buffer_size < COMLYNX_PACKET_HEADER_SIZE ||
        buffer_size > COMLYNX_PACKET_MAX_SIZE)
        return false;

    if (memcmp(buffer, k_comlynx_magic, sizeof(k_comlynx_magic)) != 0 ||
        buffer[4] != COMLYNX_PROTOCOL_VERSION || buffer[30] != 0 || buffer[31] != 0)
        return false;

    u16 payload_size = read_u16_be(buffer + 6);
    if (payload_size > COMLYNX_PACKET_MAX_PAYLOAD_SIZE ||
        buffer_size != COMLYNX_PACKET_HEADER_SIZE + payload_size)
        return false;

    ComLynxPacket decoded = {};
    decoded.type = buffer[5];
    decoded.payload_size = payload_size;
    decoded.session_id = read_u64_be(buffer + 8);
    decoded.peer_token = read_u64_be(buffer + 16);
    decoded.sequence = read_u32_be(buffer + 24);
    decoded.sender_id = buffer[28];
    decoded.flags = buffer[29];

    if (payload_size > 0)
        memcpy(decoded.payload, buffer + COMLYNX_PACKET_HEADER_SIZE, payload_size);

    if (!validate_packet(decoded))
        return false;

    *packet = decoded;
    return true;
}