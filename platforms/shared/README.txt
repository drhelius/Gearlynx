   ____                 _                  
  / ___| ___  __ _ _ __| |_   _ _ __ __  __
 | |  _ / _ \/ _` | '__| | | | | '_ \\ \/ /
 | |_| |  __/ (_| | |  | | |_| | | | |>  < 
  \____|\___|\__,_|_|  |_|\__, |_| |_/_/\_\
                          |___/            

-----------------------------------------------------

Gearlynx is a very accurate cross-platform Atari Lynx emulator written in C++ 
that runs on Windows, macOS, Linux, BSD and RetroArch, with an embedded MCP 
server for debugging and tooling.

This is an open source project with its ongoing development made possible 
thanks to the support by awesome backers. If you find it useful, please 
consider sponsoring at https://github.com/sponsors/drhelius

Instructions and tips at: https://github.com/drhelius/Gearlynx
Follow me on Twitter for updates: http://twitter.com/drhelius

-----------------------------------------------------
Downloads:
-----------------------------------------------------

Latest release: https://github.com/drhelius/Gearlynx/releases

Windows:
  - Desktop x64 and ARM64
  - MCPB x64 and ARM64
  - May need Visual C++ Redistributable and OpenGL Compatibility Pack

macOS:
  - Homebrew: brew install --cask drhelius/geardome/gearlynx
  - Desktop Apple Silicon and Intel
  - MCPB x64 and ARM64

Linux:
  - Desktop Ubuntu 24.04 and 22.04 (x64 and ARM64)
  - MCPB x64 and ARM64
  - May need libsdl3

RetroArch:
  - Libretro core available for all platforms
  - Documentation: https://docs.libretro.com/library/gearlynx/

Dev Builds:
  - GitHub Actions: https://github.com/drhelius/Gearlynx/actions/workflows/gearlynx.yml

-----------------------------------------------------
Features:
-----------------------------------------------------

- Very accurate emulation supporting the entire commercial Atari Lynx catalog
  and most homebrew and demos
- User selectable Lynx I or Lynx II emulation
- Compressed and homebrew rom support (lnx, lyx, o, and zip)
- Save states with preview
- Very accurate audio emulation with configurable low-pass filter (mimics
  original Lynx audio hardware)
- VGM recorder
- Internal database for automatic rom detection and hardware selection if
  Auto options are selected
- Bank switching (BANK1 + AUDIN) and EEPROM
- Save files (EEPROM and NVRAM)
- Supported platforms (standalone): Windows, Linux, BSD and macOS
- Supported platforms (libretro): Windows, Linux, macOS, Raspberry Pi, Android,
  iOS, tvOS, PlayStation Vita, PlayStation 3, Nintendo 3DS, Nintendo GameCube,
  Nintendo Wii, Nintendo WiiU, Nintendo Switch, Emscripten, Classic Mini systems
  (NES, SNES, C64, ...), OpenDingux, RetroFW and QNX
- Full debugger with just-in-time run-ahead disassembler that can handle
  self-modifying code, CPU and memory breakpoints, code navigation (goto address,
  JP JR and JSR double clicking), debug symbols, automatic labels, memory editor,
  memory search, Suzy and Mikey register viewer, audio channels, UART, cartridge
  and video inspector
- MCP server for AI-assisted debugging with GitHub Copilot, Claude, ChatGPT and
  similar, exposing tools for execution control, memory inspection, hardware
  status, and more
- Windows and Linux Portable Mode
- ROM loading from the command line by adding the ROM path as an argument
- ROM loading using drag & drop
- Support for modern game controllers through gamecontrollerdb.txt file located
  in the same directory as the application binary

-----------------------------------------------------
Tips:
-----------------------------------------------------

Basic Usage:
  - BIOS: Gearlynx requires a BIOS to work. It is possible to load any BIOS but
    the original with md5 fcd403db69f54290b51035d82f835e7b is recommended
  - Mouse Cursor: Automatically hides when hovering over the main output window
    or when Main Menu is disabled
  - Portable Mode: Create an empty file named portable.ini in the same directory
    as the application binary to enable portable mode

Debugging Features:
  - Docking Windows: In debug mode, you can dock windows together by pressing
    SHIFT and dragging a window onto another
  - Multi-viewport: In Windows or macOS, you can enable "multi-viewport" in the
    debug menu. You must restart the emulator for the change to take effect.
    Once enabled, you can drag debugger windows outside the main window
  - Debug Symbols: The emulator automatically tries to load a symbol (.sym) file
    when loading a ROM. It supports cc65 (VICE label file), lyxass (EQU) and
    mads (lab and hea) file formats

MCP Server:
  - Gearlynx includes a Model Context Protocol (MCP) server that enables
    AI-assisted debugging through AI agents like GitHub Copilot, Claude, ChatGPT
    and similar. The server provides tools for execution control, memory
    inspection, breakpoints, disassembly, hardware status, sprite viewing, and
    more. See MCP_README.md for complete setup instructions

Command Line Usage:
  gearlynx [options] [game_file] [symbol_file]

  Arguments:
    [game_file]              Game file: accepts ROMs (.pce, .sgx, .hes), 
                             CUE (.cue) or ZIP (.zip)
    [symbol_file]            Optional symbol file for debugging

  Options:
    -f, --fullscreen         Start in fullscreen mode
    -w, --windowed           Start in windowed mode with menu visible
        --mcp-stdio          Auto-start MCP server with stdio transport
        --mcp-http           Auto-start MCP server with HTTP transport
        --mcp-http-port N    HTTP port for MCP server (default: 7777)
    -v, --version            Display version information
    -h, --help               Display this help message

-----------------------------------------------------
License:
-----------------------------------------------------

Gearlynx is licensed under the GNU General Public License v3.0 License.

Copyright (C) 2024 Ignacio Sanchez

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see http://www.gnu.org/licenses/
