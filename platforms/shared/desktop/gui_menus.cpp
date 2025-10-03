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
#include "config.h"
#include "application.h"
#include "emu.h"
#include "renderer.h"
#include "utils.h"
#include "gearlynx.h"

static bool open_rom = false;
static bool open_ram = false;
static bool save_ram = false;
static bool open_state = false;
static bool save_state = false;
static bool open_about = false;
static bool save_screenshot = false;
static bool choose_savestates_path = false;
static bool choose_screenshots_path = false;
static bool open_bios = false;
static bool open_bios_warning = false;

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
    choose_savestates_path = false;
    choose_screenshots_path = false;
    open_bios = false;
    open_bios_warning = false;
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

        if (ImGui::MenuItem("Open ROM...", "Ctrl+O"))
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
        
        if (ImGui::MenuItem("Reset", "Ctrl+R"))
        {
            gui_action_reset();
        }

        if (ImGui::MenuItem("Pause", "Ctrl+P", &config_emulator.paused))
        {
            gui_action_pause();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Fast Forward", "Ctrl+F", &config_emulator.ffwd))
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

        if (ImGui::MenuItem("Save RAM As..."))
        {
            save_ram = true;
        }

        if (ImGui::MenuItem("Load RAM From..."))
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

        if (ImGui::MenuItem("Save State", "Ctrl+S")) 
        {
            std::string message("Saving state to slot ");
            message += std::to_string(config_emulator.save_slot + 1);
            gui_set_status_message(message.c_str(), 3000);
            emu_save_state_slot(config_emulator.save_slot + 1);
        }

        if (ImGui::MenuItem("Load State", "Ctrl+L"))
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

        if (ImGui::MenuItem("Save Screenshot", "Ctrl+X"))
        {
            gui_action_save_screenshot(NULL);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Quit", "Ctrl+Q"))
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
/*

        if (ImGui::BeginMenu("Backup RAM Dir"))
        {
            ImGui::PushItemWidth(220.0f);
            ImGui::Combo("##backup_ram_option", &config_emulator.backup_ram_dir_option, "Default Location\0Same as ROM\0Custom Location\0\0");

            switch ((Directory_Location)config_emulator.backup_ram_dir_option)
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
                        choose_backup_ram_path = true;
                    }

                    ImGui::PushItemWidth(450);
                    if (ImGui::InputText("##backup_ram_path", gui_backup_ram_path, IM_ARRAYSIZE(gui_backup_ram_path), ImGuiInputTextFlags_AutoSelectAll))
                    {
                        config_emulator.backup_ram_path.assign(gui_backup_ram_path);
                    }
                    ImGui::PopItemWidth();
                    break;
                }
            }

            ImGui::EndMenu();
        }
*/
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

        ImGui::EndMenu();
    }
}

static void menu_video(void)
{
    if (ImGui::BeginMenu("Video"))
    {
        gui_in_use = true;

        if (ImGui::MenuItem("Full Screen", "F12", &config_emulator.fullscreen))
        {
            application_trigger_fullscreen(config_emulator.fullscreen);
        }

        ImGui::MenuItem("Always Show Menu", "CTRL+M", &config_emulator.always_show_menu);
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

        ImGui::Separator();

        ImGui::MenuItem("Show FPS", "", &config_video.fps);

        ImGui::Separator();

        ImGui::MenuItem("Bilinear Filtering", "", &config_video.bilinear);

        if (ImGui::BeginMenu("Screen Ghosting"))
        {
            ImGui::MenuItem("Enable Screen Ghosting", "", &config_video.mix_frames);
            ImGui::SliderFloat("##screen_ghosting", &config_video.mix_frames_intensity, 0.0f, 1.0f, "Intensity = %.2f");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scanlines"))
        {
            ImGui::PushItemWidth(160.0f);
            ImGui::Combo("##scanlines_type", &config_video.scanlines_type, "Disabled\0Horizontal\0Vertical\0Grid\0\0");
            ImGui::MenuItem("Enable Scanlines Filter", "", &config_video.scanlines_filter);
            ImGui::SliderFloat("##scanlines_i", &config_video.scanlines_intensity, 0.0f, 1.0f, "Intensity = %.2f");
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
            keyboard_configuration_item("Pause:", &config_input.key_pause);
            keyboard_configuration_item("Option 1:", &config_input.key_option1);
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
                gamepad_configuration_item("Pause:", &config_input.gamepad_pause);
                gamepad_configuration_item("Option 1:", &config_input.gamepad_option1);
                gamepad_configuration_item("Option 2:", &config_input.gamepad_option2);

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
                emu_audio_set_lowpass_cutoff(config_audio.lowpass_cutoff);
            }
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

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

        ImGui::MenuItem("Show Output Screen", "", &config_debug.show_screen, config_debug.debug);
        ImGui::MenuItem("Show Disassembler", "", &config_debug.show_disassembler, config_debug.debug);
        ImGui::MenuItem("Show Memory Editor", "", &config_debug.show_memory, config_debug.debug);

        ImGui::Separator();

        ImGui::MenuItem("Show Mikey 65C02", "", &config_debug.show_processor, config_debug.debug);
        ImGui::MenuItem("Show Mikey 65C02 Call Stack", "", &config_debug.show_call_stack, config_debug.debug);
        ImGui::MenuItem("Show Mikey Registers", "", &config_debug.show_mikey_regs, config_debug.debug);
        ImGui::MenuItem("Show Mikey Timers", "", &config_debug.show_mikey_timers, config_debug.debug);
        ImGui::MenuItem("Show Mikey Color Registers", "", &config_debug.show_mikey_colors, config_debug.debug);
        ImGui::MenuItem("Show Mikey Audio", "", &config_debug.show_psg, config_debug.debug);

        ImGui::Separator();

        ImGui::MenuItem("Show Suzy Registers", "", &config_debug.show_suzy_regs, config_debug.debug);
        ImGui::MenuItem("Show Suzy Math Registers", "", &config_debug.show_suzy_math_regs, config_debug.debug);
        ImGui::MenuItem("Show Framebuffers", "", &config_debug.show_frame_buffers, config_debug.debug);

        ImGui::Separator();

        ImGui::MenuItem("Show Trace Logger", "", &config_debug.show_trace_logger, config_debug.debug);

#if defined(__APPLE__) || defined(_WIN32)
        ImGui::Separator();
        ImGui::MenuItem("Multi-Viewport (Restart required)", "", &config_debug.multi_viewport, config_debug.debug);
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
    if (choose_savestates_path)
        gui_file_dialog_choose_savestate_path();
    if (choose_screenshots_path)
        gui_file_dialog_choose_screenshot_path();
    if (open_bios)
        gui_file_dialog_load_bios();
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
    snprintf(button_label, 256, "%s##%s", SDL_GetKeyName(SDL_GetKeyFromScancode(*key)), text);

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
    ImGui::SameLine(120);

    const char* button_name = "";

    if (*button == SDL_CONTROLLER_BUTTON_INVALID)
    {
        button_name = "";
    }
    else if (*button >= 0 && *button < SDL_CONTROLLER_BUTTON_MAX)
    {
        static const char* gamepad_names[21] = {"A", "B", "X" ,"Y", "BACK", "GUIDE", "START", "L3", "R3", "L1", "R1", "UP", "DOWN", "LEFT", "RIGHT", "MISC", "PAD1", "PAD2", "PAD3", "PAD4", "TOUCH"};
        button_name = gamepad_names[*button];
    }
    else if (*button >= GAMEPAD_VBTN_AXIS_BASE)
    {
        int axis = *button - GAMEPAD_VBTN_AXIS_BASE;
        if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
            button_name = "L2";
        else if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
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
        *button = SDL_CONTROLLER_BUTTON_INVALID;
    }
}

static void gamepad_device_selector(void)
{
    const int max_detected_gamepads = 32;
    int index_map[max_detected_gamepads];
    index_map[0] = -1;
    int count = 1;

    std::string items;
    items.reserve(4096);
    items.append("<None>");
    items.push_back('\0');

    int num = SDL_NumJoysticks();

    SDL_JoystickID current_id = -1;
    if (IsValidPointer(application_gamepad))
        current_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(application_gamepad));

    int selected = 0;

    for (int i = 0; i < num && count < max_detected_gamepads; i++)
    {
        if (!SDL_IsGameController(i))
            continue;

        const char* name = SDL_GameControllerNameForIndex(i);
        if (!IsValidPointer(name))
            name = "Unknown Gamepad";

        index_map[count] = i;

        SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(i);

        if (current_id == id)
            selected = count;

        char id_str[64];
        SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(i);
        SDL_JoystickGetGUIDString(guid, id_str, sizeof(id_str));
        size_t len = strlen(id_str);
        const char* id_8 = id_str + (len > 8 ? len - 8 : 0);

        char label[192];
        snprintf(label, sizeof(label), "%s (ID: %s)", name, id_8);

        items.append(label);
        items.push_back('\0');
        count++;
    }

    items.push_back('\0');

    if (ImGui::Combo("##device_player", &selected, items.c_str()))
    {
        int device_index = index_map[selected];
        application_assign_gamepad(device_index);
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
            ImGui::Image((ImTextureID)(intptr_t)renderer_emu_savestates, ImVec2((height / 3.0f) * 4.0f, height), ImVec2(0, 0), ImVec2(width / 2048.0f, height / 256.0f));
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.0f), "Slot %d is empty", slot + 1);
    }
}
