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

#ifndef MIKEY_H
#define MIKEY_H

#include <iostream>
#include <fstream>
#include "common.h"
#include "comlynx.h"
#include "mikey_defines.h"
#include "random.h"

class Media;
class Memory;
class M6502;
class Suzy;
class Audio;
class Bus;
class LcdScreen;
class StateSerializer;
class TraceLogger;
struct GLYNX_Trace_Entry;
enum GLYNX_Trace_Type : u8;

class Mikey
{
public:
    struct Mikey_State
    {
        GLYNX_Mikey_Timer timers[8];
        GLYNX_Mikey_Color colors[16];
        GLYNX_Mikey_Audio audio[4];
        GLYNX_Uart uart;
        u8 ATTEN_A;
        u8 ATTEN_B;
        u8 ATTEN_C;
        u8 ATTEN_D;
        u8 MPAN;
        u8 MSTEREO;
        u8 SYSCTL1;
        u8 IODIR;
        u8 IODAT;
        u8 SERCTL;
        u8 SERDAT;
        u8 SDONEACK;
        u8 CPUSLEEP;
        u8 DISPCTL;
        u8 PBKUP;
        u8 MTEST0;
        u16_union DISPADR;
        u8 irq_pending;
        u8 irq_mask;
        bool suzy_done_pending;
        bool frame_ready;
        u16 dispadr_latch;
        bool rest;
        u32 refresh_cycle_counter;
        u32 timer_source_phase;
        char debug_msg_buffer[GLYNX_DEBUG_MSG_MAX_SIZE];
        int debug_msg_pos;
        u16_union debug_str_addr;
    };

public:
    Mikey(Suzy* suzy, Media* media, M6502* m6502, Bus* bus, Random* random);
    ~Mikey();
    void Init(Memory* memory, GLYNX_Pixel_Format pixel_format);
    void SetAudio(Audio* audio);
    void Reset(bool is_lynx2);
    bool Clock(u32 cycles);
    template<bool debug = false> u8 Read(u16 address);
    template<bool debug = false> void Write(u16 address, u8 value);
    bool IsPoweredOn();
    Mikey_State* GetState();
    LcdScreen* GetLcdScreen();
    bool SwitchAudInValue();
    void SetSuzyDone();
    void SetTraceLogger(TraceLogger* trace_logger);
    void SetDebugOutputEnabled(bool enabled);
    bool IsDebugOutputEnabled();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream, int version);
    void SetComLynxCallbacks(GLYNX_ComLynx_Publish_Callback publish_callback,
        GLYNX_ComLynx_Sample_Callback sample_callback, GLYNX_ComLynx_Break_Callback break_callback,
        GLYNX_ComLynx_Sync_Callback sync_callback, void* user_data);
    void SetComLynxTurboCallbacks(GLYNX_ComLynx_Turbo_Sample_Callback sample_callback,
        GLYNX_ComLynx_Turbo_Sync_Callback sync_callback, void* user_data);
    void SetComLynxCableConnected(bool connected);
    bool IsComLynxCableConnected() const;
    bool IsUartTurbo() const;
    u32 GetUartBitCycles() const;
    u32 GetComLynxSyncCycles() const;
    u32 GetComLynxPromiseCycles() const;
    u64 GetComLynxCycle() const;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    void ResetTraceUARTEventPairing();
#endif

private:
    void ResetTimers();
    void ResetAudio();
    void ResetUART();
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    void ResetTraceDiagnostics(bool log_reset);
#endif
    void ResetPalette();
    u32 GetTimerAccessCycles(int timer);
    u8 ReadColor(u16 address);
    void WriteColor(u16 address, u8 value, bool debug = false);
    template<bool debug = false> u8 ReadTimer(u16 address);
    template<bool debug = false> void WriteTimer(u16 address, u8 value);
    template<bool debug = false> u8 ReadAudio(u16 address);
    template<bool debug = false> void WriteAudio(u16 address, u8 value);
    u8 ReadAudioExtra(u16 address);
    void WriteAudioExtra(u16 address, u8 value, bool debug = false);
    void Advance(u32 cycles);
    void SynchronizeCPURead();
    void UpdateUART(u32 cycles);
    void UpdateTimerHardware(u32 cycles);
    void ClockTimerDomain(int prescaler, u32 remaining_cycles);
    void RebuildTimerCaches();
    void UpdateTimerServiceMask(int unit);
    void UpdateTimerStatusMask(int unit);
    void ExpireTimerStatus(u32 phase, u32 cycles);
    void ExpireTimerStatusSlot(int slot);
    u32 CalculateNextTimerSourceCycles(u32 phase);
    u32 GetNextTimerServiceCycles(u32 phase);
    void ClockTimer(int timer);
    void ClockAudio(int channel);
    void ServiceTimer(int timer);
    void ServiceAudio(int channel);
    bool BorrowInTimer(int i, GLYNX_Mikey_Timer* t);
    bool BorrowInChannel(int i, GLYNX_Mikey_Audio* c);
    void AdvanceLFSR(u8 channel);
    void RebuildTapsMask(GLYNX_Mikey_Audio* channel);
    void RebuildLFSR(GLYNX_Mikey_Audio* channel);
    void CalculateCutoff(u8 channel);
    void UpdateIRQs();
    void UartRelevelIRQ();
    void UartRxReflectHead();
    void UartRxPush(u8 data, bool parbit, bool parerr, bool framerr, bool rxbreak, u8 source);
    INLINE void TraceRedEyeEvent(u8 dir, u8 data);
    INLINE void TraceRedEyeProblemEvent(u8 dir, u8 problem, u8 value, u8 expected = 0, u8 actual = 0);
    INLINE void TraceRedEyeTimeoutEvent();
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    void UartBeginFrame(u8 data, bool chained, bool trace = true);
#else
    void UartBeginFrame(u8 data, bool chained);
#endif
    bool UartWireLevel() const;
    void UartReceiveWire(bool level, bool peer_low);
    template<bool turbo> void UartClock();
    void HorizontalBlank();
    void UpdateVideo(u32 cycles);
    void Serialize(StateSerializer& s, int version);
    INLINE void TraceTimerEvent(u8 event, int timer, u8 reg = 0, u8 raw = 0);
    INLINE void TraceInterruptEvent(u8 event, u8 reg = 0, u8 raw = 0);
    INLINE void TraceDisplayEvent(u8 event, u8 reg = 0, u8 raw = 0, int line = -1);
    INLINE void TracePaletteEvent(u8 index, u8 raw, u16 rgb444);
    INLINE void TraceAudioEvent(u8 event, int channel, u8 reg, u8 raw);
    INLINE void TraceUARTEvent(u8 event, u8 data = 0, u8 flags = 0, u8 source = 0,
        u8 lost = 0, bool chained = false);
    INLINE void TraceUARTConfigEvent(u8 value, bool register_write = false);
    INLINE void TraceCartridgeAddressEvent();
    INLINE void TraceCartridgeIOEvent(u8 event, u8 operation, u8 value);
    INLINE void TraceDebugMessageEvent(u16 address, u8 value);
    void LogTimerEvent(u8 event, int timer, u8 reg, u8 raw);
    void LogInterruptEvent(u8 event, u8 reg, u8 raw);
    void LogDisplayEvent(u8 event, u8 reg, u8 raw, int line);
    void LogPaletteEvent(u8 index, u8 raw, u16 rgb444);
    void LogAudioEvent(u8 event, int channel, u8 reg, u8 raw);
    void LogUARTEvent(u8 event, u8 data, u8 flags, u8 source,
        u8 lost, bool chained);
    void LogUARTConfigEvent(u8 value, bool register_write);
    void LogRedEyeEvent(u8 dir, u8 data);
    void LogRedEyeProblemEvent(u8 dir, u8 problem, u8 value, u8 expected, u8 actual);
    void LogRedEyeTimeoutEvent();
    void SnapshotRedEyeEntry(GLYNX_Trace_Entry& entry, u8 dir);
    void ResetRedEyeStream(u8 dir);
    void LogCartridgeAddressEvent();
    void LogCartridgeIOEvent(u8 event, u8 operation, u8 value);
    void LogDebugMessageEvent(u16 address, u8 value);

private:
    Media* m_media;
    Memory* m_memory;
    Suzy* m_suzy;
    M6502* m_m6502;
    Audio* m_audio;
    Bus* m_bus;
    Random* m_random;
    LcdScreen* m_lcd_screen;
    Mikey_State m_state;
    bool m_is_lynx2;
    bool m_debug_output_enabled;
    TraceLogger* m_trace_logger;
    u32 m_cpu_read_cycles;
    GLYNX_ComLynx_Publish_Callback m_comlynx_publish_callback;
    GLYNX_ComLynx_Sample_Callback m_comlynx_sample_callback;
    GLYNX_ComLynx_Break_Callback m_comlynx_break_callback;
    GLYNX_ComLynx_Sync_Callback m_comlynx_sync_callback;
    void* m_comlynx_user_data;
    bool m_comlynx_cable_connected;
    u64 m_comlynx_cycle;
    u64 m_uart_last_bit_cycle;
    u64 m_uart_tx_wire_start;
    u32 m_uart_tx_wire_bit_cycles;
    u16 m_uart_tx_wire_bits;
    bool m_uart_tx_wire_published;
    u8 m_uart_rx_wire_state;
    u8 m_uart_rx_wire_bit;
    u8 m_uart_rx_wire_data;
    bool m_uart_rx_wire_parity;
    bool m_uart_rx_wire_link;
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    bool m_uart_tx_trace_active;
    bool m_uart_tx_hold_trace;
    u8 m_uart_trace_cfg;
    u8 m_uart_trace_backup;
    u8 m_uart_trace_control;
    u8 m_uart_trace_turbo;
    u8 m_trace_effective_irqs;
    bool m_trace_uart_irq;
#endif
    u32 m_video_line_remainder;
    u16 m_timer_source_masks[7];
    u16 m_timer_service_mask;
    u16 m_timer_status_mask;
    u8 m_timer_active_source_mask;
    u32 m_timer_source_countdown;

#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    struct RedEyeStream
    {
        u8 buffer[131];
        u8 count;
        u8 total;
        u64 last_cycle;
    };

    RedEyeStream m_redeye[2];
#endif
    GLYNX_ComLynx_Turbo_Sample_Callback m_comlynx_turbo_sample_callback;
    GLYNX_ComLynx_Turbo_Sync_Callback m_comlynx_turbo_sync_callback;
    void* m_comlynx_turbo_user_data;
};

static const u32 k_mikey_timer_period_us[8] = { 1, 2, 4, 8, 16, 32, 64, 0 };
static const u32 k_mikey_timer_period_cycles[8] = { 16, 32, 64, 128, 256, 512, 1024, 0 };
static const u32 k_mikey_timer_source_phase[7] = { 1, 17, 56, 120, 120, 376, 897 };
static const int k_mikey_timer_forward_links[8] = { 2, 3, 4, 5, -1, 7, -1, 8 };
static const int k_mikey_timer_backward_links[8] = { -1, 11, 0, 1, 2, 3, -1, 5 };
static const int k_mikey_audio_forward_links[4] = { 1, 2, 3, -1 };
static const int k_mikey_audio_backward_links[4] = { -1, 0, 1, 2 };

static const u32 k_mikey_refresh_period_cycles = 256;
static const u32 k_mikey_refresh_inject_cycles = 4;

#include "mikey_inline.h"

#endif /* MIKEY_H */
