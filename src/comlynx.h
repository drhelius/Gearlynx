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

#ifndef COMLYNX_H
#define COMLYNX_H

#include "types.h"

#define COMLYNX_DEFAULT_PORT 7800
#define COMLYNX_MAX_PEERS 8
#define COMLYNX_SYNC_CYCLES 4096

// ComLynx is a single half-duplex open-collector bus, so a node holds the line
// for a whole back-to-back byte burst. burst_end marks the last byte of a burst
// and source identifies the peer that drove the line (0 = local loopback).
typedef void (*GLYNX_ComLynx_TX_Callback)(u8 data, bool parity_bit, bool burst_end, void* user_data);
typedef bool (*GLYNX_ComLynx_RX_Callback)(u8* data, bool* parity_bit, u8* source, void* user_data);
typedef void (*GLYNX_ComLynx_Sync_Callback)(u64 cycles, void* user_data);

#endif /* COMLYNX_H */