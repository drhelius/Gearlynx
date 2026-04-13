# Gearlynx Agent Skills

[Agent Skills](https://agentskills.io/) for the Gearlynx Atari Lynx emulator MCP server. These skills teach AI agents how to effectively use Gearlynx's MCP tools for debugging and ROM hacking tasks.

## Prerequisites

All skills require the **Gearlynx emulator** running as an MCP server. The emulator must be configured in your AI client (VS Code, Claude Desktop, Claude Code, etc.) so the agent can access the MCP tools.

See [MCP_README.md](../MCP_README.md) for complete setup instructions (STDIO, HTTP, VS Code, Claude Desktop, Claude Code).

## Installation

The recommended way to install the skills is using the [`skills`](https://skills.sh/docs) CLI, which requires no prior installation:

```bash
npx skills add drhelius/gearlynx
```

Or install a specific skill:

```bash
npx skills add drhelius/gearlynx --skill gearlynx-debugging
npx skills add drhelius/gearlynx --skill gearlynx-romhacking
```

This downloads and configures the skills for use with your AI agent. See the [skills CLI reference](https://skills.sh/docs/cli) for more details.

## Available Skills

### gearlynx-debugging

**Purpose**: Game development, debugging, and tracing of Atari Lynx games.

**What it covers**:
- Loading ROMs and debug symbols
- 6502 CPU register and flag inspection
- Setting execution, read, write, range, and IRQ breakpoints
- Stepping through code (into, over, out, frame, run-to)
- Execution tracing with interleaved hardware events
- Hardware inspection: Mikey (display, timers, audio), Suzy (sprites, math), UART, cartridge, EEPROM
- Frame buffer and screenshot capture
- Call stack analysis
- Organizing debug sessions with symbols, bookmarks, and watches

**Key MCP tools used**: `debug_pause`, `debug_step_into`, `debug_step_over`, `debug_step_out`, `set_breakpoint`, `set_breakpoint_on_irq`, `get_6502_status`, `get_disassembly`, `get_call_stack`, `get_trace_log`, `get_mikey_registers`, `get_mikey_timers`, `get_suzy_registers`, `add_symbol`, `get_screenshot`

**Example prompts**:
- "Find the VBlank interrupt handler and analyze what it does"
- "Set a breakpoint at $8000 and step through the code"
- "The game has corrupted graphics — diagnose the issue"
- "Trace the sprite rendering routine and explain the algorithm"

### gearlynx-romhacking

**Purpose**: Creating modifications, cheats, translations, and ROM hacks for Atari Lynx games.

**What it covers**:
- Memory search workflows (capture → change → compare cycle)
- Finding game variables (lives, health, score, position)
- Creating cheats (infinite lives, score modification, etc.)
- Text and string discovery for translations
- Sprite and graphics data location
- Data table and structure reverse engineering
- Save state management for safe experimentation
- Fast forwarding to reach specific game states

**Key MCP tools used**: `memory_search_capture`, `memory_search`, `memory_find_bytes`, `read_memory`, `write_memory`, `set_breakpoint` (write type), `add_memory_watch`, `add_memory_bookmark`, `save_state`, `load_state`, `toggle_fast_forward`, `get_screenshot`, `get_frame_buffer`, `controller_button`

**Example prompts**:
- "Find the lives counter and give me infinite lives"
- "Search for the score variable in memory"
- "Find all text strings in the ROM for translation"
- "Locate the sprite data for the player character"