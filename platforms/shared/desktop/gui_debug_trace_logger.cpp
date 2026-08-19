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
#include "trace_logger_formatter.h"
#include <cstring>

#define TRACE_DISK_BUFFER_SIZE (1024 * 1024)
#define TRACE_DISK_STAGING_CAPACITY 100000

static bool trace_logger_enabled = false;
static bool trace_logger_follow_latest = true;
static bool trace_logger_scroll_to_bottom = false;
static bool trace_logger_wait_for_scroll_away = false;
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
static bool trace_logger_disk_error = false;
static Uint64 trace_logger_disk_last_flush = 0;
static GLYNX_Trace_Entry trace_logger_disk_previous = {};
static bool trace_logger_disk_previous_valid = false;

static const u32 trace_logger_capacities[] = {100000, 500000, 1000000, 2000000, 5000000};
static const char* trace_logger_capacity_names[] = {"100K", "500K", "1M", "2M", "5M"};
static const char* trace_logger_capacity_labels[] = {"100K (10 MB)", "500K (50 MB)", "1M (100 MB)", "2M (200 MB)", "5M (500 MB)"};
static const u64 trace_logger_disk_sizes[] = {
    10ULL * 1024ULL * 1024ULL,
    50ULL * 1024ULL * 1024ULL,
    100ULL * 1024ULL * 1024ULL,
    250ULL * 1024ULL * 1024ULL,
    500ULL * 1024ULL * 1024ULL,
    1024ULL * 1024ULL * 1024ULL,
    0
};
static const char* trace_logger_disk_size_names[] =
    {"10MB", "50MB", "100MB", "250MB", "500MB", "1GB", "unbounded"};

static void trace_logger_menu(void);
static void trace_logger_sync_flags(void);
static void trace_logger_menu_event_filter(const char* label, int* filter, u32 mask);
static u32 trace_logger_get_config_flags(void);
static void trace_logger_set_config_flags(u32 flags);
static bool trace_logger_apply_capacity(void);
static bool trace_logger_start_disk(void);
static bool trace_logger_start(u32 flags, bool update_config);
static bool trace_logger_stop(bool show_status);
static bool trace_logger_stop_disk(bool show_status, bool flush_entries);
static bool trace_logger_flush_disk_buffer(bool flush_file);
static bool trace_logger_flush_disk_entries(void);
static void format_entry_text(const GLYNX_Trace_Entry& entry, bool cycles,
    const GLYNX_Trace_Entry* previous, char* buf, int buf_size);
static void format_entry_text(const GLYNX_Trace_Entry& entry, const GLYNX_Trace_Entry* previous,
    char* buf, int buf_size);
static void render_cpu_entry_colored(const GLYNX_Trace_Entry& entry, int prefix_length);
static void render_entry_colored(const GLYNX_Trace_Entry& entry, const GLYNX_Trace_Entry* previous, u64 index);

static void trace_logger_reset_event_pairing(void)
{
    emu_get_core()->GetMikey()->ResetTraceUARTEventPairing();
    emu_get_core()->GetSuzy()->ResetTraceEventPairing();
}

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
            trace_logger_start(trace_logger_get_config_flags(), false);
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
        if (ImGui::Combo("##trace_capacity", &config_debug.trace_capacity, trace_logger_capacity_labels, IM_ARRAYSIZE(trace_logger_capacity_labels)) && !trace_logger_apply_capacity())
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
                u64 entry_number = tl->GetSequence() - (u64)count + (u64)item;
                const GLYNX_Trace_Entry* previous = item > 0 ? &tl->GetEntry((u32)item - 1) : NULL;
                render_entry_colored(entry, previous, entry_number);
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
            trace_logger_disk_error = true;
            trace_logger_stop_disk(false, false);
        }
        else if (trace_logger_disk_limit_reached)
        {
            trace_logger_stop_disk(false, false);
            gui_set_status_message("Trace recording stopped: maximum file size reached", 4000);
        }
        else
        {
            Uint64 now = SDL_GetTicks();
            if ((now - trace_logger_disk_last_flush) >= 1000)
            {
                if (!trace_logger_flush_disk_buffer(true))
                {
                    trace_logger_disk_error = true;
                    trace_logger_stop_disk(false, false);
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
        trace_logger_stop_disk(false, true);
}

void gui_debug_trace_logger_clear(void)
{
    TraceLogger* tl = emu_get_core()->GetTraceLogger();
    if (trace_logger_enabled && config_debug.trace_output == gui_TraceOutput_Disk)
    {
        if (trace_logger_flush_disk_entries() && trace_logger_flush_disk_buffer(true))
        {
            tl->Reset();
            trace_logger_reset_event_pairing();
            trace_logger_disk_flushed_total = 0;
        }
        else
            gui_set_error_message("Error writing trace log to disk.");
    }
    else
    {
        tl->Reset();
        trace_logger_reset_event_pairing();
    }
}

void gui_debug_trace_logger_reset(void)
{
    trace_logger_stop(false);
    emu_get_core()->GetTraceLogger()->Reset();
    trace_logger_reset_event_pairing();
    trace_logger_disk_flushed_total = 0;
    trace_logger_disk_previous_valid = false;
}

void gui_debug_trace_logger_set_output_directory(const char* path)
{
    strncpy_fit(trace_logger_disk_directory, path, sizeof(trace_logger_disk_directory));
    config_debug.trace_disk_path.assign(path);
}

int gui_debug_trace_logger_memory_size_index(const char* size)
{
    if (size)
    {
        for (int i = 0; i < IM_ARRAYSIZE(trace_logger_capacity_names); i++)
        {
            if (strcmp(size, trace_logger_capacity_names[i]) == 0)
                return i;
        }
    }
    return -1;
}

int gui_debug_trace_logger_disk_size_index(const char* size)
{
    if (size)
    {
        for (int i = 0; i < IM_ARRAYSIZE(trace_logger_disk_size_names); i++)
        {
            if (strcmp(size, trace_logger_disk_size_names[i]) == 0)
                return i;
        }
    }
    return -1;
}

const char* gui_debug_trace_logger_memory_size_name(int index)
{
    if (index < 0 || index >= IM_ARRAYSIZE(trace_logger_capacity_names))
        return trace_logger_capacity_names[0];
    return trace_logger_capacity_names[index];
}

const char* gui_debug_trace_logger_disk_size_name(int index)
{
    if (index < 0 || index >= IM_ARRAYSIZE(trace_logger_disk_size_names))
        return trace_logger_disk_size_names[2];
    return trace_logger_disk_size_names[index];
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

void gui_debug_trace_logger_set_event_filters(const u32* filters)
{
    if (!filters)
        return;

    TraceLogger* trace_logger = emu_get_core()->GetTraceLogger();
    for (int i = 0; i < TRACE_TYPE_COUNT; i++)
        trace_logger->SetEventFilter((GLYNX_Trace_Type)i, filters[i]);
}

bool gui_debug_trace_logger_start(u32 flags)
{
    return trace_logger_start(flags, true);
}

static bool trace_logger_start(u32 flags, bool update_config)
{
    if (flags == 0)
    {
        flags = TRACE_FLAG_CPU | TRACE_FLAG_CPU_IRQ;
        update_config = true;
    }
    if (update_config)
        trace_logger_set_config_flags(flags);

    if (trace_logger_enabled)
    {
        trace_logger_sync_flags();
        return true;
    }

    if (config_debug.trace_output == gui_TraceOutput_Disk && !trace_logger_start_disk())
        return false;

    trace_logger_enabled = true;
    trace_logger_follow_latest = true;
    trace_logger_scroll_to_bottom = true;
    trace_logger_wait_for_scroll_away = false;
    trace_logger_sync_flags();
    return true;
}

bool gui_debug_trace_logger_stop(void)
{
    return trace_logger_stop(true);
}

static bool trace_logger_stop(bool show_status)
{
    if (!trace_logger_enabled)
        return true;

    trace_logger_reset_event_pairing();
    trace_logger_scroll_to_bottom = trace_logger_follow_latest;

    if (config_debug.trace_output == gui_TraceOutput_Disk)
        return trace_logger_stop_disk(show_status, true);

    trace_logger_enabled = false;
    emu_get_core()->GetTraceLogger()->SetEnabledFlags(0);
    return true;
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
        char buf[GLYNX_TRACE_FORMAT_BUFFER_SIZE];
        bool success = true;
        u64 oldest = tl->GetSequence() - count;

        for (u32 i = 0; i < count; i++)
        {
            const GLYNX_Trace_Entry& entry = tl->GetEntry(i);
            const GLYNX_Trace_Entry* previous = i > 0 ? &tl->GetEntry(i - 1) : NULL;
            format_entry_text(entry, previous, buf, sizeof(buf));
            if (config_debug.trace_counter)
                success = fprintf(file, "%06llu %s\n", (unsigned long long)(oldest + i), buf) >= 0;
            else
                success = fprintf(file, "%s\n", buf) >= 0;
            if (!success)
                break;
        }

        if (fflush(file) != 0 || fclose(file) != 0)
            success = false;
        if (!success)
            gui_set_error_message("Error writing trace log file.");
    }
    else
        gui_set_error_message("Unable to create trace log file.");
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
        ImGui::MenuItem("Event Counter", "", &config_debug.trace_counter);
        ImGui::MenuItem("Master Clock Cycles", "", &config_debug.trace_cycles);

        if (ImGui::BeginMenu("CPU"))
        {
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

    if (ImGui::BeginMenu("Filters"))
    {
        if (ImGui::BeginMenu("CPU"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_cpu_enabled);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_cpu_enabled);
            ImGui::MenuItem("Instructions", "", &config_debug.trace_cpu);
            ImGui::MenuItem("IRQs", "", &config_debug.trace_cpu_irq);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Suzy Math"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_suzy_math);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_suzy_math);
            trace_logger_menu_event_filter("Operations", &config_debug.trace_suzy_math_events, TRACE_SUZY_MATH_FILTER_OPERATIONS);
            trace_logger_menu_event_filter("Completions", &config_debug.trace_suzy_math_events, TRACE_SUZY_MATH_FILTER_COMPLETIONS);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Suzy Sprites"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_suzy_sprites);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_suzy_sprites);
            trace_logger_menu_event_filter("Engine", &config_debug.trace_suzy_sprite_events, TRACE_SUZY_SPRITE_FILTER_ENGINE);
            trace_logger_menu_event_filter("SCBs", &config_debug.trace_suzy_sprite_events, TRACE_SUZY_SPRITE_FILTER_SCBS);
            trace_logger_menu_event_filter("Skips", &config_debug.trace_suzy_sprite_events, TRACE_SUZY_SPRITE_FILTER_SKIPS);
            trace_logger_menu_event_filter("Collisions", &config_debug.trace_suzy_sprite_events, TRACE_SUZY_SPRITE_FILTER_COLLISIONS);
            trace_logger_menu_event_filter("Rows", &config_debug.trace_suzy_sprite_events, TRACE_SUZY_SPRITE_FILTER_ROWS);
            trace_logger_menu_event_filter("Bus", &config_debug.trace_suzy_sprite_events, TRACE_SUZY_SPRITE_FILTER_BUS);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Suzy Input"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_suzy_input);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_suzy_input);
            trace_logger_menu_event_filter("Reads", &config_debug.trace_suzy_input_events, TRACE_SUZY_INPUT_FILTER_READS);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Mikey Timers"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_mikey_timers);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_mikey_timers);
            trace_logger_menu_event_filter("Registers", &config_debug.trace_mikey_timer_events, TRACE_MIKEY_TIMER_FILTER_REGISTERS);
            trace_logger_menu_event_filter("Underflows", &config_debug.trace_mikey_timer_events, TRACE_MIKEY_TIMER_FILTER_UNDERFLOWS);
            trace_logger_menu_event_filter("IRQs", &config_debug.trace_mikey_timer_events, TRACE_MIKEY_TIMER_FILTER_IRQS);
            trace_logger_menu_event_filter("Links", &config_debug.trace_mikey_timer_events, TRACE_MIKEY_TIMER_FILTER_LINKS);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Mikey Interrupts"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_mikey_interrupts);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_mikey_interrupts);
            trace_logger_menu_event_filter("Registers and Line", &config_debug.trace_mikey_interrupt_events, TRACE_MIKEY_INTERRUPT_FILTER_ALL);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Mikey Display"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_mikey_display);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_mikey_display);
            trace_logger_menu_event_filter("Registers", &config_debug.trace_mikey_display_events, TRACE_MIKEY_DISPLAY_FILTER_REGISTERS);
            trace_logger_menu_event_filter("Palette", &config_debug.trace_mikey_display_events, TRACE_MIKEY_DISPLAY_FILTER_PALETTE);
            trace_logger_menu_event_filter("DMA", &config_debug.trace_mikey_display_events, TRACE_MIKEY_DISPLAY_FILTER_DMA);
            trace_logger_menu_event_filter("Timing", &config_debug.trace_mikey_display_events, TRACE_MIKEY_DISPLAY_FILTER_TIMING);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Mikey UART"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_mikey_uart);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_mikey_uart);
            trace_logger_menu_event_filter("Registers", &config_debug.trace_mikey_uart_events, TRACE_MIKEY_UART_FILTER_REGISTERS);
            trace_logger_menu_event_filter("Transfers", &config_debug.trace_mikey_uart_events, TRACE_MIKEY_UART_FILTER_TRANSFERS);
            trace_logger_menu_event_filter("IRQs", &config_debug.trace_mikey_uart_events, TRACE_MIKEY_UART_FILTER_IRQS);
            trace_logger_menu_event_filter("Problems", &config_debug.trace_mikey_uart_events, TRACE_MIKEY_UART_FILTER_PROBLEMS);
            trace_logger_menu_event_filter("Breaks", &config_debug.trace_mikey_uart_events, TRACE_MIKEY_UART_FILTER_BREAKS);
            trace_logger_menu_event_filter("ComLynx", &config_debug.trace_mikey_uart_events, TRACE_MIKEY_UART_FILTER_COMLYNX);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("RedEye"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_redeye);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_redeye);
            trace_logger_menu_event_filter("Packets", &config_debug.trace_redeye_events, TRACE_REDEYE_FILTER_PACKETS);
            trace_logger_menu_event_filter("Problems", &config_debug.trace_redeye_events, TRACE_REDEYE_FILTER_PROBLEMS);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Mikey Audio"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_mikey_audio);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_mikey_audio);
            trace_logger_menu_event_filter("Channels", &config_debug.trace_mikey_audio_events, TRACE_MIKEY_AUDIO_FILTER_CHANNELS);
            trace_logger_menu_event_filter("Mixer", &config_debug.trace_mikey_audio_events, TRACE_MIKEY_AUDIO_FILTER_MIXER);
            trace_logger_menu_event_filter("Clocks", &config_debug.trace_mikey_audio_events, TRACE_MIKEY_AUDIO_FILTER_CLOCKS);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Cartridge"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_cart);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_cart);
            trace_logger_menu_event_filter("Address", &config_debug.trace_cartridge_events, TRACE_CARTRIDGE_FILTER_ADDRESS);
            trace_logger_menu_event_filter("Accesses", &config_debug.trace_cartridge_events, TRACE_CARTRIDGE_FILTER_ACCESSES);
            trace_logger_menu_event_filter("EEPROM", &config_debug.trace_cartridge_events, TRACE_CARTRIDGE_FILTER_EEPROM);
            trace_logger_menu_event_filter("AUDIN", &config_debug.trace_cartridge_events, TRACE_CARTRIDGE_FILTER_AUDIN);
            trace_logger_menu_event_filter("Storage", &config_debug.trace_cartridge_events, TRACE_CARTRIDGE_FILTER_STORAGE);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        ImGui::MenuItem("Debug Messages", "", &config_debug.trace_debug_messages);

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

static void trace_logger_menu_event_filter(const char* label, int* filter, u32 mask)
{
    bool enabled = ((u32)*filter & mask) != 0;
    if (ImGui::MenuItem(label, "", &enabled))
    {
        if (enabled)
            *filter |= (int)mask;
        else
            *filter &= ~(int)mask;
    }
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
    if (!get_local_time(now, &local_time))
    {
        gui_set_error_message("Unable to read local time.");
        return false;
    }
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
    trace_logger_disk_error = false;
    trace_logger_disk_last_flush = SDL_GetTicks();
    trace_logger_disk_previous_valid = false;
    emu_get_core()->GetTraceLogger()->Reset();
    gui_set_status_message("Trace recording started", 3000);
    return true;
}

static bool trace_logger_stop_disk(bool show_status, bool flush_entries)
{
    bool success = trace_logger_disk_file != NULL && !trace_logger_disk_error &&
        !trace_logger_disk_overflow;

    if (trace_logger_disk_file)
    {
        if (flush_entries && !trace_logger_flush_disk_entries())
            success = false;
        if (!trace_logger_flush_disk_buffer(true))
            success = false;
        if (fclose(trace_logger_disk_file) != 0)
            success = false;
        trace_logger_disk_file = NULL;
    }

    trace_logger_enabled = false;
    emu_get_core()->GetTraceLogger()->SetEnabledFlags(0);
    trace_logger_reset_event_pairing();
    if (success)
    {
        if (show_status)
            gui_set_status_message("Trace recording stopped", 3000);
    }
    else
    {
        const char* message = trace_logger_disk_overflow ?
            "Trace recording stopped: staging buffer overflow." :
            "Trace recording stopped with a disk write error.";
        gui_set_error_message(message);
        Error("%s File: %s", message, trace_logger_disk_path);
    }
    return success;
}

static bool trace_logger_flush_disk_buffer(bool flush_file)
{
    if (!trace_logger_disk_file)
        return false;

    if (trace_logger_disk_buffer_used > 0)
    {
        size_t written = fwrite(trace_logger_disk_buffer, 1, trace_logger_disk_buffer_used, trace_logger_disk_file);
        if (written > 0)
        {
            trace_logger_disk_buffer_used -= written;
            if (trace_logger_disk_buffer_used > 0)
                memmove(trace_logger_disk_buffer, trace_logger_disk_buffer + written, trace_logger_disk_buffer_used);
        }
        if (trace_logger_disk_buffer_used > 0)
            return false;
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
    char entry_text[GLYNX_TRACE_FORMAT_BUFFER_SIZE];
    char line[GLYNX_TRACE_FORMAT_BUFFER_SIZE + 32];

    for (u32 i = first; i < count; i++)
    {
        const GLYNX_Trace_Entry* previous = i > 0 ? &tl->GetEntry(i - 1) :
            (trace_logger_disk_previous_valid ? &trace_logger_disk_previous : NULL);
        format_entry_text(tl->GetEntry(i), previous, entry_text, sizeof(entry_text));
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
        trace_logger_disk_previous = tl->GetEntry(i);
        trace_logger_disk_previous_valid = true;
        trace_logger_disk_flushed_total = oldest + (u64)i + 1;
    }

    return true;
}

static void trace_logger_sync_flags(void)
{
    TraceLogger* logger = emu_get_core()->GetTraceLogger();
    logger->SetEnabledFlags(trace_logger_get_config_flags());
    logger->SetEventFilter(TRACE_SUZY_MATH, (u32)config_debug.trace_suzy_math_events);
    logger->SetEventFilter(TRACE_SUZY_SPRITE, (u32)config_debug.trace_suzy_sprite_events);
    logger->SetEventFilter(TRACE_SUZY_INPUT, (u32)config_debug.trace_suzy_input_events);
    logger->SetEventFilter(TRACE_MIKEY_TIMER, (u32)config_debug.trace_mikey_timer_events);
    logger->SetEventFilter(TRACE_MIKEY_INTERRUPT, (u32)config_debug.trace_mikey_interrupt_events);
    logger->SetEventFilter(TRACE_MIKEY_DISPLAY, (u32)config_debug.trace_mikey_display_events);
    logger->SetEventFilter(TRACE_MIKEY_UART, (u32)config_debug.trace_mikey_uart_events);
    logger->SetEventFilter(TRACE_REDEYE, (u32)config_debug.trace_redeye_events);
    logger->SetEventFilter(TRACE_MIKEY_AUDIO, (u32)config_debug.trace_mikey_audio_events);
    logger->SetEventFilter(TRACE_CARTRIDGE, (u32)config_debug.trace_cartridge_events);
    logger->SetEventFilter(TRACE_DEBUG_MESSAGE, (u32)config_debug.trace_debug_events);
}

static u32 trace_logger_get_config_flags(void)
{
    u32 flags = 0;
    if (config_debug.trace_cpu_enabled && config_debug.trace_cpu)     flags |= TRACE_FLAG_CPU;
    if (config_debug.trace_cpu_enabled && config_debug.trace_cpu_irq) flags |= TRACE_FLAG_CPU_IRQ;
    if (config_debug.trace_suzy_math)    flags |= TRACE_FLAG_SUZY_MATH;
    if (config_debug.trace_suzy_sprites) flags |= TRACE_FLAG_SUZY_SPRITE;
    if (config_debug.trace_suzy_input)   flags |= TRACE_FLAG_SUZY_INPUT;
    if (config_debug.trace_mikey_timers) flags |= TRACE_FLAG_MIKEY_TIMER;
    if (config_debug.trace_mikey_interrupts) flags |= TRACE_FLAG_MIKEY_INTERRUPT;
    if (config_debug.trace_mikey_display) flags |= TRACE_FLAG_MIKEY_DISPLAY;
    if (config_debug.trace_mikey_uart)   flags |= TRACE_FLAG_MIKEY_UART;
    if (config_debug.trace_redeye)       flags |= TRACE_FLAG_REDEYE;
    if (config_debug.trace_mikey_audio)  flags |= TRACE_FLAG_MIKEY_AUDIO;
    if (config_debug.trace_cart)         flags |= TRACE_FLAG_CARTRIDGE;
    if (config_debug.trace_debug_messages) flags |= TRACE_FLAG_DEBUG_MSG;
    return flags;
}

static void trace_logger_set_config_flags(u32 flags)
{
    config_debug.trace_cpu_enabled = (flags & (TRACE_FLAG_CPU | TRACE_FLAG_CPU_IRQ)) != 0;
    config_debug.trace_cpu = (flags & TRACE_FLAG_CPU) != 0;
    config_debug.trace_cpu_irq = (flags & TRACE_FLAG_CPU_IRQ) != 0;
    config_debug.trace_suzy_math = (flags & TRACE_FLAG_SUZY_MATH) != 0;
    config_debug.trace_suzy_sprites = (flags & TRACE_FLAG_SUZY_SPRITE) != 0;
    config_debug.trace_suzy_input = (flags & TRACE_FLAG_SUZY_INPUT) != 0;
    config_debug.trace_mikey_timers = (flags & TRACE_FLAG_MIKEY_TIMER) != 0;
    config_debug.trace_mikey_interrupts = (flags & TRACE_FLAG_MIKEY_INTERRUPT) != 0;
    config_debug.trace_mikey_display = (flags & TRACE_FLAG_MIKEY_DISPLAY) != 0;
    config_debug.trace_mikey_uart = (flags & TRACE_FLAG_MIKEY_UART) != 0;
    config_debug.trace_redeye = (flags & TRACE_FLAG_REDEYE) != 0;
    config_debug.trace_mikey_audio = (flags & TRACE_FLAG_MIKEY_AUDIO) != 0;
    config_debug.trace_cart = (flags & TRACE_FLAG_CARTRIDGE) != 0;
    config_debug.trace_debug_messages = (flags & TRACE_FLAG_DEBUG_MSG) != 0;
}

static void format_entry_text(const GLYNX_Trace_Entry& entry, bool cycles,
    const GLYNX_Trace_Entry* previous, char* buf, int buf_size)
{
    GLYNX_Trace_Format_Options options = {};
    options.registers = config_debug.trace_registers;
    options.flags = config_debug.trace_flags;
    options.bytes = config_debug.trace_bytes;
    options.cycles = cycles;
    options.previous = previous;
    trace_logger_format_entry(entry, options, buf, (size_t)buf_size);
}

static void format_entry_text(const GLYNX_Trace_Entry& entry, const GLYNX_Trace_Entry* previous,
    char* buf, int buf_size)
{
    format_entry_text(entry, config_debug.trace_cycles, previous, buf, buf_size);
}

static void render_cpu_entry_colored(const GLYNX_Trace_Entry& entry, int prefix_length)
{
    ImGui::TextColored(cyan, "%04X", entry.cpu.pc);

    if (config_debug.trace_registers)
    {
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, "  A:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.a);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " X:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.x);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " Y:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.y);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " S:");
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

    if (entry.cpu.name[0] != 0)
    {
        std::string instr = entry.cpu.name;
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
    }
    else
    {
        ImGui::SameLine(0, 0);
        ImGui::TextColored(gray, "  ???");
    }

    if (config_debug.trace_bytes)
    {
        char bytes[16];
        trace_log_format_cpu_bytes(entry, bytes, sizeof(bytes));
        float char_width = ImGui::CalcTextSize("A").x;
        float bytes_column = char_width * 31;
        if (config_debug.trace_registers) bytes_column += char_width * 24;
        if (config_debug.trace_flags)     bytes_column += char_width * 10;
        bytes_column += char_width * prefix_length;
        ImGui::SameLine(bytes_column);
        ImGui::TextColored(gray, "%s", bytes);
    }
}

static void render_entry_colored(const GLYNX_Trace_Entry& entry,
    const GLYNX_Trace_Entry* previous, u64 index)
{
    char buf[GLYNX_TRACE_FORMAT_BUFFER_SIZE];
    int prefix_length = 0;

    if (config_debug.trace_counter)
    {
        char counter[32];
        snprintf(counter, sizeof(counter), "%06llu ", (unsigned long long)index);
        prefix_length += (int)strlen(counter);
        ImGui::TextColored(gray, "%s", counter);
        ImGui::SameLine(0, 0);
    }

    if (config_debug.trace_cycles)
    {
        char cycles[64];
        trace_log_format_cycle_prefix(entry, previous, cycles, sizeof(cycles));
        prefix_length += (int)strlen(cycles);
        ImGui::TextColored(gray, "%s", cycles);
        ImGui::SameLine(0, 0);
    }

    if (entry.type == TRACE_CPU)
    {
        render_cpu_entry_colored(entry, prefix_length);
        return;
    }

    format_entry_text(entry, false, NULL, buf, sizeof(buf));
    ImVec4 color = white;
    switch (entry.type)
    {
        case TRACE_CPU_IRQ: color = red; break;
        case TRACE_SUZY_MATH: color = cyan; break;
        case TRACE_SUZY_SPRITE: color = green; break;
        case TRACE_SUZY_INPUT: color = yellow; break;
        case TRACE_MIKEY_TIMER: color = orange; break;
        case TRACE_MIKEY_INTERRUPT: color = red; break;
        case TRACE_MIKEY_DISPLAY: color = green; break;
        case TRACE_MIKEY_UART: color = violet; break;
        case TRACE_REDEYE: color = brown; break;
        case TRACE_MIKEY_AUDIO: color = blue; break;
        case TRACE_CARTRIDGE: color = magenta; break;
        default: break;
    }
    ImGui::TextColored(color, "%s", buf);
}
