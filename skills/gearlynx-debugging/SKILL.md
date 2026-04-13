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
  Requires the Gearlynx MCP server. Before installing or configuring, call
  debug_get_status to check if the server is already connected. If it responds,
  the server is ready — skip setup entirely.
metadata:
  author: drhelius
  version: "1.0"
---

# Atari Lynx Game Debugging with Gearlynx

## Overview

Debug Atari Lynx games using the Gearlynx emulator as an MCP server. Control execution (pause, step, breakpoints), inspect the 6502 CPU and hardware (Mikey, Suzy), read/write memory, disassemble code, trace instructions, and capture screenshots — all through MCP tool calls. Hardware documentation is available in the [references/](references/) directory.

## MCP Server Prerequisite

**IMPORTANT — Check before installing:** Before attempting any installation or configuration, you MUST first verify if the Gearlynx MCP server is already connected in your current session. Call `debug_get_status` — if it returns a valid response, the server is active and ready.

Only if the tool is not available or the call fails, you need to help install and configure the Gearlynx MCP server:

### Installing Gearlynx

Run the bundled install script (macOS/Linux):

```bash
bash scripts/install.sh
```

This installs Gearlynx via Homebrew on macOS or downloads the latest release on Linux. It prints the binary path on completion. You can also set `INSTALL_DIR` to control where the binary goes (default: `~/.local/bin`).

Alternatively, download from [GitHub Releases](https://github.com/drhelius/Gearlynx/releases/latest) or install with `brew install --cask drhelius/geardome/gearlynx` on macOS.

### Connecting as MCP Server

Configure your AI client to run Gearlynx as an MCP server via STDIO transport. Example for Claude Desktop (`~/Library/Application Support/Claude/claude_desktop_config.json`):
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
Replace `/path/to/gearlynx` with the actual binary path from the install script. Add `--headless` before `--mcp-stdio` on headless machines.

### Hardware Documentation (References)

Atari Lynx hardware documentation is available in the [references/](references/) directory. Load them into your context when investigating specific hardware.

| Reference | File | Load when... |
|---|---|---|
| General Overview | [references/lynx1_general_overview.md](references/lynx1_general_overview.md) | System specs, feature set, overall architecture |
| Hardware Overview | [references/lynx2_hardware_overview.md](references/lynx2_hardware_overview.md) | System block diagram: CPU, Mikey, Suzy, RAM, cart |
| Hardware Quirks | [references/lynx3_software_hardware_perniciousness.md](references/lynx3_software_hardware_perniciousness.md) | Unsafe register operations, hardware gotchas |
| CPU/ROM | [references/lynx4_cpu_rom.md](references/lynx4_cpu_rom.md) | 65C02 cycle timing, CPU sleep, ROM mapping |
| Display | [references/lynx5_display.md](references/lynx5_display.md) | Frame rate, pen/palette colors, display buffer addresses |
| Sprite/Collision | [references/lynx6_sprite_collision.md](references/lynx6_sprite_collision.md) | SCB format, sprite types, collision depository |
| Audio/ROM Cart | [references/lynx7_audio_tape_romcart.md](references/lynx7_audio_tape_romcart.md) | 4 audio channels, feedback taps, stereo, ROM cart banking |
| Timers/Interrupts | [references/lynx8_timers_interrupts.md](references/lynx8_timers_interrupts.md) | 8 timer channels, linking, reload, IRQ sources |
| UART | [references/lynx8a_uart.md](references/lynx8a_uart.md) | ComLynx serial port, UART registers, baud rate, flow control |
| Other Hardware | [references/lynx9_other_hardware.md](references/lynx9_other_hardware.md) | Hardware multiply/divide, parallel port, upward compatibility |
| System Reset | [references/lynx10_system_reset.md](references/lynx10_system_reset.md) | Reset/power-up sequence, Suzy/Mikey init, boot ROM |
| System Bus Interplay | [references/lynx11_system_bus_interplay.md](references/lynx11_system_bus_interplay.md) | Bus masters, page mode, DMA timing, bus contention |
| Hardware Addresses | [references/hardware_addresses.md](references/hardware_addresses.md) | 64K memory map, Mikey/Suzy register addresses ($FC00-$FCFF, $FD00-$FDFF) |
| Interrupts & CPU Sleep | [references/irq_interrupts_cpu_sleep.md](references/irq_interrupts_cpu_sleep.md) | IRQ vector, interrupt enable/status bits, CPU sleep (JAM) |
| Sprite Engine | [references/sprite_engine.md](references/sprite_engine.md) | Display init, double-buffering, VIDBAS/COLLBAS macros |

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

### Debug Output for Homebrew Development

Homebrew games can send debug text to the Trace Logger via unused Mikey registers `$FDC0`–`$FDC4` — a `printf`-style mechanism without breakpoints.

1. `set_trace_log` with `enabled: true` and `debug_output: true`
2. Run your game — text written through the registers appears in `get_trace_log`

| Register | Write |
|----------|-------|
| `$FDC0` | Flush buffer to Trace Logger (any non-zero value) |
| `$FDC1` | Append byte as ASCII character |
| `$FDC2` | Append byte as two hex digits |
| `$FDC3` | Set string pointer low byte |
| `$FDC4` | Set string pointer high byte (triggers copy) |

**ca65 example:**

```asm
; Print string + hex value in a single trace entry
; A/X = string pointer, Y = hex value
.proc debug_print
    sta $FDC3       ; pointer low
    stx $FDC4       ; pointer high (triggers copy)
    sty $FDC2       ; append Y as hex
    lda #1
    sta $FDC0       ; flush
    rts
.endproc

    lda #<message
    ldx #>message
    ldy player_hp
    jsr debug_print

message: .asciiz "Player HP: "
```

When disabled (the default), these registers are no-ops — safe to leave in shipping code.

---

## Organizing Your Debug Session

- **Symbols**: Use `add_symbol` liberally to label addresses you've identified — makes disassembly readable
- **Bookmarks**: Use `add_disassembler_bookmark` for code locations and `add_memory_bookmark` for data regions
- **Watches**: Use `add_memory_watch` for variables you're tracking across steps
- **Save states**: Use `save_state` / `load_state` to snapshot and restore emulator state at interesting points
- **Screenshots**: Capture visual state with `get_screenshot` after significant changes
