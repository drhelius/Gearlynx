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
#include "utils.h"

#define TRACE_DISK_BUFFER_SIZE (1024 * 1024)
#define TRACE_DISK_STAGING_CAPACITY 100000

static bool trace_logger_enabled = false;
static bool trace_logger_follow_latest = true;
static bool trace_logger_scroll_to_bottom = false;
static bool trace_logger_wait_for_scroll_away = false;
static u64 trace_logger_start_total = 0;
static bool trace_logger_choose_output_path = false;
static FILE* trace_logger_disk_file = NULL;
static char trace_logger_disk_path[4096] = {};
static char trace_logger_disk_directory[4096] = {};
static char trace_logger_disk_buffer[TRACE_DISK_BUFFER_SIZE];
static size_t trace_logger_disk_buffer_used = 0;
static u64 trace_logger_disk_entries = 0;
static u64 trace_logger_disk_flushed_total = 0;
static u64 trace_logger_disk_bytes = 0;
static bool trace_logger_disk_limit_reached = false;
static bool trace_logger_disk_overflow = false;
static Uint64 trace_logger_disk_last_flush = 0;

static const u32 trace_logger_capacities[] = {100000, 500000, 1000000, 2000000, 5000000};
static const char* trace_logger_capacity_names[] = {"100K", "500K", "1M", "2M", "5M"};
static const u64 trace_logger_disk_sizes[] = {
    10ULL * 1024ULL * 1024ULL,
    50ULL * 1024ULL * 1024ULL,
    100ULL * 1024ULL * 1024ULL,
    250ULL * 1024ULL * 1024ULL,
    500ULL * 1024ULL * 1024ULL,
    1024ULL * 1024ULL * 1024ULL,
    0
};

static void trace_logger_menu(void);
static void trace_logger_sync_flags(void);
static u32 trace_logger_get_config_flags(void);
static void trace_logger_set_config_flags(u32 flags);
static bool trace_logger_apply_capacity(void);
static bool trace_logger_start_disk(void);
static void trace_logger_stop_disk(bool show_status);
static bool trace_logger_flush_disk_buffer(bool flush_file);
static bool trace_logger_flush_disk_entries(void);
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
        if (trace_logger_enabled)
        {
            gui_debug_trace_logger_stop();
        }
        else
        {
            gui_debug_trace_logger_start(trace_logger_get_config_flags());
        }
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(trace_logger_enabled && config_debug.trace_output == gui_TraceOutput_Disk);
    if (ImGui::Button("Clear"))
    {
        gui_debug_trace_logger_clear();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(trace_logger_enabled);
    ImGui::SetNextItemWidth(90.0f);
    int previous_output = config_debug.trace_output;
    if (ImGui::Combo("##trace_output", &config_debug.trace_output, "Memory\0Disk\0\0"))
    {
        if (!trace_logger_apply_capacity())
            config_debug.trace_output = previous_output;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(trace_logger_enabled);
    ImGui::SetNextItemWidth(145.0f);
    if (config_debug.trace_output == gui_TraceOutput_Memory)
    {
        int previous_capacity = config_debug.trace_capacity;
        char capacity_labels[IM_ARRAYSIZE(trace_logger_capacities)][32];
        const char* capacity_items[IM_ARRAYSIZE(trace_logger_capacities)];
        for (int i = 0; i < IM_ARRAYSIZE(trace_logger_capacities); i++)
        {
            double memory_mib = ((double)trace_logger_capacities[i] * sizeof(GLYNX_Trace_Entry)) / (1024.0 * 1024.0);
            snprintf(capacity_labels[i], sizeof(capacity_labels[i]), "%s (%.1f MiB)", trace_logger_capacity_names[i], memory_mib);
            capacity_items[i] = capacity_labels[i];
        }
        if (ImGui::Combo("##trace_capacity", &config_debug.trace_capacity, capacity_items, IM_ARRAYSIZE(capacity_items)) && !trace_logger_apply_capacity())
            config_debug.trace_capacity = previous_capacity;
    }
    else
    {
        ImGui::Combo("##trace_disk_size", &config_debug.trace_disk_size, "10 MB\0" "50 MB\0" "100 MB\0" "250 MB\0" "500 MB\0" "1 GB\0" "Unbounded\0\0");
    }
    ImGui::EndDisabled();
    if (config_debug.trace_output == gui_TraceOutput_Memory && ImGui::IsItemHovered())
    {
        double memory_mib = ((double)trace_logger_capacities[config_debug.trace_capacity] * sizeof(GLYNX_Trace_Entry)) / (1024.0 * 1024.0);
        ImGui::SetTooltip("Preallocated memory: %.1f MiB (%u bytes per entry).", memory_mib, (u32)sizeof(GLYNX_Trace_Entry));
    }

    if (config_debug.trace_output == gui_TraceOutput_Memory)
    {
        ImGui::SameLine();
        ImGui::Text("Entries: %u / %u", tl->GetCount(), tl->GetCapacity());
    }
    if (config_debug.trace_output == gui_TraceOutput_Disk && trace_logger_disk_path[0] != '\0')
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##trace_disk_file", trace_logger_disk_path, sizeof(trace_logger_disk_path), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
    }

    if (trace_logger_enabled)
        trace_logger_sync_flags();

    u32 count = tl->GetCount();
    ImGui::PushFont(gui_default_font);
    float line_height = ImGui::GetTextLineHeightWithSpacing();
    float content_height = (float)count * line_height;
    ImGui::SetNextWindowContentSize(ImVec2(0.0f, content_height));
    if ((trace_logger_enabled && trace_logger_follow_latest) || trace_logger_scroll_to_bottom)
        ImGui::SetNextWindowScroll(ImVec2(-1.0f, content_height));

    if (ImGui::BeginChild("##logger", ImVec2(ImGui::GetContentRegionAvail().x, 0), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        float scroll_y = ImGui::GetScrollY();
        float scroll_max_y = ImGui::GetScrollMaxY();
        bool at_bottom = scroll_y >= scroll_max_y - 0.5f;
        bool user_scrolling = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            (ImGui::GetIO().MouseWheel != 0.0f || ImGui::IsMouseDragging(ImGuiMouseButton_Left));
        if (trace_logger_enabled)
        {
            if (trace_logger_scroll_to_bottom)
            {
                trace_logger_follow_latest = true;
                trace_logger_wait_for_scroll_away = false;
            }
            else if (trace_logger_follow_latest && user_scrolling)
            {
                trace_logger_follow_latest = false;
                trace_logger_wait_for_scroll_away = true;
            }
            else if (!trace_logger_follow_latest)
            {
                if (trace_logger_wait_for_scroll_away)
                {
                    if (!at_bottom)
                        trace_logger_wait_for_scroll_away = false;
                }
                else if (at_bottom)
                    trace_logger_follow_latest = true;
            }
        }

        ImGuiListClipper clipper;
        clipper.Begin((int)count, line_height);

        while (clipper.Step())
        {
            for (int item = clipper.DisplayStart; item < clipper.DisplayEnd; item++)
            {
                const GLYNX_Trace_Entry& entry = tl->GetEntry((u32)item);
                u64 entry_number = tl->GetTotalLogged() - (u64)count + (u64)item - trace_logger_start_total;
                render_entry_colored(entry, (u32)entry_number);
            }
        }

        trace_logger_scroll_to_bottom = false;
    }

    ImGui::EndChild();
    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();

    if (trace_logger_choose_output_path)
    {
        trace_logger_choose_output_path = false;
        gui_file_dialog_choose_trace_path();
    }
}

void gui_debug_trace_logger_init(void)
{
    strncpy_fit(trace_logger_disk_directory, config_debug.trace_disk_path.c_str(), sizeof(trace_logger_disk_directory));
    if (!trace_logger_apply_capacity())
    {
        config_debug.trace_capacity = 0;
        trace_logger_apply_capacity();
    }
}

void gui_debug_trace_logger_update(void)
{
    if (trace_logger_enabled && config_debug.trace_output == gui_TraceOutput_Disk)
    {
        if (!trace_logger_flush_disk_entries())
        {
            trace_logger_stop_disk(false);
            if (trace_logger_disk_overflow)
                gui_set_error_message("Trace disk staging buffer overflow.");
            else
                gui_set_error_message("Error writing trace log to disk.");
        }
        else if (trace_logger_disk_limit_reached)
        {
            trace_logger_stop_disk(false);
            gui_set_status_message("Trace recording stopped: maximum file size reached", 4000);
        }
        else
        {
            Uint64 now = SDL_GetTicks();
            if ((now - trace_logger_disk_last_flush) >= 1000)
            {
                if (!trace_logger_flush_disk_buffer(true))
                {
                    trace_logger_stop_disk(false);
                    gui_set_error_message("Error flushing trace log to disk.");
                }
                else
                    trace_logger_disk_last_flush = now;
            }
        }
    }
}

void gui_debug_trace_logger_shutdown(void)
{
    if (trace_logger_disk_file)
        trace_logger_stop_disk(false);
}

void gui_debug_trace_logger_clear(void)
{
    TraceLogger* tl = emu_get_core()->GetTraceLogger();
    if (trace_logger_enabled && config_debug.trace_output == gui_TraceOutput_Disk)
    {
        if (!trace_logger_flush_disk_entries())
            gui_set_error_message("Error writing trace log to disk.");
        tl->Reset();
        trace_logger_disk_flushed_total = 0;
    }
    else
        tl->Reset();
    trace_logger_start_total = 0;
}

void gui_debug_trace_logger_set_output_directory(const char* path)
{
    strncpy_fit(trace_logger_disk_directory, path, sizeof(trace_logger_disk_directory));
    config_debug.trace_disk_path.assign(path);
}

bool gui_debug_trace_logger_configure(int output, int memory_size, int disk_size, const char* output_path)
{
    if (trace_logger_enabled)
        return false;
    if (output < gui_TraceOutput_Memory || output > gui_TraceOutput_Disk)
        return false;
    if (memory_size < 0 || memory_size >= IM_ARRAYSIZE(trace_logger_capacities))
        return false;
    if (disk_size < 0 || disk_size >= IM_ARRAYSIZE(trace_logger_disk_sizes))
        return false;

    int previous_output = config_debug.trace_output;
    int previous_memory_size = config_debug.trace_capacity;
    int previous_disk_size = config_debug.trace_disk_size;
    int previous_dir_option = config_debug.trace_disk_dir_option;
    std::string previous_path = config_debug.trace_disk_path;

    config_debug.trace_output = output;
    config_debug.trace_capacity = memory_size;
    config_debug.trace_disk_size = disk_size;
    if (output == gui_TraceOutput_Disk && output_path && output_path[0] != '\0')
    {
        config_debug.trace_disk_dir_option = Directory_Location_Custom;
        gui_debug_trace_logger_set_output_directory(output_path);
    }

    if (!trace_logger_apply_capacity())
    {
        config_debug.trace_output = previous_output;
        config_debug.trace_capacity = previous_memory_size;
        config_debug.trace_disk_size = previous_disk_size;
        config_debug.trace_disk_dir_option = previous_dir_option;
        config_debug.trace_disk_path = previous_path;
        strncpy_fit(trace_logger_disk_directory, previous_path.c_str(), sizeof(trace_logger_disk_directory));
        return false;
    }

    return true;
}

bool gui_debug_trace_logger_start(u32 flags)
{
    if (flags == 0)
        flags = TRACE_FLAG_CPU;
    trace_logger_set_config_flags(flags);

    if (trace_logger_enabled)
    {
        emu_get_core()->GetTraceLogger()->SetEnabledFlags(flags);
        return true;
    }

    if (config_debug.trace_output == gui_TraceOutput_Disk && !trace_logger_start_disk())
        return false;

    trace_logger_enabled = true;
    trace_logger_follow_latest = true;
    trace_logger_scroll_to_bottom = true;
    trace_logger_wait_for_scroll_away = false;
    trace_logger_start_total = emu_get_core()->GetTraceLogger()->GetTotalLogged();
    emu_get_core()->GetTraceLogger()->SetEnabledFlags(flags);
    return true;
}

void gui_debug_trace_logger_stop(void)
{
    if (!trace_logger_enabled)
        return;

    trace_logger_scroll_to_bottom = trace_logger_follow_latest;

    if (config_debug.trace_output == gui_TraceOutput_Disk)
        trace_logger_stop_disk(true);
    else
    {
        trace_logger_enabled = false;
        emu_get_core()->GetTraceLogger()->SetEnabledFlags(0);
    }
}

bool gui_debug_trace_logger_is_enabled(void)
{
    return trace_logger_enabled;
}

const char* gui_debug_trace_logger_get_output_path(void)
{
    return trace_logger_disk_path;
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
        if (ImGui::MenuItem("Save Log As...", NULL, false, config_debug.trace_output == gui_TraceOutput_Memory))
        {
            gui_file_dialog_save_log();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Settings"))
    {
        if (ImGui::BeginMenu("CPU"))
        {
            ImGui::MenuItem("Instruction Counter", "", &config_debug.trace_counter);
            ImGui::MenuItem("Registers", "", &config_debug.trace_registers);
            ImGui::MenuItem("Flags", "", &config_debug.trace_flags);
            ImGui::MenuItem("Bytes", "", &config_debug.trace_bytes);

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Debug Output", "", &config_debug.debug_output_enabled))
            emu_set_debug_output(config_debug.debug && config_debug.debug_output_enabled);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Enable debug output registers ($FDC0-$FDC4).");
            ImGui::Text("Games can send text to the Trace Logger.");
            ImGui::EndTooltip();
        }

        if (ImGui::BeginMenu("Disk Output"))
        {
            ImGui::BeginDisabled(trace_logger_enabled);
            ImGui::SetNextItemWidth(180.0f);
            ImGui::Combo("##trace_disk_dir", &config_debug.trace_disk_dir_option, "Default Location\0Same as ROM\0Custom Location\0\0");

            switch ((Directory_Location)config_debug.trace_disk_dir_option)
            {
                default:
                case Directory_Location_Default:
                    ImGui::Text("%s", config_root_path);
                    break;
                case Directory_Location_ROM:
                    if (!emu_is_empty())
                        ImGui::Text("%s", emu_get_core()->GetMedia()->GetFileDirectory());
                    break;
                case Directory_Location_Custom:
                    if (ImGui::MenuItem("Choose..."))
                        trace_logger_choose_output_path = true;
                    ImGui::PushItemWidth(450.0f);
                    if (ImGui::InputText("##trace_disk_path", trace_logger_disk_directory, sizeof(trace_logger_disk_directory), ImGuiInputTextFlags_AutoSelectAll))
                        config_debug.trace_disk_path.assign(trace_logger_disk_directory);
                    ImGui::PopItemWidth();
                    break;
            }
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Filter"))
    {
        ImGui::MenuItem("CPU", "", &config_debug.trace_cpu);
        ImGui::MenuItem("IRQs", "", &config_debug.trace_cpu_irq);
        ImGui::MenuItem("Suzy Math", "", &config_debug.trace_suzy_math);
        ImGui::MenuItem("Suzy Sprites", "", &config_debug.trace_suzy_sprites);
        ImGui::MenuItem("Suzy Input", "", &config_debug.trace_suzy_input);
        ImGui::MenuItem("Mikey Timers", "", &config_debug.trace_mikey_timers);
        ImGui::MenuItem("Mikey UART", "", &config_debug.trace_mikey_uart);
        ImGui::MenuItem("RedEye", "", &config_debug.trace_redeye);
        ImGui::MenuItem("Mikey Audio", "", &config_debug.trace_mikey_audio);
        ImGui::MenuItem("Cart", "", &config_debug.trace_cart);
        ImGui::MenuItem("Debug Messages", "", &config_debug.trace_debug_messages);

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

static bool trace_logger_apply_capacity(void)
{
    TraceLogger* tl = emu_get_core()->GetTraceLogger();
    u32 capacity = TRACE_DISK_STAGING_CAPACITY;
    if (config_debug.trace_output == gui_TraceOutput_Memory)
        capacity = trace_logger_capacities[config_debug.trace_capacity];

    if (!tl->SetCapacity(capacity))
    {
        gui_set_error_message("Unable to allocate the selected trace logger capacity.");
        return false;
    }
    return true;
}

static bool trace_logger_start_disk(void)
{
    if (!trace_logger_apply_capacity())
        return false;

    const char* directory = config_root_path;
    switch ((Directory_Location)config_debug.trace_disk_dir_option)
    {
        case Directory_Location_ROM:
            if (!emu_is_empty())
                directory = emu_get_core()->GetMedia()->GetFileDirectory();
            break;
        case Directory_Location_Custom:
            directory = config_debug.trace_disk_path.c_str();
            break;
        default:
            break;
    }

    time_t now = time(0);
    tm local_time;
    char date_time[32] = {};
    if (get_local_time(now, &local_time))
        strftime(date_time, sizeof(date_time), "%Y-%m-%d %H%M%S", &local_time);

    const char* rom_name = "Gearlynx";
    if (!emu_is_empty())
        rom_name = emu_get_core()->GetMedia()->GetFileName();

    bool path_available = false;
    for (int index = 0; index < 1000; index++)
    {
        char filename[1024];
        if (index == 0)
            snprintf(filename, sizeof(filename), "%s - Trace - %s.txt", rom_name, date_time);
        else
            snprintf(filename, sizeof(filename), "%s - Trace - %s (%d).txt", rom_name, date_time, index + 1);

        if (!join_path(directory, filename, trace_logger_disk_path, sizeof(trace_logger_disk_path)))
        {
            gui_set_error_message("Trace log path is too long.");
            return false;
        }
        if (!path_exists(trace_logger_disk_path))
        {
            path_available = true;
            break;
        }
    }

    if (!path_available)
    {
        gui_set_error_message("Unable to create a unique trace log filename.");
        return false;
    }

    trace_logger_disk_file = fopen_utf8(trace_logger_disk_path, "wb");
    if (!trace_logger_disk_file)
    {
        gui_set_error_message("Unable to create the trace log file.");
        trace_logger_disk_path[0] = '\0';
        return false;
    }

    trace_logger_disk_buffer_used = 0;
    trace_logger_disk_entries = 0;
    trace_logger_disk_flushed_total = 0;
    trace_logger_disk_bytes = 0;
    trace_logger_disk_limit_reached = false;
    trace_logger_disk_overflow = false;
    trace_logger_disk_last_flush = SDL_GetTicks();
    emu_get_core()->GetTraceLogger()->Reset();
    gui_set_status_message("Trace recording started", 3000);
    return true;
}

static void trace_logger_stop_disk(bool show_status)
{
    if (trace_logger_disk_file)
    {
        trace_logger_flush_disk_entries();
        trace_logger_flush_disk_buffer(true);
        fclose(trace_logger_disk_file);
        trace_logger_disk_file = NULL;
    }

    trace_logger_enabled = false;
    emu_get_core()->GetTraceLogger()->SetEnabledFlags(0);
    if (show_status)
        gui_set_status_message("Trace recording stopped", 3000);
}

static bool trace_logger_flush_disk_buffer(bool flush_file)
{
    if (!trace_logger_disk_file)
        return false;

    if (trace_logger_disk_buffer_used > 0)
    {
        size_t written = fwrite(trace_logger_disk_buffer, 1, trace_logger_disk_buffer_used, trace_logger_disk_file);
        if (written != trace_logger_disk_buffer_used)
            return false;
        trace_logger_disk_buffer_used = 0;
    }

    return !flush_file || fflush(trace_logger_disk_file) == 0;
}

static bool trace_logger_flush_disk_entries(void)
{
    if (!trace_logger_disk_file)
        return false;
    if (trace_logger_disk_limit_reached)
        return true;

    TraceLogger* tl = emu_get_core()->GetTraceLogger();
    u32 count = tl->GetCount();
    u64 total = tl->GetTotalLogged();
    u64 oldest = total - (u64)count;
    if (trace_logger_disk_flushed_total < oldest)
    {
        trace_logger_disk_overflow = true;
        return false;
    }
    u32 first = (u32)(trace_logger_disk_flushed_total - oldest);
    char entry_text[256];
    char line[320];

    for (u32 i = first; i < count; i++)
    {
        format_entry_text(tl->GetEntry(i), entry_text, sizeof(entry_text));
        int length;
        if (config_debug.trace_counter)
            length = snprintf(line, sizeof(line), "%06llu %s\n", (unsigned long long)trace_logger_disk_entries, entry_text);
        else
            length = snprintf(line, sizeof(line), "%s\n", entry_text);
        if (length < 0)
            return false;

        size_t line_size = MIN((size_t)length, sizeof(line) - 1);
        u64 max_size = trace_logger_disk_sizes[config_debug.trace_disk_size];
        if (max_size > 0 && trace_logger_disk_bytes + (u64)line_size > max_size)
        {
            trace_logger_disk_limit_reached = true;
            break;
        }
        if (trace_logger_disk_buffer_used + line_size > sizeof(trace_logger_disk_buffer) && !trace_logger_flush_disk_buffer(false))
            return false;
        memcpy(trace_logger_disk_buffer + trace_logger_disk_buffer_used, line, line_size);
        trace_logger_disk_buffer_used += line_size;
        trace_logger_disk_entries++;
        trace_logger_disk_bytes += (u64)line_size;
    }

    trace_logger_disk_flushed_total = total;
    return true;
}

static void trace_logger_sync_flags(void)
{
    emu_get_core()->GetTraceLogger()->SetEnabledFlags(trace_logger_get_config_flags());
}

static u32 trace_logger_get_config_flags(void)
{
    u32 flags = 0;
    if (config_debug.trace_cpu)          flags |= TRACE_FLAG_CPU;
    if (config_debug.trace_cpu_irq)      flags |= TRACE_FLAG_CPU_IRQ;
    if (config_debug.trace_suzy_math)    flags |= TRACE_FLAG_SUZY_MATH;
    if (config_debug.trace_suzy_sprites) flags |= TRACE_FLAG_SUZY_SPRITE;
    if (config_debug.trace_suzy_input)   flags |= TRACE_FLAG_SUZY_INPUT;
    if (config_debug.trace_mikey_timers) flags |= TRACE_FLAG_MIKEY_TIMER;
    if (config_debug.trace_mikey_uart)   flags |= TRACE_FLAG_MIKEY_UART;
    if (config_debug.trace_redeye)       flags |= TRACE_FLAG_REDEYE;
    if (config_debug.trace_mikey_audio)  flags |= TRACE_FLAG_MIKEY_AUDIO;
    if (config_debug.trace_cart)         flags |= TRACE_FLAG_CART_SHIFT;
    if (config_debug.trace_debug_messages) flags |= TRACE_FLAG_DEBUG_MSG;
    return flags;
}

static void trace_logger_set_config_flags(u32 flags)
{
    config_debug.trace_cpu = (flags & TRACE_FLAG_CPU) != 0;
    config_debug.trace_cpu_irq = (flags & TRACE_FLAG_CPU_IRQ) != 0;
    config_debug.trace_suzy_math = (flags & TRACE_FLAG_SUZY_MATH) != 0;
    config_debug.trace_suzy_sprites = (flags & TRACE_FLAG_SUZY_SPRITE) != 0;
    config_debug.trace_suzy_input = (flags & TRACE_FLAG_SUZY_INPUT) != 0;
    config_debug.trace_mikey_timers = (flags & TRACE_FLAG_MIKEY_TIMER) != 0;
    config_debug.trace_mikey_uart = (flags & TRACE_FLAG_MIKEY_UART) != 0;
    config_debug.trace_redeye = (flags & TRACE_FLAG_REDEYE) != 0;
    config_debug.trace_mikey_audio = (flags & TRACE_FLAG_MIKEY_AUDIO) != 0;
    config_debug.trace_cart = (flags & TRACE_FLAG_CART_SHIFT) != 0;
    config_debug.trace_debug_messages = (flags & TRACE_FLAG_DEBUG_MSG) != 0;
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
                snprintf(buf, buf_size, "  [SUZY] SPRITES   START  SCB:$%04X  Tick:%llu", entry.sprite.scb_addr,
                         (unsigned long long)entry.cycle);
            else if (entry.sprite.is_end)
                snprintf(buf, buf_size, "  [SUZY] SPRITES   END  Cycles:%u  Tick:%llu", entry.sprite.total_cycles,
                         (unsigned long long)entry.cycle);
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
        {
            char source[16] = "";
            char gap[24] = "";
            char lost[24] = "";

            if (entry.uart.kind == GLYNX_UART_TRACE_CFG)
            {
                bool turbo = (entry.uart.flags & 0x20) != 0;
                u32 baud = turbo ? 1000000u : 1000000u / ((entry.uart.backup + 1u) * 8u);
                snprintf(buf, buf_size, "  [MIKEY] UART CFG SERCTL:$%02X  %lu baud  %s  TX:%s RX:%s%s%s",
                         entry.uart.data, (unsigned long)baud,
                         (entry.uart.data & 0x10) ? ((entry.uart.data & 0x01) ? "PAR:EVEN" : "PAR:ODD ") : "PAR:OFF ",
                         (entry.uart.data & 0x80) ? "IRQ" : "-  ",
                         (entry.uart.data & 0x40) ? "IRQ" : "-  ",
                         (entry.uart.data & 0x04) ? "  TXOPEN" : "",
                         (entry.uart.data & 0x02) ? "  BREAK" : "");
                break;
            }

            if (entry.uart.kind == GLYNX_UART_TRACE_TX)
            {
                if (entry.uart.chained)
                    snprintf(source, sizeof(source), "  [CHAINED]");
            }
            else
            {
                if (entry.uart.kind == GLYNX_UART_TRACE_RX)
                    snprintf(source, sizeof(source), "  SRC:%s", entry.uart.source == 0 ? "LOCAL" : "LINK");

                snprintf(gap, sizeof(gap), entry.uart.kind == GLYNX_UART_TRACE_RD ? "  HELD:%uus" : "  GAP:%uus",
                         (unsigned)entry.uart.gap_us);

                if (entry.uart.flags & 0x10)
                    snprintf(lost, sizeof(lost), "  [OVERRUN lost:$%02X]", entry.uart.lost);
            }

            static const char* k_kind[] = { "TX", "RX", "RD" };

            snprintf(buf, buf_size, "  [MIKEY] UART %s  Data:$%02X  BIT9:%d%s%s%s%s%s%s",
                     k_kind[entry.uart.kind < 3 ? entry.uart.kind : 0], entry.uart.data,
                     (entry.uart.flags & 0x01) ? 1 : 0, source, gap, lost,
                     (entry.uart.flags & 0x02) ? "  [PARERR]" : "",
                     (entry.uart.flags & 0x04) ? "  [FRAMERR]" : "",
                     (entry.uart.flags & 0x08) ? "  [BREAK]" : "");
            break;
        }
        case TRACE_REDEYE:
        {
            static const char* k_msg[] = { "LOGON  ", "MSG1   ", "START  ", "DATA   ",
                                           "REQUEST", "RESEND ", "MSG6   ", "MSG7   " };
            const char* csum = entry.redeye.checksum_ok ? "" : "  [BAD CSUM]";
            u8 msg = entry.redeye.msg & 7;
            const char* dir = entry.redeye.dir ? "RX" : "TX";

            // Logon packets carry player, player mask and game id rather than a
            // packed header.
            if (entry.redeye.size == 5 && (msg == 0 || msg == 2) && entry.redeye.len >= 4)
            {
                snprintf(buf, buf_size, "  [REDEYE] %s %s plr:%d  players:$%02X  game:$%02X%02X%s",
                         dir, k_msg[msg], entry.redeye.payload[0], entry.redeye.payload[1],
                         entry.redeye.payload[3], entry.redeye.payload[2], csum);
                break;
            }

            if (msg == 5 && entry.redeye.len >= 1)
            {
                snprintf(buf, buf_size, "  [REDEYE] %s %s p%d seq%d  players:$%02X%s",
                         dir, k_msg[msg], entry.redeye.player, entry.redeye.seq,
                         entry.redeye.payload[0], csum);
                break;
            }

            char payload[48] = "";
            int at = 0;

            if (entry.redeye.len > 0)
                at = snprintf(payload, sizeof(payload), "  data:");

            for (int i = 0; i < entry.redeye.len && at < (int)sizeof(payload) - 4; i++)
                at += snprintf(payload + at, sizeof(payload) - at, " %02X", entry.redeye.payload[i]);

            snprintf(buf, buf_size, "  [REDEYE] %s %s p%d seq%d  size:%d%s%s",
                     dir, k_msg[msg], entry.redeye.player, entry.redeye.seq,
                     entry.redeye.size, payload, csum);
            break;
        }
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
        case TRACE_DEBUG_MESSAGE:
            snprintf(buf, buf_size, "  [DEBUG] %s", entry.debug_msg.text);
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
        TextColoredEx("  %s%s", c_white.c_str(), instr.c_str());

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
        case TRACE_REDEYE:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(brown, "%s", buf);
            break;
        case TRACE_MIKEY_AUDIO:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(blue, "%s", buf);
            break;
        case TRACE_CART_SHIFT:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(magenta, "%s", buf);
            break;
        case TRACE_DEBUG_MESSAGE:
            format_entry_text(entry, buf, sizeof(buf));
            ImGui::TextColored(white, "%s", buf);
            break;
        default:
            break;
    }
}
