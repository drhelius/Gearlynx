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

#include <cstdio>
#include <cstdlib>
#include "comlynx/comlynx_wire.h"

static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main()
{
    ComLynxWireFrame idle[1] = {{100, 16, 0x07FF}};
    Check(comlynx_wire_level(NULL, 0, 0), "empty wire is released");
    Check(comlynx_wire_level(idle, 1, 100), "released frame stays high");

    ComLynxWireFrame frame = {100, 16, 0x060A};
    Check(comlynx_frame_level(frame, 99), "frame is idle before start");
    Check(!comlynx_frame_level(frame, 100), "start bit drives low");
    Check(comlynx_frame_level(frame, 116), "first data bit is released");
    Check(comlynx_frame_level(frame, 276), "frame is idle after stop bit");

    ComLynxWireFrame overlap[2] = {
        {100, 16, 0x07FE},
        {108, 16, 0x07FF}
    };
    Check(!comlynx_wire_level(overlap, 2, 100), "low dominates a released peer");
    Check(!comlynx_wire_level(overlap, 2, 108), "partial overlap remains low");

    ComLynxWireFrame identical[2] = {
        {200, 16, 0x06AA},
        {200, 16, 0x06AA}
    };
    for (u64 cycle = 200; cycle < 376; cycle++)
    {
        Check(comlynx_wire_level(identical, 2, cycle) ==
            comlynx_frame_level(identical[0], cycle),
            "identical simultaneous frames do not collide");
    }

    ComLynxWireFrame conflict[2] = {
        {300, 16, 0x07FE},
        {300, 16, 0x07FC}
    };
    Check(!comlynx_wire_level(conflict, 2, 316), "conflicting data resolves low");

    printf("ComLynx wire tests passed\n");
    return 0;
}