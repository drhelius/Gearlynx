---
name: gearlynx-debugging
description: >-
  Debug and trace Atari Lynx games using the Gearlynx emulator MCP server.
  Provides workflows for 6502 CPU debugging, breakpoint management, hardware
  inspection, disassembly analysis, and execution tracing. Use when the user
  wants to debug a Lynx game, trace code execution, inspect CPU registers or
  hardware state, set breakpoints, analyze interrupts, step through 6502
  instructions, reverse engineer game code, examine Mikey/Suzy registers, view
  the call stack, or diagnose rendering, audio, or timing issues. Also use when
  the user mentions Atari Lynx development, Lynx homebrew testing, or 6502
  debugging with Gearlynx.
compatibility: >-
  Requires the Gearlynx emulator running as an MCP server (stdio or HTTP
  transport). Configure your AI client to connect to Gearlynx via MCP before
  using this skill.
metadata:
  author: drhelius
  version: "1.0"
---

# Atari Lynx Game Debugging with Gearlynx

## Overview

Debug Atari Lynx games using the Gearlynx emulator as an MCP server. Control execution (pause, step, breakpoints), inspect the 6502 CPU and hardware (Mikey, Suzy), read/write memory, disassemble code, trace instructions, and capture screenshots — all through MCP tool calls. The emulator also serves Lynx hardware documentation as MCP resources.

## MCP Server Requirement

This skill requires the **Gearlynx MCP server** to be installed and connected. All debugging operations are performed through Gearlynx MCP tools. Verify connectivity by calling `debug_get_status` — if it returns a response, the server is active.

### Installing Gearlynx

Run the bundled install script (macOS/Linux):

```bash
bash scripts/install.sh
```

This installs Gearlynx via Homebrew on macOS or downloads the latest release on Linux. It prints the binary path on completion. You can also set `GEARLYNX_INSTALL_DIR` to control where the binary goes (default: `~/.local/bin`).

Alternatively, download from [GitHub Releases](https://github.com/drhelius/Gearlynx/releases/latest) or install with `brew install --cask drhelius/geardome/gearlynx` on macOS.

### Connecting as MCP Server

Gearlynx runs as an MCP server using STDIO transport (recommended). Add `--headless` on headless machines (no display required).

**VS Code** — create `.vscode/mcp.json`:
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

**Claude Code:**
```bash
claude mcp add --transport stdio gearlynx -- /path/to/gearlynx --mcp-stdio
```

**Claude Desktop** — edit config (`~/Library/Application Support/Claude/claude_desktop_config.json` on macOS):
```json
{
  "mcpServers": {
    "gearlynx": {
      "command": "/path/to/gearlynx",
      "args": ["--mcp-stdio"]
    }
  }
}
```

**Headless (no display)** — add `--headless` for servers or CI environments:
```json
"args": ["--headless", "--mcp-stdio"]
```

Replace `/path/to/gearlynx` with the actual binary path from the install script.

### Hardware Documentation (MCP Resources)

The Gearlynx MCP server provides built-in Atari Lynx hardware documentation as MCP resources. Load them into your context when investigating specific hardware.

| Resource | URI | Load when... |
|---|---|---|
| Hardware Addresses | `gearlynx://hardware/hardware_addresses` | 64K memory map, Mikey/Suzy register addresses ($FC00-$FCFF, $FD00-$FDFF) |
| Timers/Interrupts | `gearlynx://hardware/lynx8_timers_interrupts` | 8 timer channels, linking, reload, IRQ sources |
| Interrupts & CPU Sleep | `gearlynx://hardware/irq_interrupts_cpu_sleep` | IRQ vector, interrupt enable/status bits, CPU sleep (JAM) |
| Sprite/Collision | `gearlynx://hardware/lynx6_sprite_collision` | SCB format, sprite types, collision depository |
| Sprite Engine | `gearlynx://hardware/sprite_engine` | Display init, double-buffering, VIDBAS/COLLBAS macros |
| Display | `gearlynx://hardware/lynx5_display` | Frame rate, pen/palette colors, display buffer addresses |
| Audio | `gearlynx://hardware/lynx7_audio_tape_romcart` | 4 audio channels, feedback taps, stereo, ROM cart banking |
| CPU/ROM | `gearlynx://hardware/lynx4_cpu_rom` | 65C02 cycle timing, CPU sleep, ROM mapping |
| Hardware Overview | `gearlynx://hardware/lynx2_hardware_overview` | System block diagram: CPU, Mikey, Suzy, RAM, cart |
| Hardware Quirks | `gearlynx://hardware/lynx3_software_hardware_perniciousness` | Unsafe register operations, hardware gotchas |

---

## Debugging Workflow

### 1. Load and Orient

```
load_media → get_media_info → get_6502_status → get_screenshot
```

Start every session by loading the ROM, confirming it loaded correctly, then checking CPU state and taking a screenshot to understand the current game state. If a `.sym`, `.lbl`, or `.noi` file exists alongside the ROM, symbols are loaded automatically.

Load additional symbols with `load_symbols` or add individual labels with `add_symbol`.

### 2. Pause and Inspect

Always call `debug_pause` before inspecting state. While paused:

- **CPU state**: `get_6502_status` — registers A, X, Y, S, P (flags), PC, interrupt status, MAPCTL visibility
- **Disassembly**: `get_disassembly` with a start/end address range — only shows executed code paths
- **Call stack**: `get_call_stack` — current subroutine hierarchy
- **Memory**: `read_memory` with area name (RAM, Zero Page, Stack, Bank 0, Bank 0A, Bank 1, Bank 1A, BIOS, EEPROM) and address/length

### 3. Set Breakpoints

Use breakpoints to stop execution at points of interest:

| Breakpoint Type | Tool | Use Case |
|---|---|---|
| Execution | `set_breakpoint` (type: exec) | Stop when PC reaches address |
| Read | `set_breakpoint` (type: read) | Stop when memory address is read |
| Write | `set_breakpoint` (type: write) | Stop when memory address is written |
| Range | `set_breakpoint_range` | Cover an address range (exec/read/write) |
| IRQ | `set_breakpoint_on_irq` | Stop on specific timer IRQ (Timer0-7: HBLANK, VBLANK, UART, etc.) |

**Important**: Read/write breakpoints stop with PC at the instruction *after* the memory access.

Manage breakpoints with `list_breakpoints`, `remove_breakpoint`, `list_breakpoints_on_irq`, `clear_breakpoint_on_irq`.

### 4. Step Through Code

After hitting a breakpoint or pausing:

| Action | Tool | Behavior |
|---|---|---|
| Step Into | `debug_step_into` | Execute one instruction, enter subroutines |
| Step Over | `debug_step_over` | Execute one instruction, skip JSR calls |
| Step Out | `debug_step_out` | Run until RTS/RTI returns from current subroutine |
| Step Frame | `debug_step_frame` | Execute until next VBLANK |
| Run To | `debug_run_to_cursor` | Continue until PC reaches target address |
| Continue | `debug_continue` | Resume normal execution |

After each step, call `get_6502_status` and `get_disassembly` to see where you are.

### 5. Trace Execution

The trace logger records CPU instructions interleaved with hardware events (Suzy math/sprites, Mikey timers/audio/UART, cartridge access).

1. `set_trace_log` with `enabled: true` to start recording (optionally filter event types)
2. Let the game run or step through code
3. `set_trace_log` with `enabled: false` to stop (entries are preserved)
4. `get_trace_log` to read recorded entries

Tracing is essential for understanding timing-sensitive code, interrupt handlers, and hardware interaction sequences.

---

## Hardware Inspection

### Mikey (Display, Timers, Audio)

- `get_mikey_registers` — all registers at $FD00-$FDFF, or filter by address
- `get_mikey_timers` — Timer 0-7 status (HBLANK, VBLANK, UART, sample rate, etc.)
- `get_mikey_audio` — audio channel 0-3 status (volume, feedback, output, etc.)
- `get_lcd_status` — LCD pixel/DMA info (only on visible lines)
- `write_mikey_register` — modify a Mikey register live

### Suzy (Sprites, Math)

- `get_suzy_registers` — all registers at $FC00-$FCFF, or filter by address
- `write_suzy_register` — modify a Suzy register live

### Other Hardware

- `get_uart_status` — ComLynx serial port state
- `get_cart_status` — cartridge address generation, bank info, AUDIN
- `get_eeprom_status` — EEPROM type, size, mode, state, IO pins

### Frame Buffers and Screenshots

- `get_screenshot` — current rendered frame as PNG
- `get_frame_buffer` — raw debug frame buffer (VIDBAS from Suzy or DISPADR from Mikey)

Use screenshots after stepping or continuing to see the visual impact of changes.

---

## Common Debugging Scenarios

### Finding an Interrupt Handler

1. `set_breakpoint_on_irq` for the target IRQ (e.g., Timer0 for HBLANK, Timer2 for VBLANK)
2. `debug_continue` to run until the IRQ fires
3. `get_6502_status` + `get_disassembly` to see the handler code
4. `get_call_stack` to see how deep you are
5. `add_symbol` to label the handler address and any subroutines it calls

### Diagnosing Graphics Corruption

1. `debug_pause` → `get_suzy_registers` — check VIDBAS, COLLBAS, sprite control
2. `get_frame_buffer` — capture both Suzy and Mikey buffers
3. `get_mikey_registers` — check DISPADR, display DMA settings
4. `get_lcd_status` — verify line-by-line rendering state
5. Set read/write breakpoints on display buffer addresses to catch corruption source

### Analyzing a Subroutine

1. `set_breakpoint` at the subroutine entry point
2. `debug_continue` → when hit, `get_6502_status`
3. Step through with `debug_step_into` / `debug_step_over`
4. After each step: check registers, read relevant memory
5. `add_symbol` for the routine and any called subroutines
6. `add_disassembler_bookmark` to mark interesting locations

### Tracking a Variable

1. `add_memory_watch` on the variable's address — watches are visible in the emulator GUI
2. Set a write breakpoint with `set_breakpoint` (type: write) on that address
3. When hit, `get_disassembly` reveals what code is modifying it
4. `get_call_stack` shows the call chain leading to the write

### Timing Analysis

1. `set_breakpoint_on_irq` for Timer interrupts
2. `set_trace_log` with `enabled: true` to start recording
3. `get_trace_log` to see the interleaved CPU + hardware events
4. Analyze timer reload values via `get_mikey_timers`
5. Correlate timer fires with code execution in the trace

---

## Organizing Your Debug Session

- **Symbols**: Use `add_symbol` liberally to label addresses you've identified — makes disassembly readable
- **Bookmarks**: Use `add_disassembler_bookmark` for code locations and `add_memory_bookmark` for data regions
- **Watches**: Use `add_memory_watch` for variables you're tracking across steps
- **Save states**: Use `save_state` / `load_state` to snapshot and restore emulator state at interesting points
- **Screenshots**: Capture visual state with `get_screenshot` after significant changes
