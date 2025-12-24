# Gearlynx MCP Server

A [Model Context Protocol](https://modelcontextprotocol.io/introduction) server for the Gearlynx emulator, enabling AI-assisted debugging and development of Atari Lynx games.

This server provides tools for game development, rom hacking, reverse engineering, and debugging through standardized MCP protocols compatible with AI agents like GitHub Copilot, Claude, ChatGPT and others.

## Features

- **Full Debugger Access**: 6502 CPU registers, memory inspection, breakpoints, and execution control
- **Multiple Memory Areas**: Access RAM, Zero Page, Stack, Cartridge ROM, and BIOS
- **Disassembly**: View disassembled 6502 code for any address range
- **Hardware Inspection**: 6502 CPU, Mikey (timers, audio, display), Suzy (sprites, math), UART (ComLynx)
- **Frame Buffer Capture**: Capture VIDBAS (Suzy) and DISPADR (Mikey) frame buffers
- **Symbol Support**: Add, remove, load, and list debug symbols
- **Bookmarks**: Memory and disassembler bookmarks for navigation
- **Call Stack**: View function call hierarchy
- **Screenshot Capture**: Get current frame as PNG image
- **Documentation Resources**: Built-in hardware and programming documentation for AI context
- **GUI Integration**: MCP server runs alongside the emulator GUI, sharing the same state

## Transport Modes

The Gearlynx MCP server supports two transport modes:

### STDIO Transport (Recommended)

The default mode uses standard input/output for communication. The emulator is launched by the AI client and communicates through stdin/stdout pipes.

### HTTP Transport

The HTTP transport mode runs the emulator with an embedded web server on `localhost:7777/mcp`. The emulator stays running independently while the AI client connects via HTTP.

## Quick Start

### STDIO Mode with VS Code

1. **Install [GitHub Copilot extension](https://code.visualstudio.com/docs/copilot/overview)** in VS Code

2. **Configure VS Code settings**:

   Add to your workspace folder a file named `.vscode/mcp.json` with:

   ```json
   {
     "servers": {
       "gearlynx": {
         "command": "/path/to/gearlynx",
         "args": ["--mcp-stdio"]
       }
     }
   }
   ```

   **Important:** Update the `command` path to match your build location:
   - **macOS:** `/path/to/gearlynx`
   - **Linux:** `/path/to/gearlynx`
   - **Windows:** `C:/path/to/gearlynx.exe`

3. **Restart VS Code** may be necessary for settings to take effect

4. **Open GitHub Copilot Chat** and start debugging:
   - The emulator will auto-start with MCP server enabled
   - Load a game ROM
   - Start chatting with Copilot about the game state
   - You can add context from "MCP Resources" if needed

### STDIO Mode with Claude Desktop

#### Option 1: Desktop Extension (Recommended)

The easiest way to install Gearlynx MCP server on Claude Desktop is using the MCPB package:

1. **Download the latest MCPB package** for your platform from the [releases page](https://github.com/drhelius/gearlynx/releases).

2. **Install the extension**:
   - Open Claude Desktop
   - Navigate to **Settings > Extensions**
   - Click **Advanced settings**
   - In the Extension Developer section, click **Install Extension…**
   - Select the downloaded `.mcpb` file

3. **Start debugging**: The extension is now available in your conversations. The emulator will automatically launch when the tool is enabled.

#### Option 2: Manual Configuration

If you prefer to build from source or configure manually:

1. **Edit Claude Desktop config file**:

   Follow [these instructions](https://modelcontextprotocol.io/quickstart/user#for-claude-desktop-users) to access Claude's config file, then edit it to include:

   ```json
   {
     "mcpServers": {
       "gearlynx": {
         "command": "/path/to/gearlynx/platforms/macos/gearlynx",
         "args": ["--mcp-stdio"]
       }
     }
   }
   ```

   **Config file locations:**
   - **macOS:** `~/Library/Application Support/Claude/claude_desktop_config.json`
   - **Windows:** `%APPDATA%\Claude\claude_desktop_config.json`
   - **Linux:** `~/.config/Claude/claude_desktop_config.json`

   **Important:** Update the `command` path to match your build location.

2. **Restart Claude Desktop**

### HTTP Mode

1. **Start the emulator manually** with HTTP transport:
   ```bash
   ./gearlynx --mcp-http
   # Server will start on http://localhost:7777/mcp
   
   # Or specify a custom port:
   ./gearlynx --mcp-http --mcp-http-port 3000
   # Server will start on http://localhost:3000/mcp
   ```

   You can optionally start the server using the "MCP" menu in the GUI.

2. **Configure VS Code** `.vscode/mcp.json`:
   ```json
   {
     "servers": {
       "gearlynx": {
         "type": "http",
         "url": "http://localhost:7777/mcp",
         "headers": {}
       }
     }
   }
   ```

3. **Or configure Claude Desktop**:
   ```json
   {
     "mcpServers": {
       "gearlynx": {
         "type": "http",
         "url": "http://localhost:7777/mcp"
       }
     }
   }
   ```

4. **Restart your AI client** and start debugging

> **Note:** The MCP HTTP Server must be running standalone before connecting the AI client.

## Usage Examples

Once configured, you can ask your AI assistant:

### Basic Commands

- "What game is currently loaded?"
- "Load the ROM at /path/to/game.lnx"
- "Show me the current CPU registers"
- "Read 16 bytes from RAM starting at 0x0000"
- "Set a breakpoint at address 0x8000"
- "Pause execution and show me the Mikey timers"
- "Step through the next 5 instructions"
- "Capture a screenshot of the current frame"
- "Tap the A button on the controller"
- "Show me the Suzy registers"

### Advanced Debugging Workflows

- "Find the VBlank interrupt handler, analyze what it does, and add symbols for all the subroutines it calls"

- "Locate the sprite update routine. Study how this game manages its sprite system, explain the algorithm, and add bookmarks to key sections. Also add watches for any sprite-related variables you find"

- "There's a data decompression routine around address 0xC000. Step through it instruction by instruction, reverse engineer the compression algorithm, and explain how it works with examples"

- "Find where the game stores its level data in ROM. Analyze the data structure format, create a memory map showing each section, and add symbols for the data tables"

- "The game is rendering corrupted graphics. Examine the Suzy registers, check the VIDBAS frame buffer, inspect the Mikey display settings, and diagnose what's causing the corruption. Set up watches on relevant memory addresses"

## Available MCP Tools

The server exposes tools organized in the following categories:

### Execution Control
- `debug_pause` - Pause emulation
- `debug_continue` - Resume emulation
- `debug_step_into` - Step into next 6502 instruction (enters subroutines)
- `debug_step_over` - Step over next 6502 instruction (skips subroutines like JSR)
- `debug_step_out` - Step out of current subroutine (continues until RTS/RTI)
- `debug_step_frame` - Step one video frame (executes until next VBLANK)
- `debug_run_to_cursor` - Continue execution until reaching specified address
- `debug_reset` - Reset the Atari Lynx emulated system
- `debug_get_status` - Get debugger status (paused, at_breakpoint, pc address)

### CPU & Registers
- `get_6502_status` - Get 6502 CPU status (registers, flags, interrupts, memory map visibility based on MAPCTL)
- `write_6502_register` - Write to a 6502 CPU register (PC, A, X, Y, S, P)

### Memory Operations
- `list_memory_areas` - List memory editor tabs (RAM, Zero Page, Stack, Cartridge, BIOS) with CPU address ranges
- `read_memory` - Read from specific memory area
- `write_memory` - Write to specific memory area
- `get_memory_selection` - Get current memory selection range
- `select_memory_range` - Select a range of memory addresses
- `set_memory_selection_value` - Set all bytes in selection to specified value
- `add_memory_bookmark` - Add bookmark in memory area
- `remove_memory_bookmark` - Remove memory bookmark
- `list_memory_bookmarks` - List all bookmarks in memory area
- `add_memory_watch` - Add watch (tracked memory location)
- `remove_memory_watch` - Remove memory watch
- `list_memory_watches` - List all watches in memory area
- `memory_search_capture` - Capture memory snapshot for search comparison
- `memory_search` - Search memory with operators (<, >, ==, !=, <=, >=), compare types (previous, value, address), and data types (hex, signed, unsigned)

### Disassembly & Debugging
- `get_disassembly` - Get disassembled 6502 code for specified address range (only executed code is available)
- `add_symbol` - Add symbol (label) at specified address
- `remove_symbol` - Remove symbol
- `list_symbols` - List all defined symbols
- `load_symbols` - Load debug symbols from file (.sym format with 'ADDRESS LABEL' entries)
- `add_disassembler_bookmark` - Add bookmark in disassembler
- `remove_disassembler_bookmark` - Remove disassembler bookmark
- `list_disassembler_bookmarks` - List all disassembler bookmarks
- `get_call_stack` - View function call hierarchy

### Breakpoints
- `set_breakpoint` - Set execution, read, or write breakpoint at address. Read/write breakpoints stop with PC at instruction after memory access
- `set_breakpoint_range` - Set breakpoint for an address range (exec, read, or write)
- `remove_breakpoint` - Remove breakpoint (single address or range)
- `list_breakpoints` - List all breakpoints
- `set_breakpoint_on_irq` - Set breakpoint on specific IRQ (Timer0-7)
- `clear_breakpoint_on_irq` - Clear IRQ breakpoint
- `list_breakpoints_on_irq` - List which IRQs have breakpoints set

### Hardware Status
- `get_mikey_registers` - Get all Mikey registers ($FD00-$FDFF) or filter by specific address
- `write_mikey_register` - Write to a Mikey register
- `get_mikey_timers` - Get timer status (Timer 0-7: HBLANK, VBLANK, UART, etc.)
- `get_mikey_audio` - Get audio channel status (Channel 0-3)
- `get_suzy_registers` - Get all Suzy registers ($FC00-$FCFF) or filter by specific address
- `write_suzy_register` - Write to a Suzy register
- `get_uart_status` - Get UART (ComLynx) status

### Screen Capture
- `get_screenshot` - Capture current screen frame as base64 PNG
- `get_frame_buffer` - Capture debug frame buffer as base64 PNG (VIDBAS from Suzy or DISPADR from Mikey)

### Media & State Management
- `get_media_info` - Get loaded ROM info (file path, type, size, CRC, rotation, EEPROM, BIOS status)
- `load_media` - Load ROM file (.lnx, .lyx, .o, .zip). Automatically loads .sym symbol file if present
- `list_save_state_slots` - List all 5 save state slots with information (rom name, timestamp, screenshot availability)
- `select_save_state_slot` - Select active save state slot (1-5) for save/load operations
- `save_state` - Save emulator state to currently selected slot
- `load_state` - Load emulator state from currently selected slot
- `set_fast_forward_speed` - Set fast forward speed multiplier (0: 1.5x, 1: 2x, 2: 2.5x, 3: 3x, 4: Unlimited)
- `toggle_fast_forward` - Toggle fast forward mode on/off

### Controller Input
- `controller_button` - Control a button on the Lynx controller. Use action 'press' to hold, 'release' to let go, or 'press_and_release' for a quick tap. Buttons: up, down, left, right, a, b, option1, option2, pause

## Available MCP Resources

In addition to tools, the MCP server provides documentation resources that AI assistants can access to better understand the Atari Lynx hardware and programming.

MCP clients usually offer resources in the "Add context..." section of the chat interface. You may need to manually add them when you think they are relevant.

### Hardware Documentation Resources

Complete technical reference documentation for Atari Lynx hardware components:

- **Chapter 1: General Overview** (`gearlynx://hardware/lynx1_general_overview`) - Marketing Feature Set and External Specification
- **Chapter 2: Hardware Overview** (`gearlynx://hardware/lynx2_hardware_overview`) - System components overview including CPU, Mikey, and Suzy
- **Chapter 3: Software Related Hardware Perniciousness** (`gearlynx://hardware/lynx3_software_hardware_perniciousness`) - Important notes about hardware quirks and issues
- **Chapter 4-5: CPU/ROM** (`gearlynx://hardware/lynx4_cpu_rom`) - CPU cycle timing, ROM, and CPU Sleep documentation
- **Chapter 6: Display** (`gearlynx://hardware/lynx5_display`) - Frame Rate, Pen Numbers/Color Palette, Display Buffers
- **Chapter 7: Sprite/Collision** (`gearlynx://hardware/lynx6_sprite_collision`) - Sprite features, data structure, and engine documentation
- **Chapter 8-10: Audio, Magnetic Tape, ROM Cart** (`gearlynx://hardware/lynx7_audio_tape_romcart`) - Audio system, magnetic tape interface, and ROM cartridge
- **Chapter 11: Timers/Interrupts** (`gearlynx://hardware/lynx8_timers_interrupts`) - Timer system and interrupt handling
- **Chapter 11a: UART** (`gearlynx://hardware/lynx8a_uart`) - UART connector signals, baud rate, and data format
- **Chapter 12-13: Other Hardware Features** (`gearlynx://hardware/lynx9_other_hardware`) - Multiply/Divide hardware, Parallel Port, Expansion Connector
- **Chapter 14-19: System Reset and Related Topics** (`gearlynx://hardware/lynx10_system_reset`) - System Reset/Power Up, Definitions, Variances, Exemptions, Common Errors, Savegame
- **Appendix 1: System Bus Interplay** (`gearlynx://hardware/lynx11_system_bus_interplay`) - System bus architecture and interactions
- **Appendix 2: Interrupts and CPU Sleep** (`gearlynx://hardware/irq_interrupts_cpu_sleep`) - Handy interrupts, CPU sleep mode, and debugging
- **Appendix 3: Hardware Addresses** (`gearlynx://hardware/hardware_addresses`) - Complete memory map and register documentation for Mikey and Suzy
- **Appendix 4: Sprite Engine** (`gearlynx://hardware/sprite_engine`) - Display creation, double-buffering, and sprite positioning

## How MCP Works in Gearlynx

- The MCP server runs **alongside** the GUI in a background thread
- The emulator GUI remains fully functional (you can play/debug normally while using MCP)
- Commands from the AI are queued and executed on the GUI thread
- Both GUI and MCP share the same emulator state
- Changes made through MCP are instantly reflected in the GUI and vice versa

## Architecture

### STDIO Transport
```
┌─────────────────┐                    ┌──────────────────┐
│   VS Code /     │       stdio        │     Gearlynx     │
│ Claude Desktop  │◄──────────────────►│    MCP Server    │
│   (AI Client)   │       pipes        │   (background)   │
└─────────────────┘                    └──────────────────┘
        │                                       │
        └───► Launches ►────────────────────────┘
                                                │
                                                │ Shared State
                                                ▼
                                       ┌──────────────────┐
                                       │   Emulator Core  │
                                       │   + GUI Window   │
                                       └──────────────────┘
```

### HTTP Transport
```
┌─────────────────┐                    ┌──────────────────┐
│   VS Code /     │  HTTP (port 7777)  │     Gearlynx     │
│ Claude Desktop  │◄──────────────────►│ MCP HTTP Server  │
│   (AI Client)   │                    │    (listener)    │
└─────────────────┘                    └──────────────────┘
                                                │
                                                │ Shared State
                                                ▼
                                       ┌──────────────────┐
                                       │   Emulator Core  │
                                       │   + GUI Window   │
                                       └──────────────────┘
```
