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

#define GUI_DEBUG_MEMORY_IMPORT
#include "gui_debug_memory.h"

#include "../../../src/gearlynx.h"
#include "imgui/imgui.h"
#include "gui_debug_memeditor.h"
#include "gui_filedialogs.h"
#include "config.h"
#include "gui.h"
#include "emu.h"

static MemEditor mem_edit[MEMORY_EDITOR_MAX];
static int mem_edit_select = -1;
static int current_mem_edit = 0;
static char set_value_buffer[5] = {0};

static void memory_editor_menu(void);

void gui_debug_window_memory(void)
{
    for (int i = 0; i < MEMORY_EDITOR_MAX; i++)
    {
        mem_edit[i].SetGuiFont(gui_roboto_font);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(670, 330), ImGuiCond_FirstUseEver);

    ImGui::Begin("Memory Editor", &config_debug.show_memory, ImGuiWindowFlags_MenuBar);

    memory_editor_menu();

    GearlynxCore* core = emu_get_core();
    Memory* memory = core->GetMemory();
    Cartridge* cart = core->GetCartridge();


    if (ImGui::BeginTabBar("##memory_tabs", ImGuiTabBarFlags_None))
    {
        //TODO: Add memory editor tabs
        // if (ImGui::BeginTabItem("RAM", NULL, mem_edit_select == MEMORY_EDITOR_RAM ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        // {
        //     ImGui::PushFont(gui_default_font);
        //      if (mem_edit_select == MEMORY_EDITOR_RAM)
        //         mem_edit_select = -1;
        //     current_mem_edit = MEMORY_EDITOR_RAM;
        //     mem_edit[current_mem_edit].Draw(memory->GetWram(), 0x2000);
        //     ImGui::PopFont();
        //     ImGui::EndTabItem();
        // }

        // if (ImGui::BeginTabItem("ZERO PAGE", NULL, mem_edit_select == MEMORY_EDITOR_ZERO_PAGE ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        // {
        //     ImGui::PushFont(gui_default_font);
        //      if (mem_edit_select == MEMORY_EDITOR_ZERO_PAGE)
        //         mem_edit_select = -1;
        //     current_mem_edit = MEMORY_EDITOR_ZERO_PAGE;
        //     mem_edit[current_mem_edit].Draw(memory->GetWram(), 0x100);
        //     ImGui::PopFont();
        //     ImGui::EndTabItem();
        // }

        // if (IsValidPointer(cart->GetROM()) && ImGui::BeginTabItem("ROM", NULL, mem_edit_select == MEMORY_EDITOR_ROM ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        // {
        //     ImGui::PushFont(gui_default_font);
        //     if (mem_edit_select == MEMORY_EDITOR_ROM)
        //         mem_edit_select = -1;
        //     current_mem_edit = MEMORY_EDITOR_ROM;
        //     mem_edit[current_mem_edit].Draw(cart->GetROM(), cart->GetROMSize());
        //     ImGui::PopFont();
        //     ImGui::EndTabItem();
        // }

        // if (ImGui::BeginTabItem("VRAM", NULL, mem_edit_select == MEMORY_EDITOR_VRAM ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        // {
        //     ImGui::PushFont(gui_default_font);
        //     if (mem_edit_select == MEMORY_EDITOR_VRAM)
        //         mem_edit_select = -1;
        //     current_mem_edit = MEMORY_EDITOR_VRAM;
        //     mem_edit[current_mem_edit].Draw((u8*)huc6270->GetVRAM(), HUC6270_VRAM_SIZE, 0, 2);
        //     ImGui::PopFont();
        //     ImGui::EndTabItem();
        // }

        // if (ImGui::BeginTabItem("SAT", NULL, mem_edit_select == MEMORY_EDITOR_SAT ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        // {
        //     ImGui::PushFont(gui_default_font);
        //     if (mem_edit_select == MEMORY_EDITOR_SAT)
        //         mem_edit_select = -1;
        //     current_mem_edit = MEMORY_EDITOR_SAT;
        //     mem_edit[current_mem_edit].Draw((u8*)huc6270->GetSAT(), HUC6270_SAT_SIZE, 0, 2);
        //     ImGui::PopFont();
        //     ImGui::EndTabItem();
        // }

        // if (ImGui::BeginTabItem("PALETTES", NULL, mem_edit_select == MEMORY_EDITOR_PALETTES ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        // {
        //     ImGui::PushFont(gui_default_font);
        //     if (mem_edit_select == MEMORY_EDITOR_PALETTES)
        //         mem_edit_select = -1;
        //     current_mem_edit = MEMORY_EDITOR_PALETTES;
        //     mem_edit[current_mem_edit].Draw((u8*)huc6260->GetColorTable(), 512, 0, 2);
        //     ImGui::PopFont();
        //     ImGui::EndTabItem();
        // }

        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_copy_memory(void)
{
    mem_edit[current_mem_edit].Copy();
}

void gui_debug_paste_memory(void)
{
    mem_edit[current_mem_edit].Paste();
}

void gui_debug_select_all(void)
{
    mem_edit[current_mem_edit].SelectAll();
}

void gui_debug_memory_goto(int editor, int address)
{
    mem_edit_select = editor;
    mem_edit[mem_edit_select].JumpToAddress(address);
}

void gui_debug_save_memory_dump(const char* file_path)
{
    mem_edit[current_mem_edit].SaveToFile(file_path);
}

static void memory_editor_menu(void)
{
    ImGui::BeginMenuBar();

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Memory As..."))
        {
            gui_file_dialog_save_memory_dump();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Copy", "Ctrl+C"))
        {
            gui_debug_copy_memory();
        }

        if (ImGui::MenuItem("Paste", "Ctrl+V"))
        {
            gui_debug_paste_memory();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Selection"))
    {
        if (ImGui::MenuItem("Select All", "Ctrl+A"))
        {
            mem_edit[current_mem_edit].SelectAll();
        }

        if (ImGui::MenuItem("Clear Selection"))
        {
            mem_edit[current_mem_edit].ClearSelection();
        }

        if (ImGui::BeginMenu("Set value"))
        {
            ImGui::SetNextItemWidth(50);
            if (ImGui::InputTextWithHint("##set_value", "XXXX", set_value_buffer, IM_ARRAYSIZE(set_value_buffer), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
            {
                try
                {
                    mem_edit[current_mem_edit].SetValueToSelection((int)std::stoul(set_value_buffer, 0, 16));
                    set_value_buffer[0] = 0;
                }
                catch(const std::invalid_argument&)
                {
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Set!", ImVec2(40, 0)))
            {
                try
                {
                    mem_edit[current_mem_edit].SetValueToSelection((int)std::stoul(set_value_buffer, 0, 16));
                    set_value_buffer[0] = 0;
                }
                catch(const std::invalid_argument&)
                {
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Bookmarks"))
    {
        if (ImGui::MenuItem("Clear All"))
        {
            mem_edit[current_mem_edit].RemoveBookmarks();
        }

        if (ImGui::MenuItem("Add Bookmark"))
        {
            mem_edit[current_mem_edit].AddBookmark();
        }

        std::vector<MemEditor::Bookmark>* bookmarks = mem_edit[current_mem_edit].GetBookmarks();

        if (bookmarks->size() > 0)
            ImGui::Separator();

        for (long unsigned int i = 0; i < bookmarks->size(); i++)
        {
            MemEditor::Bookmark* bookmark = &(*bookmarks)[i];

            char label[80];
            snprintf(label, 80, "$%04X: %s", bookmark->address, bookmark->name);

            if (ImGui::MenuItem(label))
            {
                mem_edit[current_mem_edit].JumpToAddress(bookmark->address);
            }
        }

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}
