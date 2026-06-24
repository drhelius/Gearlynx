# Gearlynx Debug-Monitor Protocol

This document describes the wire contract between Gearlynx and external debug
clients (notably the [LynxDebug VS Code extension](https://github.com/BrianPeek/gearlynx-vscode)).
It covers the command-line surface that enables the servers, the debug-monitor
request/response/event format, and the framebuffer stream format.

## Protocol version

The debug-monitor protocol is versioned by a single integer,
`DM_PROTOCOL_VERSION`, defined in
`platforms/shared/desktop/vscode/debug_monitor_server.h`. The current version is **1**.

Bump this version on any **breaking** change to the request/response/event JSON
format (renamed/removed fields, changed semantics). Purely additive changes
(new commands, new optional fields) do not require a bump.

A client negotiates the version with the [`handshake`](#handshake) command
immediately after connecting. The reference client (LynxDebug) warns the user on
a mismatch but still attempts to operate (warn-but-continue).

## Command-line surface

The debug servers are started from the desktop app's argument parser
(`platforms/shared/desktop/main.cpp`):

| Flag | Description |
|------|-------------|
| `--debug-monitor` | Start the debug-monitor TCP server (default port `6502`). |
| `--debug-monitor-port N` | Set the debug-monitor port (`1`-`65534`; port `N + 1` is used for the framebuffer stream). |
| `--headless` | Run without the GUI window. Requires one of `--mcp-stdio`, `--mcp-http`, or `--debug-monitor`. |

When the debug monitor is enabled on port `P`, the **framebuffer stream** server
is started on port `P + 1` (`platforms/shared/desktop/emu.cpp`).

Example (the LynxDebug `launch` configuration spawns this for you):

```
Gearlynx --debug-monitor --debug-monitor-port 6502 --headless game.lnx
```

## Debug-monitor transport

- **Transport:** TCP, bound to `127.0.0.1`, single client at a time. A new
  connection replaces any previous one.
- **Framing:** each message is `Content-Length: <n>\r\n\r\n` followed by `<n>`
  bytes of UTF-8 JSON (the same framing the Debug Adapter Protocol uses).
- **Message size:** frames larger than 4 MiB are rejected.
- **Timeouts:** accepted debug-monitor client sockets use a 10 second send
  timeout. Idle receive waits are allowed so a paused debug session can stay
  connected.
- `TCP_NODELAY` is set on both ends.

### Requests

A request is a JSON object:

```json
{ "id": 12, "cmd": "registers_get", "...": "command-specific params" }
```

- `id` (integer) -- client-assigned, echoed back in the response.
- `cmd` (string) -- the command name.
- Any additional keys are command parameters.

### Responses

```json
{ "id": 12, "success": true, "data": { "...": "..." } }
```

- `id` matches the request.
- `success` (bool) -- `false` indicates failure; `data.error` carries a message.
- `data` -- command-specific payload object.

### Events

Unsolicited state-change notifications use `id: 0` and an `event` field:

```json
{ "id": 0, "event": "stopped", "data": { "reason": "breakpoint", "pc": 512 } }
```

Emitted events: `stopped`, `resumed`, `terminated`. Each event also carries a
monotonically increasing `seq` field in its `data` (sequence counter), in
addition to the event-specific fields shown above.

### Commands

| Command | Params | Returns |
|---------|--------|---------|
| `handshake` | -- | `protocolVersion`, `emulatorVersion` |
| `registers_get` | -- | CPU register snapshot |
| `registers_set` | `name`, `value` | -- |
| `memory_get` | `area`, `offset`, `size` | `hex` |
| `memory_set` | `area`, `offset`, `hex` | -- |
| `breakpoint_set` | `address`, `type` | -- |
| `breakpoint_delete` | `address` | -- |
| `breakpoint_list` | -- | `breakpoints[]` |
| `continue` / `pause` | -- | -- |
| `step_in` / `step_over` / `step_out` / `step_frame` | -- | -- |
| `reset` | -- | -- |
| `status` | -- | run/stop status |
| `disassembly_get` | `start`, `end` | `lines[]` |
| `load_rom` | `path` | `ok` |
| `call_stack` | -- | call-stack data |
| `memory_areas` | -- | `areas[]` |
| `hardware_status` | -- | Mikey/audio/LCD/cart status |
| `controller_button` | `button`, `action` | -- |
| `trace_log_set` | `enabled`, `flags` | -- |
| `trace_log_get` | `start`, `count` | trace data |
| `rewind_step_back` | -- | `ok` |

### handshake

Negotiates the protocol version. Sent first by the client after connecting.

Request:

```json
{ "id": 1, "cmd": "handshake" }
```

Response:

```json
{ "id": 1, "success": true, "data": { "protocolVersion": 1, "emulatorVersion": "1.0.0" } }
```

- `protocolVersion` (integer) -- the emulator's `DM_PROTOCOL_VERSION`.
- `emulatorVersion` (string) -- the Gearlynx build string (`GLYNX_VERSION`),
  informational only.

## Framebuffer stream

A separate raw-binary TCP server (debug-monitor port `+ 1`) streams frames at
display rate (`platforms/shared/desktop/vscode/framebuffer_server.cpp`).
Framebuffer dimensions are capped to the Lynx display envelope and frames larger
than 160 x 160 RGBA pixels are dropped before allocation or transmission.

Per frame, the server sends an 8-byte little-endian header followed by the pixel
payload:

| Offset | Size | Field |
|--------|------|-------|
| 0 | u16 LE | width (pixels) |
| 2 | u16 LE | height (pixels) |
| 4 | u32 LE | size (bytes of pixel data that follow) |
| 8 | `size` | RGBA pixel data (4 bytes/pixel, top-to-bottom rows) |

The stream has no per-frame request; once connected the client reads
header+payload repeatedly. The framebuffer stream is **not** version-gated by the
handshake; its format is considered stable.
