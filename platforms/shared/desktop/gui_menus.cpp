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

#define GUI_MENUS_IMPORT
#include "gui_menus.h"
#include "gui.h"
#include "gui_filedialogs.h"
#include "gui_popups.h"
#include "gui_actions.h"
#include "gui_debug_disassembler.h"
#include "gui_debug_widgets.h"
#include "config.h"
#include "application.h"
#include "display.h"
#include "gamepad.h"
#include "emu.h"
#include "ogl_renderer.h"
#include "utils.h"
#include "gearlynx.h"

static bool open_rom = false;
static bool open_ram = false;
static bool save_ram = false;
static bool open_state = false;
static bool save_state = false;
static bool open_about = false;
static bool save_screenshot = false;
static bool save_vgm = false;
static bool choose_savestates_path = false;
static bool choose_savefiles_path = false;
static bool choose_screenshots_path = false;
static bool open_bios = false;
static bool open_bios_warning = false;
static bool save_debug_settings = false;
static bool load_debug_settings = false;

static void menu_gearlynx(void);
static void menu_emulator(void);
static void menu_video(void);
static void menu_input(void);
static void menu_audio(void);
static void menu_debug(void);
static void menu_about(void);
static void file_dialogs(void);
static void keyboard_configuration_item(const char* text, SDL_Scancode* key);
static void gamepad_configuration_item(const char* text, int* button);
static void hotkey_configuration_item(const char* text, config_Hotkey* hotkey);
static void gamepad_device_selector(void);
static void draw_savestate_slot_info(int slot);

void gui_init_menus(void)
{
    gui_shortcut_open_rom = false;
}

void gui_main_menu(void)
{
    open_rom = false;
    open_ram = false;
    save_ram = false;
    open_state = false;
    save_state = false;
    open_about = false;
    save_screenshot = false;
    save_vgm = false;
    choose_savestates_path = false;
    choose_savefiles_path = false;
    choose_screenshots_path = false;
    open_bios = false;
    open_bios_warning = false;
    save_debug_settings = false;
    load_debug_settings = false;
    gui_main_menu_hovered = false;

    if (application_show_menu && ImGui::BeginMainMenuBar())
    {
        gui_main_menu_hovered = ImGui::IsWindowHovered();

        menu_gearlynx();
        menu_emulator();
        menu_video();
        menu_input();
        menu_audio();
        menu_debug();
        menu_about();

        gui_main_menu_height = (int)ImGui::GetWindowSize().y;

        ImGui::EndMainMenuBar();       
    }

    file_dialogs();
}

static void menu_gearlynx(void)
{
    if (ImGui::BeginMenu(GLYNX_TITLE))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("Open ROM...", config_hotkeys[config_HotkeyIndex_OpenROM].str))
        {
            if (emu_is_bios_loaded())
                open_rom = true;
            else
                open_bios_warning = true;
        }

        if (ImGui::BeginMenu("Open Recent"))
        {
            for (int i = 0; i < config_max_recent_roms; i++)
            {
                if (config_emulator.recent_roms[i].length() > 0)
                {
                    if (ImGui::MenuItem(config_emulator.recent_roms[i].c_str()))
                    {
                        if (emu_is_bios_loaded())
                        {
                            char rom_path[4096];
                            strcpy(rom_path, config_emulator.recent_roms[i].c_str());
                            gui_load_rom(rom_path);
                        }
                        else
                            open_bios_warning = true;
                    }
                }
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();
        
        if (ImGui::MenuItem("Reset", config_hotkeys[config_HotkeyIndex_Reset].str))
        {
            gui_action_reset();
        }

        if (ImGui::MenuItem("Pause", config_hotkeys[config_HotkeyIndex_Pause].str, &config_emulator.paused))
        {
            gui_action_pause();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Fast Forward", config_hotkeys[config_HotkeyIndex_FFWD].str, &config_emulator.ffwd))
        {
            gui_action_ffwd();
        }

        if (ImGui::BeginMenu("Fast Forward Speed"))
        {
            ImGui::PushItemWidth(100.0f);
            ImGui::Combo("##fwd", &config_emulator.ffwd_speed, "X 1.5\0X 2\0X 2.5\0X 3\0Unlimited\0\0");
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        ImGui::Separator();

        bool has_save_ram = !emu_is_empty() && (emu_get_core()->GetMedia()->GetSaveMemorySize() > 0);

        if (ImGui::MenuItem("Save RAM As...", NULL, false, has_save_ram))
        {
            save_ram = true;
        }

        if (ImGui::MenuItem("Load RAM From...", NULL, false, has_save_ram))
        {
            open_ram = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save State As..."))
        {
            save_state = true;
        }

        if (ImGui::MenuItem("Load State From..."))
        {
            open_state = true;
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Save State Slot"))
        {
            ImGui::PushItemWidth(100.0f);
            ImGui::Combo("##slot", &config_emulator.save_slot, "Slot 1\0Slot 2\0Slot 3\0Slot 4\0Slot 5\0\0");
            ImGui::PopItemWidth();

            ImGui::Separator();
            draw_savestate_slot_info(config_emulator.save_slot);

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Save State", config_hotkeys[config_HotkeyIndex_SaveState].str))
        {
            std::string message("Saving state to slot ");
            message += std::to_string(config_emulator.save_slot + 1);
            gui_set_status_message(message.c_str(), 3000);
            emu_save_state_slot(config_emulator.save_slot + 1);
        }

        if (ImGui::MenuItem("Load State", config_hotkeys[config_HotkeyIndex_LoadState].str))
        {
            std::string message("Loading state from slot ");
            message += std::to_string(config_emulator.save_slot + 1);
            gui_set_status_message(message.c_str(), 3000);
            emu_load_state_slot(config_emulator.save_slot + 1);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Slot: %d", config_emulator.save_slot + 1);
            ImGui::Separator();
            draw_savestate_slot_info(config_emulator.save_slot);
            ImGui::EndTooltip();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save Screenshot As..."))
        {
            save_screenshot = true;
        }

        if (ImGui::MenuItem("Save Screenshot", config_hotkeys[config_HotkeyIndex_Screenshot].str))
        {
            gui_action_save_screenshot(NULL);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Quit", config_hotkeys[config_HotkeyIndex_Quit].str))
        {
            application_trigger_quit();
        }

        ImGui::EndMenu();
    }
}

static void menu_emulator(void)
{
    if (ImGui::BeginMenu("Emulator"))
    {
        gui_in_use = true;

        if (ImGui::BeginMenu("Console Type"))
        {
            ImGui::PushItemWidth(120.0f);
            if (ImGui::Combo("##console_type", &config_emulator.console_type, "Auto\0Lynx I\0Lynx II\0\0"))
            {
                emu_force_console_type(config_emulator.console_type);
            }
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("BIOS"))
        {
            if (ImGui::MenuItem("Load BIOS..."))
            {
                open_bios = true;
            }
            ImGui::PushItemWidth(350);
            if (ImGui::InputText("##bios_path", gui_bios_path, IM_ARRAYSIZE(gui_bios_path), ImGuiInputTextFlags_AutoSelectAll))
            {
                config_emulator.bios_path.assign(gui_bios_path);
                gui_load_bios(gui_bios_path);
            }
            ImGui::PopItemWidth();

            ImGui::Separator();
            if (emu_get_core()->GetMedia()->IsBiosValid())
            {
                ImGui::TextColored(ImVec4(0.10f, 0.90f, 0.10f, 1.0f), "Valid BIOS");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.98f, 0.15f, 0.45f, 1.0f), "BIOS not loaded or invalid!");
                ImGui::TextColored(ImVec4(0.98f, 0.15f, 0.45f, 1.0f), "lynxboot.img recommended for most games.");
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Save States Dir"))
        {
            ImGui::PushItemWidth(220.0f);
            if (ImGui::Combo("##savestate_option", &config_emulator.savestates_dir_option, "Default Location\0Same as ROM\0Custom Location\0\0"))
            {
                update_savestates_data();
            }

            switch ((Directory_Location)config_emulator.savestates_dir_option)
            {
                case Directory_Location_Default:
                {
                    ImGui::Text("%s", config_root_path);
                    break;
                }
                case Directory_Location_ROM:
                {
                    if (!emu_is_empty())
                        ImGui::Text("%s", emu_get_core()->GetMedia()->GetFileDirectory());
                    break;
                }
                case Directory_Location_Custom:
                {
                    if (ImGui::MenuItem("Choose..."))
                    {
                        choose_savestates_path = true;
                    }

                    ImGui::PushItemWidth(450);
                    if (ImGui::InputText("##savestate_path", gui_savestates_path, IM_ARRAYSIZE(gui_savestates_path), ImGuiInputTextFlags_AutoSelectAll))
                    {
                        config_emulator.savestates_path.assign(gui_savestates_path);
                        update_savestates_data();
                    }
                    ImGui::PopItemWidth();
                    break;
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Savefiles Dir"))
        {
            ImGui::PushItemWidth(220.0f);
            ImGui::Combo("##savefiles_option", &config_emulator.savefiles_dir_option, "Default Location\0Same as ROM\0Custom Location\0\0");

            switch ((Directory_Location)config_emulator.savefiles_dir_option)
            {
                case Directory_Location_Default:
                {
                    ImGui::Text("%s", config_root_path);
                    break;
                }
                case Directory_Location_ROM:
                {
                    if (!emu_is_empty())
                        ImGui::Text("%s", emu_get_core()->GetMedia()->GetFileDirectory());
                    break;
                }
                case Directory_Location_Custom:
                {
                    if (ImGui::MenuItem("Choose..."))
                    {
                        choose_savefiles_path = true;
                    }

                    ImGui::PushItemWidth(450);
                    if (ImGui::InputText("##savefiles_path", gui_savefiles_path, IM_ARRAYSIZE(gui_savefiles_path), ImGuiInputTextFlags_AutoSelectAll))
                    {
                        config_emulator.savefiles_path.assign(gui_savefiles_path);
                    }
                    ImGui::PopItemWidth();
                    break;
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Screenshots Dir"))
        {
            ImGui::PushItemWidth(220.0f);
            ImGui::Combo("##screenshots_option", &config_emulator.screenshots_dir_option, "Default Location\0Same as ROM\0Custom Location\0\0");

            switch ((Directory_Location)config_emulator.screenshots_dir_option)
            {
                case Directory_Location_Default:
                {
                    ImGui::Text("%s", config_root_path);
                    break;
                }
                case Directory_Location_ROM:
                {
                    if (!emu_is_empty())
                        ImGui::Text("%s", emu_get_core()->GetMedia()->GetFileDirectory());
                    break;
                }
                case Directory_Location_Custom:
                {
                    if (ImGui::MenuItem("Choose..."))
                    {
                        choose_screenshots_path = true;
                    }

                    ImGui::PushItemWidth(450);
                    if (ImGui::InputText("##screenshots_path", gui_screenshots_path, IM_ARRAYSIZE(gui_screenshots_path), ImGuiInputTextFlags_AutoSelectAll))
                    {
                        config_emulator.screenshots_path.assign(gui_screenshots_path);
                    }
                    ImGui::PopItemWidth();
                    break;
                }
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::MenuItem("Show ROM info", "", &config_emulator.show_info);
        ImGui::MenuItem("Status Messages", "", &config_emulator.status_messages);

        ImGui::Separator();

        ImGui::MenuItem("Start Paused", "", &config_emulator.start_paused);
        ImGui::MenuItem("Pause When Inactive", "", &config_emulator.pause_when_inactive);

        ImGui::Separator();

        if (ImGui::BeginMenu("Hotkeys"))
        {
            hotkey_configuration_item("Open ROM:", &config_hotkeys[config_HotkeyIndex_OpenROM]);
            hotkey_configuration_item("Quit:", &config_hotkeys[config_HotkeyIndex_Quit]);
            hotkey_configuration_item("Reset:", &config_hotkeys[config_HotkeyIndex_Reset]);
            hotkey_configuration_item("Pause:", &config_hotkeys[config_HotkeyIndex_Pause]);
            hotkey_configuration_item("Fast Forward:", &config_hotkeys[config_HotkeyIndex_FFWD]);
            hotkey_configuration_item("Save State:", &config_hotkeys[config_HotkeyIndex_SaveState]);
            hotkey_configuration_item("Load State:", &config_hotkeys[config_HotkeyIndex_LoadState]);
            hotkey_configuration_item("Save State Slot 1:", &config_hotkeys[config_HotkeyIndex_SelectSlot1]);
            hotkey_configuration_item("Save State Slot 2:", &config_hotkeys[config_HotkeyIndex_SelectSlot2]);
            hotkey_configuration_item("Save State Slot 3:", &config_hotkeys[config_HotkeyIndex_SelectSlot3]);
            hotkey_configuration_item("Save State Slot 4:", &config_hotkeys[config_HotkeyIndex_SelectSlot4]);
            hotkey_configuration_item("Save State Slot 5:", &config_hotkeys[config_HotkeyIndex_SelectSlot5]);
            hotkey_configuration_item("Screenshot:", &config_hotkeys[config_HotkeyIndex_Screenshot]);
            hotkey_configuration_item("Fullscreen:", &config_hotkeys[config_HotkeyIndex_Fullscreen]);
            hotkey_configuration_item("Show Main Menu:", &config_hotkeys[config_HotkeyIndex_ShowMainMenu]);

            gui_popup_modal_hotkey();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug Hotkeys"))
        {
            hotkey_configuration_item("Reload ROM:", &config_hotkeys[config_HotkeyIndex_ReloadROM]);
            hotkey_configuration_item("Step Into:", &config_hotkeys[config_HotkeyIndex_DebugStepInto]);
            hotkey_configuration_item("Step Over:", &config_hotkeys[config_HotkeyIndex_DebugStepOver]);
            hotkey_configuration_item("Step Out:", &config_hotkeys[config_HotkeyIndex_DebugStepOut]);
            hotkey_configuration_item("Step Frame:", &config_hotkeys[config_HotkeyIndex_DebugStepFrame]);
            hotkey_configuration_item("Continue:", &config_hotkeys[config_HotkeyIndex_DebugContinue]);
            hotkey_configuration_item("Break:", &config_hotkeys[config_HotkeyIndex_DebugBreak]);
            hotkey_configuration_item("Run to Cursor:", &config_hotkeys[config_HotkeyIndex_DebugRunToCursor]);
            hotkey_configuration_item("Toggle Breakpoint:", &config_hotkeys[config_HotkeyIndex_DebugBreakpoint]);
            hotkey_configuration_item("Go Back:", &config_hotkeys[config_HotkeyIndex_DebugGoBack]);

            gui_popup_modal_hotkey();

            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::MenuItem("Single Instance", "", &config_debug.single_instance);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("RESTART REQUIRED");
            ImGui::NewLine();
            ImGui::Text("When enabled, opening a ROM while another instance is running");
            ImGui::Text("will send the ROM to the running instance instead of");
            ImGui::Text("starting a new one.");
            ImGui::EndTooltip();
        }

        ImGui::EndMenu();
    }
}

static void menu_video(void)
{
    if (ImGui::BeginMenu("Video"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("Full Screen", config_hotkeys[config_HotkeyIndex_Fullscreen].str, &config_emulator.fullscreen))
        {
            application_trigger_fullscreen(config_emulator.fullscreen);
        }

#if !defined(__APPLE__)
        if (ImGui::BeginMenu("Fullscreen Mode"))
        {
            ImGui::PushItemWidth(130.0f);
            ImGui::Combo("##fullscreen_mode", &config_emulator.fullscreen_mode, "Borderless\0Exclusive\0\0");
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }
#endif

        ImGui::Separator();

        ImGui::MenuItem("Always Show Menu", config_hotkeys[config_HotkeyIndex_ShowMainMenu].str, &config_emulator.always_show_menu);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("This option will enable menu even in fullscreen.");
            ImGui::Text("Menu always shows in debug mode.");
            ImGui::EndTooltip();
        }

        if (ImGui::MenuItem("Resize Window to Content"))
        {
            if (!config_debug.debug)
            {
                application_trigger_fit_to_content(gui_main_window_width, gui_main_window_height + gui_main_menu_height);
            }
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Scale"))
        {
            ImGui::PushItemWidth(250.0f);
            ImGui::Combo("##scale", &config_video.scale, "Integer Scale (Auto)\0Integer Scale (Manual)\0Scale to Window Height\0Scale to Window Width & Height\0\0");
            if (config_video.scale == 1)
                ImGui::SliderInt("##scale_manual", &config_video.scale_manual, 1, 10);
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Aspect Ratio"))
        {
            ImGui::PushItemWidth(190.0f);
            ImGui::Combo("##ratio", &config_video.ratio, "Square Pixels (1:1 PAR)\0Standard (4:3 DAR)\0Wide (16:9 DAR)\0Wide (16:10 DAR)\0\0");
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }


        if (ImGui::BeginMenu("Rotation"))
        {
            ImGui::PushItemWidth(120.0f);
            ImGui::Combo("##rotation", &config_video.rotation, "Auto\0Rotate LEFT\0Rotate RIGHT\0Disabled\0\0");
            emu_force_rotation(config_video.rotation);
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::MenuItem("Show FPS", "", &config_video.fps);

        if (ImGui::MenuItem("Vertical Sync", "", &config_video.sync))
        {
            display_set_vsync(config_video.sync);

            if (config_video.sync)
            {
                config_audio.sync = true;
                config_emulator.ffwd = false;
            }
        }

        ImGui::Separator();

        ImGui::MenuItem("Bilinear Filtering", "", &config_video.bilinear);

        if (ImGui::BeginMenu("Screen Ghosting"))
        {
            ImGui::MenuItem("Enable Screen Ghosting", "", &config_video.ghosting);
            ImGui::SliderInt("##screen_ghosting_history", &config_video.ghosting_history, 2, MAX_FRAME_HISTORY, "Frame History = %d");
            ImGui::SliderFloat("##screen_ghosting", &config_video.ghosting_intensity, 0.0f, 1.0f, "Trails = %.2f");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scanlines"))
        {
            ImGui::PushItemWidth(160.0f);
            ImGui::Combo("##scanlines_type", &config_video.scanlines_type, "Disabled\0Horizontal\0Vertical\0Grid\0\0");
            ImGui::SliderFloat("##scanlines_i", &config_video.scanlines_intensity, 0.0f, 1.0f, "Intensity = %.2f");
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Background Color"))
        {
            ImGui::ColorEdit3("##normal_bg", config_video.background_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float);
            ImGui::SameLine();
            ImGui::Text("Normal Background");

            ImGui::Separator();

            if (ImGui::ColorEdit3("##debugger_bg", config_video.background_color_debugger, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float))
            {
                ImGuiStyle& style = ImGui::GetStyle();
                style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(config_video.background_color_debugger[0], config_video.background_color_debugger[1], config_video.background_color_debugger[2], 1.0f);
            }
            ImGui::SameLine();
            ImGui::Text("Debugger Background");

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
}

static void menu_input(void)
{
    if (ImGui::BeginMenu("Input"))
    {
        gui_in_use = true;

        if (ImGui::BeginMenu("Keyboard"))
        {
            keyboard_configuration_item("Left:", &config_input.key_left);
            keyboard_configuration_item("Right:", &config_input.key_right);
            keyboard_configuration_item("Up:", &config_input.key_up);
            keyboard_configuration_item("Down:", &config_input.key_down);
            keyboard_configuration_item("A:", &config_input.key_A);
            keyboard_configuration_item("B:", &config_input.key_B);
            keyboard_configuration_item("Option 1:", &config_input.key_option1);
            keyboard_configuration_item("Pause:", &config_input.key_pause);
            keyboard_configuration_item("Option 2:", &config_input.key_option2);

            gui_popup_modal_keyboard();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Gamepads"))
        {
            ImGui::MenuItem("Enable Gamepad", "", &config_input.gamepad);

            if (ImGui::BeginMenu("Device"))
            {
                gamepad_device_selector();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Directional Controls"))
            {
                ImGui::PushItemWidth(150.0f);
                ImGui::Combo("##directional", &config_input.gamepad_directional, "D-pad\0Left Analog Stick\0\0");
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Button Configuration"))
            {
                gamepad_configuration_item("A:", &config_input.gamepad_A);
                gamepad_configuration_item("B:", &config_input.gamepad_B);
                gamepad_configuration_item("Option 1:", &config_input.gamepad_option1);
                gamepad_configuration_item("Pause:", &config_input.gamepad_pause);
                gamepad_configuration_item("Option 2:", &config_input.gamepad_option2);

                gui_popup_modal_gamepad();

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Shortcut Configuration"))
            {
                gamepad_configuration_item("Save State:", &config_input_gamepad_shortcuts.gamepad_shortcuts[config_HotkeyIndex_SaveState]);
                gamepad_configuration_item("Load State:", &config_input_gamepad_shortcuts.gamepad_shortcuts[config_HotkeyIndex_LoadState]);
                gamepad_configuration_item("Save State Slot 1:", &config_input_gamepad_shortcuts.gamepad_shortcuts[config_HotkeyIndex_SelectSlot1]);
                gamepad_configuration_item("Save State Slot 2:", &config_input_gamepad_shortcuts.gamepad_shortcuts[config_HotkeyIndex_SelectSlot2]);
                gamepad_configuration_item("Save State Slot 3:", &config_input_gamepad_shortcuts.gamepad_shortcuts[config_HotkeyIndex_SelectSlot3]);
                gamepad_configuration_item("Save State Slot 4:", &config_input_gamepad_shortcuts.gamepad_shortcuts[config_HotkeyIndex_SelectSlot4]);
                gamepad_configuration_item("Save State Slot 5:", &config_input_gamepad_shortcuts.gamepad_shortcuts[config_HotkeyIndex_SelectSlot5]);

                ImGui::Separator();

                gamepad_configuration_item("Reset:", &config_input_gamepad_shortcuts.gamepad_shortcuts[config_HotkeyIndex_Reset]);
                gamepad_configuration_item("Pause:", &config_input_gamepad_shortcuts.gamepad_shortcuts[config_HotkeyIndex_Pause]);
                gamepad_configuration_item("Fast Forward:", &config_input_gamepad_shortcuts.gamepad_shortcuts[config_HotkeyIndex_FFWD]);
                gamepad_configuration_item("Screenshot:", &config_input_gamepad_shortcuts.gamepad_shortcuts[config_HotkeyIndex_Screenshot]);

                gui_popup_modal_gamepad();

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
}

static void menu_audio(void)
{
    if (ImGui::BeginMenu("Audio"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("Enable Audio", "", &config_audio.enable))
        {
            emu_audio_mute(!config_audio.enable);
        }

        ImGui::Separator();

        for (int i = 0; i < 4; i++)
        {
            std::string label = "Channel " + std::to_string(i) + " Volume";
            if (ImGui::BeginMenu(label.c_str(), config_audio.enable))
            {
                ImGui::PushItemWidth(200.0f);
                if (ImGui::SliderFloat("##psg_volume", &config_audio.volume[i], 0.0f, 2.0f, "Volume = %.2f", ImGuiSliderFlags_AlwaysClamp))
                {
                    emu_audio_set_volume(i, config_audio.volume[i]);
                }
                ImGui::PopItemWidth();
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Anything above 1.00 may cause clipping.");
                    ImGui::EndTooltip();
                }
                ImGui::EndMenu();
            }
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Low-pass Filter", config_audio.enable))
        {
            ImGui::PushItemWidth(200.0f);
            if (SliderIntWithSteps("##psg_filter", &config_audio.lowpass_cutoff, 100, 5000, 10, "Cutoff = %d Hz"))
            {
                emu_audio_set_lowpass_cutoff((float)config_audio.lowpass_cutoff);
            }
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

#ifndef GLYNX_DISABLE_VGMRECORDER
        ImGui::Separator();

        bool is_recording = emu_is_vgm_recording();

        if (ImGui::MenuItem("Start VGM Recording...", "", false, !is_recording && !emu_is_empty()))
        {
            save_vgm = true;
        }

        if (ImGui::MenuItem("Stop VGM Recording", "", false, is_recording))
        {
            emu_stop_vgm_recording();
            gui_set_status_message("VGM recording stopped", 3000);
        }
#endif

        ImGui::EndMenu();
    }
}

static void menu_debug(void)
{
#if !defined(GLYNX_DISABLE_DISASSEMBLER)
    if (ImGui::BeginMenu("Debug"))
    {
        gui_in_use = true;

        ImGui::MenuItem("Enable", "", &config_debug.debug);

        ImGui::Separator();

        if (ImGui::MenuItem("Save Debug Settings...", "", false, config_debug.debug))
        {
            save_debug_settings = true;
        }

        if (ImGui::MenuItem("Load Debug Settings...", "", false, config_debug.debug))
        {
            load_debug_settings = true;
        }

        ImGui::MenuItem("Auto Save/Load Debug Settings", "", &config_debug.auto_debug_settings, config_debug.debug);

        ImGui::Separator();

        if (ImGui::MenuItem("Reload ROM", config_hotkeys[config_HotkeyIndex_ReloadROM].str, false, config_debug.debug && !emu_is_empty()))
        {
            gui_action_reload_rom();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("MCP Server", config_debug.debug))
        {
            bool mcp_running = emu_mcp_is_running();
            int transport_mode = emu_mcp_get_transport_mode();
            bool http_running = mcp_running && (transport_mode == 1);
            bool stdio_running = mcp_running && (transport_mode == 0);

            if (ImGui::MenuItem("Start HTTP Server", "", false, !mcp_running))
            {
                emu_mcp_set_transport(1, config_emulator.mcp_tcp_port);
                emu_mcp_start();
            }

            if (ImGui::MenuItem("Stop HTTP Server", "", false, http_running))
            {
                emu_mcp_stop();
            }

            ImGui::Separator();

            if (stdio_running)
                ImGui::TextColored(ImVec4(0.90f, 0.70f, 0.10f, 1.0f), "STDIO mode active");
            else if (http_running)
                ImGui::TextColored(ImVec4(0.10f, 0.90f, 0.10f, 1.0f), "Listening on %d", config_emulator.mcp_tcp_port);
            else
                ImGui::TextColored(ImVec4(0.98f, 0.15f, 0.45f, 1.0f), "Stopped");

            ImGui::Separator();

            ImGui::Text("HTTP Port:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(50);
            if (ImGui::InputInt("##mcp_port", &config_emulator.mcp_tcp_port, 0, 0))
            {
                if (config_emulator.mcp_tcp_port < 1)
                    config_emulator.mcp_tcp_port = 1;
                if (config_emulator.mcp_tcp_port > 65535)
                    config_emulator.mcp_tcp_port = 65535;
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::MenuItem("Show Output Screen", "", &config_debug.show_screen, config_debug.debug);

        if (ImGui::BeginMenu("Output Scale", config_debug.debug))
        {
            ImGui::PushItemWidth(200.0f);
            ImGui::SliderInt("##debug_scale", &config_debug.scale, 1, 10);
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::MenuItem("Show Disassembler", "", &config_debug.show_disassembler, config_debug.debug);
        ImGui::MenuItem("Show Memory Editor", "", &config_debug.show_memory, config_debug.debug);

        ImGui::Separator();

        if (ImGui::BeginMenu("Mikey", config_debug.debug))
        {
            ImGui::MenuItem("Show 65C02", "", &config_debug.show_processor);
            ImGui::MenuItem("Show 65C02 Call Stack", "", &config_debug.show_call_stack);
            ImGui::MenuItem("Show 65C02 Breakpoints", "", &config_debug.show_breakpoints);
            ImGui::MenuItem("Show 65C02 Symbols", "", &config_debug.show_symbols);
            ImGui::MenuItem("Show Registers", "", &config_debug.show_mikey_regs);
            ImGui::MenuItem("Show Timers", "", &config_debug.show_mikey_timers);
            ImGui::MenuItem("Show Color Registers", "", &config_debug.show_mikey_colors);
            ImGui::MenuItem("Show Audio", "", &config_debug.show_psg);
            ImGui::MenuItem("Show UART", "", &config_debug.show_uart);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Suzy", config_debug.debug))
        {
            ImGui::MenuItem("Show Registers", "", &config_debug.show_suzy_regs);
            ImGui::MenuItem("Show Math Registers", "", &config_debug.show_suzy_math_regs);
            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::MenuItem("Show Framebuffers", "", &config_debug.show_frame_buffers, config_debug.debug);
        ImGui::MenuItem("Show LCD / Video DMA", "", &config_debug.show_lcd, config_debug.debug);

        ImGui::Separator();

        ImGui::MenuItem("Show EEPROM", "", &config_debug.show_eeprom, config_debug.debug);
        ImGui::MenuItem("Show Cartridge", "", &config_debug.show_cart, config_debug.debug);

        ImGui::Separator();

        ImGui::MenuItem("Show Trace Logger", "", &config_debug.show_trace_logger, config_debug.debug);

#if defined(__APPLE__) || defined(_WIN32)
        ImGui::Separator();
        ImGui::MenuItem("Multi-Viewport", "", &config_debug.multi_viewport, config_debug.debug);
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("RESTART REQUIRED");
            ImGui::NewLine();
            ImGui::Text("Enables docking of debug windows outside of main window.");
            ImGui::EndTooltip();
        }
#endif

        ImGui::Separator();

        if (ImGui::BeginMenu("Font Size", config_debug.debug))
        {
            ImGui::PushItemWidth(110.0f);
            if (ImGui::Combo("##font", &config_debug.font_size, "Very Small\0Small\0Medium\0Large\0\0"))
            {
                gui_default_font = gui_default_fonts[config_debug.font_size];
            }
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
#endif
}

static void menu_about(void)
{
    if (ImGui::BeginMenu("About"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("About " GLYNX_TITLE " " GLYNX_VERSION " ..."))
        {
            open_about = true;
        }
        ImGui::EndMenu();
    }
}

static void file_dialogs(void)
{
    gui_file_dialog_process_results();

    if (open_rom || gui_shortcut_open_rom)
    {
        gui_shortcut_open_rom = false;
        gui_file_dialog_open_rom();
    }
    if (open_ram)
        gui_file_dialog_load_ram();
    if (save_ram)
        gui_file_dialog_save_ram();
    if (open_state)
        gui_file_dialog_load_state();
    if (save_state)
        gui_file_dialog_save_state();
    if (save_screenshot)
        gui_file_dialog_save_screenshot();
    if (save_vgm)
        gui_file_dialog_save_vgm();
    if (choose_savestates_path)
        gui_file_dialog_choose_savestate_path();
    if (choose_savefiles_path)
        gui_file_dialog_choose_savefile_path();
    if (choose_screenshots_path)
        gui_file_dialog_choose_screenshot_path();
    if (open_bios)
        gui_file_dialog_load_bios();
    if (save_debug_settings)
        gui_file_dialog_save_debug_settings();
    if (load_debug_settings)
        gui_file_dialog_load_debug_settings();
    if (open_about)
    {
        gui_dialog_in_use = true;
        ImGui::OpenPopup("About " GLYNX_TITLE);
    }
    if (open_bios_warning)
    {
        gui_dialog_in_use = true;
        ImGui::OpenPopup("BIOS");
    }

    gui_popup_modal_about();
    gui_popup_modal_bios();
}

static void keyboard_configuration_item(const char* text, SDL_Scancode* key)
{
    ImGui::Text("%s", text);
    ImGui::SameLine(100);

    char button_label[256];
    snprintf(button_label, 256, "%s##%s", SDL_GetKeyName(SDL_GetKeyFromScancode(*key, SDL_KMOD_NONE, false)), text);

    if (ImGui::Button(button_label, ImVec2(90,0)))
    {
        gui_configured_key = key;
        ImGui::OpenPopup("Keyboard Configuration");
    }

    ImGui::SameLine();

    char remove_label[256];
    snprintf(remove_label, sizeof(remove_label), "X##rk%s", text);

    if (ImGui::Button(remove_label))
    {
        *key = SDL_SCANCODE_UNKNOWN;
    }
}

static void gamepad_configuration_item(const char* text, int* button)
{
    ImGui::Text("%s", text);
    ImGui::SameLine(130);

    const char* button_name = "";

    if (*button == SDL_GAMEPAD_BUTTON_INVALID)
    {
        button_name = "";
    }
    else if (*button >= 0 && *button < SDL_GAMEPAD_BUTTON_COUNT)
    {
        static const char* gamepad_names[21] = {"A", "B", "X" ,"Y", "BACK", "GUIDE", "START", "L3", "R3", "L1", "R1", "UP", "DOWN", "LEFT", "RIGHT", "MISC", "PAD1", "PAD2", "PAD3", "PAD4", "TOUCH"};
        button_name = gamepad_names[*button];
    }
    else if (*button >= GAMEPAD_VBTN_AXIS_BASE)
    {
        int axis = *button - GAMEPAD_VBTN_AXIS_BASE;
        if (axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER)
            button_name = "L2";
        else if (axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
            button_name = "R2";
        else
            button_name = "??";
    }

    char button_label[256];
    snprintf(button_label, sizeof(button_label), "%s##%s", button_name, text);

    if (ImGui::Button(button_label, ImVec2(70,0)))
    {
        gui_configured_button = button;
        ImGui::OpenPopup("Gamepad Configuration");
    }

    ImGui::SameLine();

    char remove_label[256];
    snprintf(remove_label, sizeof(remove_label), "X##rg%s", text);

    if (ImGui::Button(remove_label))
    {
        *button = SDL_GAMEPAD_BUTTON_INVALID;
    }
}

static void hotkey_configuration_item(const char* text, config_Hotkey* hotkey)
{
    ImGui::Text("%s", text);
    ImGui::SameLine(150);

    char button_label[256];
    snprintf(button_label, sizeof(button_label), "%s##%s", hotkey->str[0] != '\0' ? hotkey->str : "<None>", text);

    if (ImGui::Button(button_label, ImVec2(150,0)))
    {
        gui_configured_hotkey = hotkey;
        ImGui::OpenPopup("Hotkey Configuration");
    }

    ImGui::SameLine();

    char remove_label[256];
    snprintf(remove_label, sizeof(remove_label), "X##rh%s", text);

    if (ImGui::Button(remove_label))
    {
        hotkey->key = SDL_SCANCODE_UNKNOWN;
        hotkey->mod = SDL_KMOD_NONE;
        config_update_hotkey_string(hotkey);
    }
}

static void gamepad_device_selector(void)
{
    Gamepad_Detected_Info info;
    gamepad_get_detected(&info);

    std::string items;
    items.reserve(4096);
    items.append("<None>");
    items.push_back('\0');

    for (int i = 0; i < info.count; i++)
    {
        items.append(info.labels[i]);
        items.push_back('\0');
    }
    items.push_back('\0');

    int selected = (info.current_index >= 0) ? info.current_index + 1 : 0;

    if (ImGui::Combo("##device_player", &selected, items.c_str()))
    {
        SDL_JoystickID instance_id = (selected > 0) ? info.ids[selected - 1] : 0;
        gamepad_assign(instance_id);
    }
}

static void draw_savestate_slot_info(int slot)
{
    if (emu_savestates[slot].rom_name[0] != 0)
    {
        if (emu_savestates[slot].version != GLYNX_SAVESTATE_VERSION)
        {
            ImGui::TextColored(ImVec4(0.98f, 0.15f, 0.45f, 1.0f), "This savestate is from an older version and will not work" );
            if (emu_savestates[slot].emu_build[0] != 0)
                ImGui::TextColored(ImVec4(0.98f, 0.15f, 0.45f, 1.0f), "Use %s - %s", GLYNX_TITLE, emu_savestates[slot].emu_build);
            ImGui::Separator();
        }

        ImGui::Text("%s", emu_savestates[slot].rom_name);
        char date[64];
        get_date_time_string(emu_savestates[slot].timestamp, date, sizeof(date));
        ImGui::Text("%s", date);

        if (IsValidPointer(emu_savestates_screenshots[slot].data))
        {
            float width = (float)emu_savestates_screenshots[slot].width;
            float height = (float)emu_savestates_screenshots[slot].height;
            ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_savestates, ImVec2(width, height), ImVec2(0, 0), ImVec2(width / 256.0f, height / 256.0f));
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.0f), "Slot %d is empty", slot + 1);
    }
}
