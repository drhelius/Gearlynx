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

#define GUI_DEBUG_TRACE_LOGGER_IMPORT
#include "gui_debug_trace_logger.h"

#include "imgui.h"
#include "gui.h"
#include "gui_filedialogs.h"
#include "gui_debug_constants.h"
#include "gui_debug_text.h"
#include "config.h"
#include "emu.h"
#include "gui_debug.h"

static bool trace_logger_enabled = false;
static u64 trace_logger_start_total = 0;

static void trace_logger_menu(void);
static void trace_logger_sync_flags(void);
static void format_entry_text(const GLYNX_Trace_Entry& entry, char* buf, int buf_size);
static void format_cpu_entry(const GLYNX_Trace_Entry& entry, char* buf, int buf_size);
static void render_entry_colored(const GLYNX_Trace_Entry& entry, u32 index);

void gui_debug_window_trace_logger(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(340, 168), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(544, 362), ImGuiCond_FirstUseEver);

    ImGui::Begin("Trace Logger", &config_debug.show_trace_logger, ImGuiWindowFlags_MenuBar);

    trace_logger_menu();

    TraceLogger* tl = emu_get_core()->GetTraceLogger();

    if (ImGui::Button(trace_logger_enabled ? "Stop" : "Start"))
    {
        trace_logger_enabled = !trace_logger_enabled;
        if (trace_logger_enabled)
        {
            trace_logger_start_total = tl->GetTotalLogged();
            trace_logger_sync_flags();
        }
        else
            tl->SetEnabledFlags(0);
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear"))
    {
        gui_debug_trace_logger_clear();
    }

    ImGui::SameLine();
    ImGui::Text("Entries: %u / %d", tl->GetCount(), TRACE_BUFFER_SIZE);

    if (trace_logger_enabled)
        trace_logger_sync_flags();

    if (ImGui::BeginChild("##logger", ImVec2(ImGui::GetContentRegionAvail().x, 0), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::PushFont(gui_default_font);

        u32 count = tl->GetCount();

        ImGuiListClipper clipper;
        clipper.Begin((int)count, ImGui::GetTextLineHeightWithSpacing());

        while (clipper.Step())
        {
            for (int item = clipper.DisplayStart; item < clipper.DisplayEnd; item++)
            {
                const GLYNX_Trace_Entry& entry = tl->GetEntry((u32)item);
                u64 entry_number = tl->GetTotalLogged() - (u64)count + (u64)item - trace_logger_start_total;
                render_entry_colored(entry, (u32)entry_number);
            }
        }

        ImGui::PopFont();
    }

    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_trace_logger_clear(void)
{
    emu_get_core()->GetTraceLogger()->Reset();
    trace_logger_start_total = 0;
}

void gui_debug_save_log(const char* file_path)
{
    FILE* file = fopen_utf8(file_path, "w");

    if (file != NULL)
    {
        TraceLogger* tl = emu_get_core()->GetTraceLogger();
        u32 count = tl->GetCount();
        char buf[256];

        for (u32 i = 0; i < count; i++)
        {
            const GLYNX_Trace_Entry& entry = tl->GetEntry(i);
            format_entry_text(entry, buf, sizeof(buf));
            if (config_debug.trace_counter)
                fprintf(file, "%06u %s\n", i, buf);
            else
                fprintf(file, "%s\n", buf);
        }

        fclose(file);
    }
}

static void trace_logger_menu(void)
{
    ImGui::BeginMenuBar();

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Log As..."))
        {
            gui_file_dialog_save_log();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("CPU"))
    {
        ImGui::MenuItem("Instruction Counter", "", &config_debug.trace_counter);
        ImGui::MenuItem("Registers", "", &config_debug.trace_registers);
        ImGui::MenuItem("Flags", "", &config_debug.trace_flags);
        ImGui::MenuItem("Bytes", "", &config_debug.trace_bytes);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Filter"))
    {
        ImGui::MenuItem("IRQs", "", &config_debug.trace_cpu_irq);
        ImGui::MenuItem("Suzy Math", "", &config_debug.trace_suzy_math);
        ImGui::MenuItem("Suzy Sprites", "", &config_debug.trace_suzy_sprites);
        ImGui::MenuItem("Suzy Input", "", &config_debug.trace_suzy_input);
        ImGui::MenuItem("Mikey Timers", "", &config_debug.trace_mikey_timers);
        ImGui::MenuItem("Mikey UART", "", &config_debug.trace_mikey_uart);
        ImGui::MenuItem("Mikey Audio", "", &config_debug.trace_mikey_audio);
        ImGui::MenuItem("Cart", "", &config_debug.trace_cart);

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

static void trace_logger_sync_flags(void)
{
    u32 flags = TRACE_FLAG_CPU;
    if (config_debug.trace_cpu_irq)       flags |= TRACE_FLAG_CPU_IRQ;
    if (config_debug.trace_suzy_math)    flags |= TRACE_FLAG_SUZY_MATH;
    if (config_debug.trace_suzy_sprites) flags |= TRACE_FLAG_SUZY_SPRITE;
    if (config_debug.trace_suzy_input)   flags |= TRACE_FLAG_SUZY_INPUT;
    if (config_debug.trace_mikey_timers) flags |= TRACE_FLAG_MIKEY_TIMER;
    if (config_debug.trace_mikey_uart)   flags |= TRACE_FLAG_MIKEY_UART;
    if (config_debug.trace_mikey_audio)  flags |= TRACE_FLAG_MIKEY_AUDIO;
    if (config_debug.trace_cart)         flags |= TRACE_FLAG_CART_SHIFT;
    emu_get_core()->GetTraceLogger()->SetEnabledFlags(flags);
}

static void format_cpu_entry(const GLYNX_Trace_Entry& entry, char* buf, int buf_size)
{
    Memory* memory = emu_get_core()->GetMemory();
    GLYNX_Disassembler_Record* record = memory->GetDisassemblerRecord(entry.cpu.pc);

    char instr[64] = "???";
    char bytes[10] = "";
    if (IsValidPointer(record))
    {
        snprintf(instr, sizeof(instr), "%s", record->name);

        char* p = instr;
        while (*p)
        {
            if (*p == '{')
            {
                char* end = strchr(p, '}');
                if (end)
                    memmove(p, end + 1, strlen(end + 1) + 1);
                else
                    break;
            }
            else
                p++;
        }
        snprintf(bytes, sizeof(bytes), "%s", record->bytes);
    }

    char registers[40] = "";
    if (config_debug.trace_registers)
        snprintf(registers, sizeof(registers), "A:%02X X:%02X Y:%02X S:%02X  ",
                 entry.cpu.a, entry.cpu.x, entry.cpu.y, entry.cpu.s);

    char flags[20] = "";
    if (config_debug.trace_flags)
    {
        u8 p = entry.cpu.p;
        snprintf(flags, sizeof(flags), "%c%c-%c%c%c%c%c  ",
                 (p & FLAG_NEGATIVE) ? 'N' : 'n',
                 (p & FLAG_OVERFLOW) ? 'V' : 'v',
                 (p & FLAG_BREAK) ? 'B' : 'b',
                 (p & FLAG_DECIMAL) ? 'D' : 'd',
                 (p & FLAG_INTERRUPT) ? 'I' : 'i',
                 (p & FLAG_ZERO) ? 'Z' : 'z',
                 (p & FLAG_CARRY) ? 'C' : 'c');
    }

    snprintf(buf, buf_size, "%04X  %s%s%-24s %s",
             entry.cpu.pc,
             registers, flags, instr,
             config_debug.trace_bytes ? bytes : "");
}

static void format_entry_text(const GLYNX_Trace_Entry& entry, char* buf, int buf_size)
{
    switch (entry.type)
    {
        case TRACE_CPU:
            format_cpu_entry(entry, buf, buf_size);
            break;
        case TRACE_CPU_IRQ:
            snprintf(buf, buf_size, "  [CPU]  IRQ       PC:$%04X  Vector:$%04X  Mask:%02X",
                     entry.irq.pc, entry.irq.vector, entry.irq.irq_mask);
            break;
        case TRACE_SUZY_MATH:
            if (entry.math.completed)
                snprintf(buf, buf_size, "  [SUZY] MATH      DONE");
            else if (entry.math.is_divide)
                snprintf(buf, buf_size, "  [SUZY] DIVIDE    $%04X%04X / $%04X = $%08X  R:$%04X%s",
                         entry.math.op_a, entry.math.op_b & 0xFFFF, entry.math.op_b,
                         entry.math.result, entry.math.remainder,
                         entry.math.div_by_zero ? " [DIV0]" : "");
            else
                snprintf(buf, buf_size, "  [SUZY] MULTIPLY  $%04X * $%04X = $%08X%s%s",
                         entry.math.op_a, entry.math.op_b, entry.math.result,
                         entry.math.is_signed ? " [SIGN]" : "",
                         entry.math.accumulate ? " [ACC]" : "");
            break;
        case TRACE_SUZY_SPRITE:
            if (entry.sprite.is_start)
                snprintf(buf, buf_size, "  [SUZY] SPRITES   START  SCB:$%04X", entry.sprite.scb_addr);
            else if (entry.sprite.is_end)
                snprintf(buf, buf_size, "  [SUZY] SPRITES   END  Cycles:%u", entry.sprite.total_cycles);
            else if (entry.sprite.skipped)
                snprintf(buf, buf_size, "  [SUZY]  SPRITE   SCB:$%04X  [SKIP]", entry.sprite.scb_addr);
            else
            {
                static const char* k_types[] = {"BG","BGNC","BSHD","BNDY","NORM","NCOL","XOR","SHDW"};
                snprintf(buf, buf_size, "  [SUZY]  SPRITE   SCB:$%04X  Next:$%04X  (%d,%d)  %dBPP %s",
                         entry.sprite.scb_addr, entry.sprite.scb_next,
                         entry.sprite.hpos, entry.sprite.vpos,
                         entry.sprite.bpp, k_types[entry.sprite.type & 7]);
            }
            break;
        case TRACE_SUZY_INPUT:
            snprintf(buf, buf_size, "  [SUZY]  INPUT    %s:$%02X",
                     entry.input.is_joystick ? "JOYSTICK" : "SWITCHES", entry.input.value);
            break;
        case TRACE_MIKEY_TIMER:
            snprintf(buf, buf_size, "  [MIKEY] TIMER %d  IRQ  Backup:$%02X",
                     entry.timer.timer_id, entry.timer.backup);
            break;
        case TRACE_MIKEY_UART:
            snprintf(buf, buf_size, "  [MIKEY] UART %s  Data:$%02X%s",
                     entry.uart.is_tx ? "TX" : "RX", entry.uart.data,
                     (!entry.uart.is_tx && entry.uart.flags) ? "  [ERR]" : "");
            break;
        case TRACE_MIKEY_AUDIO:
        {
            static const char* k_audio_regs[] = {"VOL","FDBK","OUT","LFSR","BKUP","CTL","CNT","MISC"};
            snprintf(buf, buf_size, "  [MIKEY] AUDIO %d  %s=$%02X",
                     entry.audio.channel, k_audio_regs[entry.audio.reg & 7], entry.audio.value);
            break;
        }
        case TRACE_CART_SHIFT:
            snprintf(buf, buf_size, "  [CART]  SHIFT    Addr:$%02X  Bit:%d",
                     entry.cart.addr_shift, entry.cart.bit);
            break;
        default:
            snprintf(buf, buf_size, "  [???]");
            break;
    }
}

static void render_cpu_entry_colored(const GLYNX_Trace_Entry& entry)
{
    Memory* memory = emu_get_core()->GetMemory();
    GLYNX_Disassembler_Record* record = memory->GetDisassemblerRecord(entry.cpu.pc);

    ImGui::TextColored(cyan, "%04X", entry.cpu.pc);

    if (config_debug.trace_registers)
    {
        ImGui::SameLine(0, 0);
        ImGui::TextColored(violet, "  A:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.a);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(violet, " X:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.x);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(violet, " Y:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.y);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(violet, " S:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.s);
    }

    if (config_debug.trace_flags)
    {
        u8 p = entry.cpu.p;
        ImGui::SameLine(0, 0);
        ImGui::TextColored(yellow, "  %c%c-%c%c%c%c%c",
                 (p & FLAG_NEGATIVE) ? 'N' : 'n',
                 (p & FLAG_OVERFLOW) ? 'V' : 'v',
                 (p & FLAG_BREAK) ? 'B' : 'b',
                 (p & FLAG_DECIMAL) ? 'D' : 'd',
                 (p & FLAG_INTERRUPT) ? 'I' : 'i',
                 (p & FLAG_ZERO) ? 'Z' : 'z',
                 (p & FLAG_CARRY) ? 'C' : 'c');
    }

    if (IsValidPointer(record))
    {
        std::string instr = record->name;
        size_t pos;
        pos = instr.find("{n}");
        if (pos != std::string::npos)
            instr.replace(pos, 3, c_white);
        pos = instr.find("{o}");
        if (pos != std::string::npos)
            instr.replace(pos, 3, c_brown);
        pos = instr.find("{e}");
        if (pos != std::string::npos)
            instr.replace(pos, 3, c_blue);

        ImGui::SameLine(0, 0);
        TextColoredEx("  %s%s", c_white, instr.c_str());

        if (config_debug.trace_bytes)
        {
            float char_width = ImGui::CalcTextSize("A").x;
            float bytes_column = char_width * 28;
            if (config_debug.trace_registers) bytes_column += char_width * 24;
            if (config_debug.trace_flags)     bytes_column += char_width * 9;
            if (config_debug.trace_counter)   bytes_column += char_width * 7;
            ImGui::SameLine(bytes_column);
            ImGui::TextColored(gray, "%s", record->bytes);
        }
    }
    else
    {
        ImGui::SameLine(0, 0);
        ImGui::TextColored(gray, "  ???");
    }
}

static void render_entry_colored(const GLYNX_Trace_Entry& entry, u32 index)
{
    char buf[256];

    if (config_debug.trace_counter)
    {
        ImGui::TextColored(gray, "%06u ", index);
        ImGui::SameLine(0, 0);
    }

    switch (entry.type)
    {
        case TRACE_CPU:
            render_cpu_entry_colored(entry);
            break;
        case TRACE_CPU_IRQ:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(red, "%s", buf);
            break;
        case TRACE_SUZY_MATH:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(cyan, "%s", buf);
            break;
        case TRACE_SUZY_SPRITE:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(green, "%s", buf);
            break;
        case TRACE_SUZY_INPUT:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(yellow, "%s", buf);
            break;
        case TRACE_MIKEY_TIMER:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(orange, "%s", buf);
            break;
        case TRACE_MIKEY_UART:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(violet, "%s", buf);
            break;
        case TRACE_MIKEY_AUDIO:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(blue, "%s", buf);
            break;
        case TRACE_CART_SHIFT:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(magenta, "%s", buf);
            break;
        default:
            break;
    }
}
