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

#define COMLYNX_MAX_PEERS 8
#define COMLYNX_MAX_SYNC_CYCLES 128
#define COMLYNX_MAX_PROMISE_CYCLES 256
#define COMLYNX_TURBO_SYNC_CYCLES 8
#define COMLYNX_TURBO_PROMISE_CYCLES 8

typedef void (*GLYNX_ComLynx_Publish_Callback)(u64 start_cycle, u32 bit_cycles, u16 bits, void* user_data);
typedef bool (*GLYNX_ComLynx_Sample_Callback)(u64 cycle, void* user_data);
typedef void (*GLYNX_ComLynx_Break_Callback)(bool asserted, u64 cycle, void* user_data);
typedef void (*GLYNX_ComLynx_Sync_Callback)(u64 cycles, u32 promise_cycles, void* user_data);

#endif /* COMLYNX_H */