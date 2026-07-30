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

#ifndef SUZY_DEFINES_H
#define SUZY_DEFINES_H

// Shared sprite and row timing

static const u32 k_suzy_ram_read_ticks                      = 2;
static const u32 k_suzy_visible_row_ticks                   = 60;
static const u32 k_suzy_control_line_ticks                  = 54;
static const u32 k_suzy_linked_scb_control_overlap_ticks    = 48;

// Accurate renderer setup
static const u32 k_suzy_active_scb_startup_ticks            = 35;
static const u32 k_suzy_palette_fetch_ticks                 = 8;

// Legacy sprite renderer
static const u32 k_suzy_rmw_ticks                           = 1;
static const u32 k_suzy_fast_process_ticks                  = 3;
static const u32 k_suzy_packed_reload_row_ticks             = 2;
static const u32 k_suzy_packed_wide_row_discount_ticks      = 6;
static const u32 k_suzy_literal_line_ticks                  = 6;
static const u32 k_suzy_literal_byte_ticks                  = 1;
static const u32 k_suzy_literal_wide_reload_row_ticks       = 14;
static const u32 k_suzy_packed_line_ticks                   = 6;
static const u32 k_suzy_packed_pair_ticks                   = 2;
static const u32 k_suzy_packed_quad_ticks                   = 1;
static const u32 k_suzy_packed_scb_ticks                    = 13;

// Shared RAM transactions and FIFO geometry
static const u32 k_suzy_collision_clear_burst_ticks         = 10;
static const u32 k_suzy_collision_detect_burst_ticks        = 18;
static const u32 k_suzy_collision_merge_burst_ticks         = 8;
static const u32 k_suzy_source_fifo_burst_ticks             = 10;
static const u32 k_suzy_source_fifo_bytes                   = 8;
static const u32 k_suzy_source_fifo_turnover_ticks          = 1;
static const u32 k_suzy_video_write_burst_ticks             = 10;
static const u32 k_suzy_video_read_burst_ticks              = 10;
static const u32 k_suzy_video_merge_burst_ticks             = 8;

// Accurate common pipeline
static const u32 k_suzy_xor_byte_ticks                      = 2;
static const u32 k_suzy_pipeline_pixel_pair_ticks           = 5;
static const u32 k_suzy_clipped_row_ticks                   = 46;

// Accurate literal pipeline
static const u32 k_suzy_literal_1bpp_pixel_pair_ticks       = 4;
static const u32 k_suzy_literal_row_internal_ticks          = 70;
static const u32 k_suzy_literal_4bpp_upscale_ticks          = 72;
static const u32 k_suzy_literal_4bpp_collision_ticks        = 78;
static const u32 k_suzy_literal_4bpp_source_overlap_ticks   = 8;
static const u32 k_suzy_collision_pipeline_group_ticks      = 5;
static const u32 k_suzy_unpacker_shift_bits                 = 12;
static const u32 k_suzy_literal_1bpp_half_pair_ticks        = 2;
static const u32 k_suzy_pixel_builder_even_end_ticks        = 6;
static const u32 k_suzy_regular_clip_fifo_ticks             = 6;

// Accurate packed pipeline
static const u32 k_suzy_packed_packet_ticks                 = 4;
static const u32 k_suzy_packed_readiness_ticks              = 72;
static const u32 k_suzy_packed_scaled_packet_ticks          = 1;
static const u32 k_suzy_packed_scaled_packet_pair_ticks     = 3;
static const u32 k_suzy_packed_row_bus_ticks                = 12;
static const u32 k_suzy_packed_row_internal_ticks           = 24;

// LCD DMA arbitration
static const u32 k_suzy_lcd_dma_burst_ticks                 = 28;
static const u32 k_suzy_bus_grant_overhead_ticks            = 10;
static const u32 k_suzy_lcd_dma_overlappable_ticks          = 10;

#include "m6502.h"

//#define GLYNX_DEBUG_SUZY

#if defined(GLYNX_DEBUG_SUZY)
    #define DebugSuzy(msg, ...) Debug("* SUZY  [PC=%04X]: " msg, m_m6502->GetState()->PC.GetValue(), ##__VA_ARGS__)
#else
    #define DebugSuzy(msg, ...)
#endif

#define SHIFTREG_EOF 0xFFFFFFFFu

#define SUZY_TMPADRL     0xFC00
#define SUZY_TMPADRH     0xFC01
#define SUZY_TILTACUML   0xFC02
#define SUZY_TILTACUMH   0xFC03
#define SUZY_HOFFL       0xFC04
#define SUZY_HOFFH       0xFC05
#define SUZY_VOFFL       0xFC06
#define SUZY_VOFFH       0xFC07
#define SUZY_VIDBASL     0xFC08
#define SUZY_VIDBASH     0xFC09
#define SUZY_COLLBASL    0xFC0A
#define SUZY_COLLBASH    0xFC0B
#define SUZY_VIDADRL     0xFC0C
#define SUZY_VIDADRH     0xFC0D
#define SUZY_COLLADRL    0xFC0E
#define SUZY_COLLADRH    0xFC0F
#define SUZY_SCBNEXTL    0xFC10
#define SUZY_SCBNEXTH    0xFC11
#define SUZY_SPRDLINEL   0xFC12
#define SUZY_SPRDLINEH   0xFC13
#define SUZY_HPOSSTRTL   0xFC14
#define SUZY_HPOSSTRTH   0xFC15
#define SUZY_VPOSSTRTL   0xFC16
#define SUZY_VPOSSTRTH   0xFC17
#define SUZY_SPRHSIZL    0xFC18
#define SUZY_SPRHSIZH    0xFC19
#define SUZY_SPRVSIZL    0xFC1A
#define SUZY_SPRVSIZH    0xFC1B
#define SUZY_STRETCHL    0xFC1C
#define SUZY_STRETCHH    0xFC1D
#define SUZY_TILTL       0xFC1E
#define SUZY_TILTH       0xFC1F
#define SUZY_SPRDOFFL    0xFC20
#define SUZY_SPRDOFFH    0xFC21
#define SUZY_SPRVPOSL    0xFC22
#define SUZY_SPRVPOSH    0xFC23
#define SUZY_COLLOFFL    0xFC24
#define SUZY_COLLOFFH    0xFC25
#define SUZY_VSIZACUML   0xFC26
#define SUZY_VSIZACUMH   0xFC27
#define SUZY_HSIZOFFL    0xFC28
#define SUZY_HSIZOFFH    0xFC29
#define SUZY_VSIZOFFL    0xFC2A
#define SUZY_VSIZOFFH    0xFC2B
#define SUZY_SCBADRL     0xFC2C
#define SUZY_SCBADRH     0xFC2D
#define SUZY_PROCADRL    0xFC2E
#define SUZY_PROCADRH    0xFC2F
#define SUZY_MATHD       0xFC52
#define SUZY_MATHC       0xFC53
#define SUZY_MATHB       0xFC54
#define SUZY_MATHA       0xFC55
#define SUZY_MATHP       0xFC56
#define SUZY_MATHN       0xFC57
#define SUZY_MATHH       0xFC60
#define SUZY_MATHG       0xFC61
#define SUZY_MATHF       0xFC62
#define SUZY_MATHE       0xFC63
#define SUZY_MATHM       0xFC6C
#define SUZY_MATHL       0xFC6D
#define SUZY_MATHK       0xFC6E
#define SUZY_MATHJ       0xFC6F
#define SUZY_SPRCTL0     0xFC80
#define SUZY_SPRCTL1     0xFC81
#define SUZY_SPRCOLL     0xFC82
#define SUZY_SPRINIT     0xFC83
#define SUZY_SUZYHREV    0xFC88
#define SUZY_SUZYSREV    0xFC89
#define SUZY_SUZYBUSEN   0xFC90
#define SUZY_SPRGO       0xFC91
#define SUZY_SPRSYS      0xFC92
#define SUZY_JOYSTICK    0xFCB0
#define SUZY_SWITCHES    0xFCB1
#define SUZY_RCART0      0xFCB2
#define SUZY_RCART1      0xFCB3
#define SUZY_LEDS        0xFCC0
#define SUZY_PPORTSTAT   0xFCC2
#define SUZY_PPORTDATA   0xFCC3
#define SUZY_HOWIE       0xFCC4

#endif /* SUZY_DEFINES_H */
