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

#include <istream>
#include <ostream>
#include "suzy.h"
#include "media.h"
#include "memory.h"
#include "m6502.h"
#include "mikey.h"
#include "input.h"
#include "state_serializer.h"
#include "trace_logger.h"

Suzy::Suzy(Media* media, M6502* m6502, Input* input, Bus* bus)
{
    m_media = media;
    m_m6502 = m6502;
    m_input = input;
    m_bus = bus;
    InitPointer(m_mikey);
    InitPointer(m_memory);
    InitPointer(m_ram);
    InitPointer(m_trace_logger);
    m_sprite_total_cycles = 0;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_trace_math_op_a = 0;
    m_trace_math_op_b = 0;
    m_trace_math_result = 0;
    m_trace_math_elapsed = 0;
    m_trace_math_remainder = 0;
    m_trace_math_divide = false;
    m_trace_math_sign = false;
    m_trace_math_accumulate = false;
    m_trace_math_div_by_zero = false;
    m_trace_math_valid = false;
    m_trace_sprite_active = false;
#endif
    m_fast_sprite_rendering = false;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_sprite_bounding_box_mode = GLYNX_SPRITE_BOUNDING_BOX_DISABLED;
    m_sprite_bounding_box_decay = 0;
    m_sprite_bounding_box_active = false;
    m_sprite_bounding_box_valid = false;
    m_sprite_bounding_box_min_x = 0;
    m_sprite_bounding_box_min_y = 0;
    m_sprite_bounding_box_max_x = 0;
    m_sprite_bounding_box_max_y = 0;
#endif
    Reset();
}
Suzy::~Suzy()
{
}

void Suzy::Init(Memory* memory, Mikey* mikey)
{
    m_memory = memory;
    m_mikey = mikey;
    m_ram = m_memory->GetRAM();
    ComputeQuadLUT();
    Reset();
}

void Suzy::SetTraceLogger(TraceLogger* trace_logger)
{
    m_trace_logger = trace_logger;
}

void Suzy::LogMathOperationEvent(u32 op_a, u32 op_b, u32 result, u16 remainder,
    bool divide, bool sign, bool accumulate, bool div_by_zero, u32 elapsed_cycles)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (m_trace_logger->IsEventEnabled(TRACE_SUZY_MATH, TRACE_SUZY_MATH_COMPLETION))
    {
        m_trace_math_op_a = op_a;
        m_trace_math_op_b = op_b;
        m_trace_math_result = result;
        m_trace_math_remainder = remainder;
        m_trace_math_divide = divide;
        m_trace_math_sign = sign;
        m_trace_math_accumulate = accumulate;
        m_trace_math_div_by_zero = div_by_zero;
        m_trace_math_elapsed = elapsed_cycles;
        m_trace_math_valid = true;
    }
    if (m_trace_logger->IsEventEnabled(TRACE_SUZY_MATH, TRACE_SUZY_MATH_OPERATION))
    {
        GLYNX_Trace_Entry entry = {};
        entry.type = TRACE_SUZY_MATH;
        entry.math.event = TRACE_SUZY_MATH_OPERATION;
        entry.math.op_a = op_a;
        entry.math.op_b = op_b;
        entry.math.result = result;
        entry.math.remainder = remainder;
        entry.math.is_divide = divide;
        entry.math.is_signed = sign;
        entry.math.accumulate = accumulate;
        entry.math.div_by_zero = div_by_zero;
        entry.math.elapsed_cycles = elapsed_cycles;
        m_trace_logger->TraceLog(entry);
    }
#else
    UNUSED(op_a);
    UNUSED(op_b);
    UNUSED(result);
    UNUSED(remainder);
    UNUSED(divide);
    UNUSED(sign);
    UNUSED(accumulate);
    UNUSED(div_by_zero);
    UNUSED(elapsed_cycles);
#endif
}

void Suzy::LogMathCompletionEvent()
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (!m_trace_math_valid)
        return;
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_SUZY_MATH;
    entry.math.event = TRACE_SUZY_MATH_COMPLETION;
    entry.math.op_a = m_trace_math_op_a;
    entry.math.op_b = m_trace_math_op_b;
    entry.math.result = m_trace_math_result;
    entry.math.remainder = m_trace_math_remainder;
    entry.math.is_divide = m_trace_math_divide;
    entry.math.is_signed = m_trace_math_sign;
    entry.math.accumulate = m_trace_math_accumulate;
    entry.math.div_by_zero = m_trace_math_div_by_zero;
    entry.math.completed = true;
    entry.math.elapsed_cycles = m_trace_math_elapsed;
    m_trace_logger->TraceLog(entry);
#endif
}

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
void Suzy::ResetTraceEventPairing()
{
    ResetTraceMathEventPairing();
    m_trace_sprite_active = false;
}
#endif

void Suzy::LogSpriteEvent(u8 event, u8 reason)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (event == TRACE_SUZY_SPRITE_ENGINE_END && !m_trace_sprite_active)
        return;

    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_SUZY_SPRITE;
    entry.sprite.event = event;
    entry.sprite.reason = reason;
    bool next_scb = event == TRACE_SUZY_SPRITE_ENGINE_START ||
        (event == TRACE_SUZY_SPRITE_SKIP &&
        (reason == TRACE_SUZY_SPRITE_SKIP_STOPPED ||
        reason == TRACE_SUZY_SPRITE_SKIP_INVALID_TERMINAL));
    entry.sprite.scb_addr = next_scb ? m_state.SCBNEXT.value : m_state.SCBADR.value;
    entry.sprite.scb_next = m_state.SCBNEXT.value;
    entry.sprite.hpos = (s16)m_state.HPOSSTRT.value;
    entry.sprite.vpos = (s16)m_state.VPOSSTRT.value;
    entry.sprite.sprctl0 = m_state.SPRCTL0;
    entry.sprite.sprctl1 = m_state.SPRCTL1;
    entry.sprite.sprcoll = m_state.SPRCOLL;
    entry.sprite.sprinit = m_state.SPRINIT;
    entry.sprite.sprgo = m_state.SPRGO;
    entry.sprite.suzybusen = m_state.SUZYBUSEN;
    entry.sprite.bpp = (u8)(((m_state.SPRCTL0 >> 6) & 3) + 1);
    entry.sprite.type = m_state.SPRCTL0 & 7;
    entry.sprite.is_start = event == TRACE_SUZY_SPRITE_ENGINE_START;
    entry.sprite.is_end = event == TRACE_SUZY_SPRITE_ENGINE_END;
    entry.sprite.skipped = event == TRACE_SUZY_SPRITE_SKIP;
    entry.sprite.total_cycles = m_sprite_total_cycles;
    entry.sprite.collision_id = m_state.SPRCOLL & 0x0F;
    if (event == TRACE_SUZY_SPRITE_COLLISION)
    {
        u16 colpos = m_state.SCBADR.value + m_state.COLLOFF.value;
        entry.sprite.depository = m_ram[colpos];
    }
    else
        entry.sprite.depository = m_state.fred;
    entry.sprite.everon = m_state.everon;
    entry.sprite.source_pixels = m_state.row_source_pixels;
    entry.sprite.output_pixels = m_state.row_output_pixels;
    entry.sprite.charged_cycles = m_state.row_timing_charged_ticks;
    m_trace_logger->TraceLog(entry);

    if (event == TRACE_SUZY_SPRITE_ENGINE_START)
        m_trace_sprite_active = true;
    else if (event == TRACE_SUZY_SPRITE_ENGINE_END)
        m_trace_sprite_active = false;
#else
    UNUSED(event);
    UNUSED(reason);
#endif
}

void Suzy::LogInputEvent(u8 value, bool joystick)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_SUZY_INPUT;
    entry.input.event = TRACE_SUZY_INPUT_READ;
    entry.input.value = value;
    entry.input.is_joystick = joystick;
    m_trace_logger->TraceLog(entry);
#else
    UNUSED(value);
    UNUSED(joystick);
#endif
}

void Suzy::LogSpriteBusEvent(u32 cycles, u8 reason)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_SUZY_SPRITE;
    entry.sprite.event = TRACE_SUZY_SPRITE_BUS;
    entry.sprite.scb_addr = m_state.SCBADR.value;
    entry.sprite.sprctl0 = m_state.SPRCTL0;
    entry.sprite.sprctl1 = m_state.SPRCTL1;
    entry.sprite.sprcoll = m_state.SPRCOLL;
    entry.sprite.sprinit = m_state.SPRINIT;
    entry.sprite.bpp = (u8)(((m_state.SPRCTL0 >> 6) & 3) + 1);
    entry.sprite.type = m_state.SPRCTL0 & 7;
    entry.sprite.charged_cycles = cycles;
    entry.sprite.reason = reason;
    m_trace_logger->TraceLog(entry);
#else
    UNUSED(cycles);
    UNUSED(reason);
#endif
}

void Suzy::LogCartridgeEvent(u8 event, u8 value, bool write, u8 bank)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    bank = (u8)m_media->GetEffectiveCartBank(bank);
    if (event == TRACE_CARTRIDGE_STORAGE && !m_media->IsCartBankPersistent(bank))
        return;
    GLYNX_Trace_Entry entry = {};
    entry.type = TRACE_CARTRIDGE;
    entry.cart.event = event;
    entry.cart.address = m_media->GetLastCartBankAddress(bank);
    entry.cart.value = value;
    entry.cart.write = write;
    entry.cart.bank = bank;
    entry.cart.audin = m_media->GetAudin() && m_media->GetAudinValue();
    entry.cart.addr_shift = (u8)m_media->GetAddressShift();
    entry.cart.bit = m_media->GetShiftRegisterBit() ? 1 : 0;
    entry.cart.page = m_media->GetCounterValue();
    if (!m_media->GetShiftRegisterStrobe())
        entry.cart.page = (entry.cart.page - 1) & 0x7FF;
    m_trace_logger->TraceLog(entry);
#else
    UNUSED(event);
    UNUSED(value);
    UNUSED(write);
    UNUSED(bank);
#endif
}

void Suzy::SignalBlitterDone()
{
    m_mikey->SetSuzyDone();
}

void Suzy::SetFastSpriteRendering(bool enabled)
{
    m_fast_sprite_rendering = enabled;
}

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
void Suzy::SetSpriteBoundingBox(GLYNX_Sprite_Bounding_Box_Mode mode, int decay)
{
    if (mode < GLYNX_SPRITE_BOUNDING_BOX_DISABLED || mode > GLYNX_SPRITE_BOUNDING_BOX_SPRCOLL_BIT_7)
        mode = GLYNX_SPRITE_BOUNDING_BOX_DISABLED;

    m_sprite_bounding_box_mode = mode;
    m_sprite_bounding_box_decay = CLAMP(decay, 0, 10);

    if (mode == GLYNX_SPRITE_BOUNDING_BOX_DISABLED)
    {
        m_sprite_bounding_box_active = false;
        m_sprite_bounding_box_list.clear();
        m_sprite_bounding_box_list_display.clear();
    }

    for (size_t i = 0; i < m_sprite_bounding_box_list_display.size(); i++)
    {
        if (m_sprite_bounding_box_list_display[i].frames_left > m_sprite_bounding_box_decay)
            m_sprite_bounding_box_list_display[i].frames_left = (u8)m_sprite_bounding_box_decay;
    }
}
#endif

void Suzy::Reset()
{
    memset(&m_state, 0, sizeof(Suzy_State));
    m_sprite_total_cycles = 0;
    m_state.shift_register_bit = -1;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    ResetTraceEventPairing();
#endif

    for (int i = 0; i < 16; ++i)
        m_state.pen_map[i] = i;

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    m_sprite_bounding_box_active = false;
    m_sprite_bounding_box_valid = false;
    m_sprite_bounding_box_list.clear();
    m_sprite_bounding_box_list_display.clear();

    m_scb_accumulation_enabled = false;
    m_frame_scb_list.clear();
    m_frame_scb_list_display.clear();
#endif
}

void Suzy::MathRunMultiply()
{
    DebugSuzy("MathRunMultiply called");

    m_state.sprsys_lastcarrybit = false;

    u16 ab = (u16(REG_MATHA) << 8) | REG_MATHB;
    u16 cd = (u16(REG_MATHC) << 8) | REG_MATHD;

    u32 result = (u32)ab * (u32)cd;

    bool negative_result = m_state.sprsys_sign && (m_state.math_sign_A ^ m_state.math_sign_C);
    if (negative_result)
    {
        m_state.sprsys_lastcarrybit = (result != 0);
        result = (u32)(-((s32)result));
    }

    REG_MATHE = (result >> 24) & 0xFF;
    REG_MATHF = (result >> 16) & 0xFF;
    REG_MATHG = (result >> 8) & 0xFF;
    REG_MATHH = result & 0xFF;

    if (m_state.sprsys_accumulate)
    {
        u32 acc = (u32(REG_MATHJ) << 24) | (u32(REG_MATHK) << 16) | (u32(REG_MATHL) << 8) | u32(REG_MATHM);
        u64 sum = u64(acc) + u64(result);
        m_state.sprsys_lastcarrybit = (sum > 0xFFFFFFFF);
        m_state.sprsys_mathbit = (sum > 0xFFFFFFFF);
        REG_MATHJ = (sum >> 24) & 0xFF;
        REG_MATHK = (sum >> 16) & 0xFF;
        REG_MATHL = (sum >> 8) & 0xFF;
        REG_MATHM = sum & 0xFF;
    }

    m_state.sprsys_unsafe = true;
    m_state.sprsys_mathbusy = true;
    m_state.math_cycles = 44 + ((m_state.sprsys_accumulate || m_state.sprsys_sign) ? 10 : 0);

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    ResetTraceMathEventPairing();
#endif
    TraceMathOperationEvent(ab, cd, result, 0, false, m_state.sprsys_sign,
        m_state.sprsys_accumulate, false, m_state.math_cycles);
}

void Suzy::MathRunDivide()
{
    DebugSuzy("MathRunDivide called");

    m_state.sprsys_lastcarrybit = false;
    m_state.sprsys_mathbit = false;

    u32 dividend = (u32(REG_MATHE) << 24) | (u32(REG_MATHF) << 16) | (u32(REG_MATHG) << 8) | u32(REG_MATHH);
    u16 divisor = (u16(REG_MATHN) << 8) | REG_MATHP;
    bool zero_divisor = (divisor == 0);
    u32 quotient = 0;
    u16 remainder = 0;

    if (zero_divisor)
    {
        quotient = 0xFFFFFFFF;
        m_state.sprsys_mathbit = true;
        m_state.sprsys_lastcarrybit = true;
    }
    else
    {
        quotient = dividend / divisor;
        remainder = (u16)(dividend % divisor);
        m_state.sprsys_lastcarrybit = (remainder != 0);
    }

    // Result to MATHA/B/C/D
    REG_MATHA = (quotient >> 24) & 0xFF;
    REG_MATHB = (quotient >> 16) & 0xFF;
    REG_MATHC = (quotient >> 8) & 0xFF;
    REG_MATHD = quotient & 0xFF;

    // Remainder to MATHL/M, clear MATHJ/K
    REG_MATHJ = 0;
    REG_MATHK = 0;
    REG_MATHL = (remainder >> 8) & 0xFF;
    REG_MATHM = remainder & 0xFF;

    m_state.sprsys_unsafe = true;
    m_state.sprsys_mathbusy = true;
    m_state.math_cycles = 176 + (14 * l_zero16(divisor));

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    ResetTraceMathEventPairing();
#endif
    TraceMathOperationEvent(dividend, divisor, quotient, remainder, true, false,
        false, zero_divisor, m_state.math_cycles);
}

void Suzy::ComputeQuadLUT()
{
    static const int DR = 0;
    static const int DL = 1;
    static const int UR = 2;
    static const int UL = 3;

    static const int k_quad_sequence[4][4] = {
        { DR, UR, UL, DL },
        { DL, DR, UR, UL },
        { UR, UL, DL, DR },
        { UL, DL, DR, UR }
    };

    for (int quad = 0; quad < 4; quad++)
        for (int start = 0; start < 4; start++)
            for (int flip  = 0; flip  < 4; flip++)  // 0=none, 1=H, 2=V, 3=HV (bit0=H, bit1=V)
            {
                int final_quad = k_quad_sequence[start][quad] ^ flip;
                m_quad_lut[quad][start][flip].left = IS_SET_BIT(final_quad, 0);
                m_quad_lut[quad][start][flip].up = IS_SET_BIT(final_quad, 1);
            }
}

void Suzy::SaveState(std::ostream& stream)
{
    StateSerializer serializer(stream);
    Serialize(serializer, GLYNX_SAVESTATE_VERSION);
}

void Suzy::LoadState(std::istream& stream)
{
    LoadState(stream, GLYNX_SAVESTATE_VERSION);
}

void Suzy::LoadState(std::istream& stream, int version)
{
    StateSerializer serializer(stream);
    Serialize(serializer, version);
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    ResetTraceEventPairing();
#endif

    if (version < 14)
    {
        if (m_state.sprsys_spritesbusy && (m_state.sprite_cycles > 0))
            m_state.fsm_phase = SUZY_PHASE_LEGACY_DELAY;
        else
        {
            m_state.fsm_phase = SUZY_PHASE_IDLE;
            m_state.sprsys_spritesbusy = false;
            m_state.sprite_cycles = 0;
        }
    }

}

void Suzy::Serialize(StateSerializer& s, int version)
{
    G_SERIALIZE(s, m_state.TMPADR);
    G_SERIALIZE(s, m_state.TILTACUM);
    G_SERIALIZE(s, m_state.HOFF);
    G_SERIALIZE(s, m_state.VOFF);
    G_SERIALIZE(s, m_state.VIDBAS);
    G_SERIALIZE(s, m_state.COLLBAS);
    G_SERIALIZE(s, m_state.VIDADR);
    G_SERIALIZE(s, m_state.COLLADR);
    G_SERIALIZE(s, m_state.SCBNEXT);
    G_SERIALIZE(s, m_state.SPRDLINE);
    G_SERIALIZE(s, m_state.HPOSSTRT);
    G_SERIALIZE(s, m_state.VPOSSTRT);
    G_SERIALIZE(s, m_state.SPRHSIZ);
    G_SERIALIZE(s, m_state.SPRVSIZ);
    G_SERIALIZE(s, m_state.STRETCH);
    G_SERIALIZE(s, m_state.TILT);
    G_SERIALIZE(s, m_state.SPRDOFF);
    G_SERIALIZE(s, m_state.SPRVPOS);
    G_SERIALIZE(s, m_state.COLLOFF);
    G_SERIALIZE(s, m_state.VSIZACUM);
    G_SERIALIZE(s, m_state.HSIZOFF);
    G_SERIALIZE(s, m_state.VSIZOFF);
    G_SERIALIZE(s, m_state.SCBADR);
    G_SERIALIZE(s, m_state.PROCADR);
    G_SERIALIZE(s, m_state.SPRCTL0);
    G_SERIALIZE(s, m_state.SPRCTL1);
    G_SERIALIZE(s, m_state.SPRCOLL);
    G_SERIALIZE(s, m_state.SPRINIT);
    G_SERIALIZE(s, m_state.SUZYBUSEN);
    G_SERIALIZE(s, m_state.SPRGO);
    G_SERIALIZE(s, m_state.sprsys_sign);
    G_SERIALIZE(s, m_state.sprsys_accumulate);
    G_SERIALIZE(s, m_state.sprsys_dontcollide);
    G_SERIALIZE(s, m_state.sprsys_vstrech);
    G_SERIALIZE(s, m_state.sprsys_lefthand);
    G_SERIALIZE(s, m_state.sprsys_unsafe);
    G_SERIALIZE(s, m_state.sprsys_stopsprites);
    G_SERIALIZE(s, m_state.sprsys_mathbusy);
    G_SERIALIZE(s, m_state.sprsys_mathbit);
    G_SERIALIZE(s, m_state.sprsys_lastcarrybit);
    G_SERIALIZE(s, m_state.sprsys_spritesbusy);
    G_SERIALIZE_ARRAY(s, m_state.pen_map, 16);
    G_SERIALIZE(s, m_state.sprite_cycles);
    G_SERIALIZE(s, m_state.math_cycles);
    G_SERIALIZE(s, m_state.math_sign_A);
    G_SERIALIZE(s, m_state.math_sign_C);
    G_SERIALIZE(s, m_state.shift_register_address);
    G_SERIALIZE(s, m_state.shift_register_current);
    G_SERIALIZE(s, m_state.shift_register_bit);
    G_SERIALIZE(s, m_state.fred);
    G_SERIALIZE(s, m_state.everon);

    if (version >= 14)
    {
        G_SERIALIZE(s, m_state.fsm_phase);
        G_SERIALIZE(s, m_state.spr_quadrant);
        G_SERIALIZE(s, m_state.quad_row);
        G_SERIALIZE(s, m_state.quad_pixel_height);
        G_SERIALIZE(s, m_state.sprite_row_started);
        G_SERIALIZE(s, m_state.row_x);
        G_SERIALIZE(s, m_state.row_emit_count);
        G_SERIALIZE(s, m_state.row_h_accum);
        G_SERIALIZE(s, m_state.row_render);
        G_SERIALIZE(s, m_state.row_pen);
        G_SERIALIZE(s, m_state.pack_state);
        G_SERIALIZE(s, m_state.pack_count);
        G_SERIALIZE(s, m_state.pack_pen);
        G_SERIALIZE(s, m_state.pack_is_literal);
        G_SERIALIZE(s, m_state.pack_pixel_pair);
    }

    if (version >= 19)
    {
        G_SERIALIZE(s, m_state.row_collision_burst_mask);
        G_SERIALIZE(s, m_state.row_collision_read_burst_mask);
        G_SERIALIZE(s, m_state.row_video_burst_mask);
        G_SERIALIZE(s, m_state.row_video_read_burst_mask);
        G_SERIALIZE(s, m_state.row_timing_bus_ticks);
        G_SERIALIZE(s, m_state.row_timing_internal_ticks);
        G_SERIALIZE(s, m_state.row_timing_internal_base_ticks);
        G_SERIALIZE(s, m_state.row_timing_charged_ticks);
        G_SERIALIZE(s, m_state.row_source_bytes);
        G_SERIALIZE(s, m_state.row_source_pixels);
        G_SERIALIZE(s, m_state.row_output_pixels);
        G_SERIALIZE(s, m_state.row_packed_packet_ticks);
        G_SERIALIZE(s, m_state.row_packed_rle_seen);
        G_SERIALIZE(s, m_state.row_packed_literal_excess);
        G_SERIALIZE(s, m_state.row_packed_builder_stall_ticks);
        G_SERIALIZE(s, m_state.row_packed_literal_start_pixels);
        G_SERIALIZE(s, m_state.row_packed_literal_run);
        G_SERIALIZE(s, m_state.row_video_pixels);
        G_SERIALIZE(s, m_state.row_video_words);
        G_SERIALIZE(s, m_state.row_expansion_outputs);
        G_SERIALIZE(s, m_state.expansion_fifo_primed);
        G_SERIALIZE(s, m_state.row_lcd_dma_granted);
        G_SERIALIZE(s, m_state.lcd_dma_pending_ticks);
        G_SERIALIZE(s, m_state.row_collision_group_mask);
        G_SERIALIZE(s, m_state.row_collision_read_group_mask);
        G_SERIALIZE(s, m_state.row_pipeline_warm);
        G_SERIALIZE(s, m_state.scb_control_line_pending);
    }
    else if (s.IsLoading())
    {
        m_state.row_collision_burst_mask = 0;
        m_state.row_collision_read_burst_mask = 0;
        m_state.row_video_burst_mask = 0;
        m_state.row_video_read_burst_mask = 0;
        m_state.row_timing_bus_ticks = 0;
        m_state.row_timing_internal_ticks = 0;
        m_state.row_timing_internal_base_ticks = 0;
        m_state.row_timing_charged_ticks = 0;
        m_state.row_source_bytes = 0;
        m_state.row_source_pixels = 0;
        m_state.row_output_pixels = 0;
        m_state.row_packed_packet_ticks = 0;
        m_state.row_packed_rle_seen = false;
        m_state.row_packed_literal_excess = 0;
        m_state.row_packed_builder_stall_ticks = 0;
        m_state.row_packed_literal_start_pixels = 0;
        m_state.row_packed_literal_run = false;
        m_state.row_video_pixels = 0;
        m_state.row_video_words = 0;
        m_state.row_expansion_outputs = 0;
        m_state.expansion_fifo_primed = false;
        m_state.row_lcd_dma_granted = false;
        m_state.lcd_dma_pending_ticks = 0;
        m_state.row_collision_group_mask = 0;
        m_state.row_collision_read_group_mask = 0;
        m_state.row_pipeline_warm = false;
        m_state.scb_control_line_pending = false;
    }
}
