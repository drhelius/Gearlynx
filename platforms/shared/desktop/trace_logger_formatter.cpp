#include <stdio.h>
#include <string.h>
#include "trace_logger_formatter.h"
#include "m6502.h"

static void strip_tags(const char* source, char* destination, size_t size)
{
    size_t output = 0;
    for (size_t input = 0; source[input] && output + 1 < size; input++)
    {
        if (source[input] == '{')
        {
            const char* end = strchr(source + input, '}');
            if (end)
            {
                input = (size_t)(end - source);
                continue;
            }
        }
        destination[output++] = source[input];
    }
    destination[output] = 0;
}

void trace_log_format_cycle_prefix(const GLYNX_Trace_Entry& entry,
    const GLYNX_Trace_Entry* previous, char* buffer, size_t buffer_size)
{
    if (!previous)
        snprintf(buffer, buffer_size, "@%012llu                ", (unsigned long long)entry.cycle);
    else if (entry.cycle < previous->cycle)
        snprintf(buffer, buffer_size, "@%012llu RESET          ", (unsigned long long)entry.cycle);
    else
        snprintf(buffer, buffer_size, "@%012llu +%-12llu ", (unsigned long long)entry.cycle,
            (unsigned long long)(entry.cycle - previous->cycle));
}

void trace_log_format_cpu_bytes(const GLYNX_Trace_Entry& entry, char* buffer, size_t buffer_size)
{
    size_t offset = 0;
    buffer[0] = 0;
    for (u8 i = 0; i < entry.cpu.size && offset + 4 < buffer_size; i++)
        offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
            "%02X ", entry.cpu.opcodes[i]);
}

static void format_cpu(const GLYNX_Trace_Entry& entry,
    const GLYNX_Trace_Format_Options& options, char* buffer, size_t size)
{
    char mnemonic[80] = "???";
    if (entry.cpu.name[0] != 0)
        strip_tags(entry.cpu.name, mnemonic, sizeof(mnemonic));

    char registers[64] = "";
    if (options.registers)
        snprintf(registers, sizeof(registers), "A:%02X X:%02X Y:%02X S:%02X ",
            entry.cpu.a, entry.cpu.x, entry.cpu.y, entry.cpu.s);

    char flags[24] = "";
    if (options.flags)
    {
        u8 value = entry.cpu.p;
        snprintf(flags, sizeof(flags), "%c%c-%c%c%c%c%c ",
            value & FLAG_NEGATIVE ? 'N' : 'n', value & FLAG_OVERFLOW ? 'V' : 'v',
            value & FLAG_BREAK ? 'B' : 'b', value & FLAG_DECIMAL ? 'D' : 'd',
            value & FLAG_INTERRUPT ? 'I' : 'i', value & FLAG_ZERO ? 'Z' : 'z',
            value & FLAG_CARRY ? 'C' : 'c');
    }

    char bytes[16] = "";
    if (options.bytes)
        trace_log_format_cpu_bytes(entry, bytes, sizeof(bytes));

    snprintf(buffer, size, "[CPU] %04X  %s%s%-24s %s", entry.cpu.pc,
        registers, flags, mnemonic, bytes);
}

static void format_math(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    const char* phase = entry.math.event == TRACE_SUZY_MATH_COMPLETION ? "DONE" : "START";
    if (entry.math.is_divide)
        snprintf(buffer, size, "[SUZY] DIV %-5s $%08X / $%04X = $%08X R:$%04X Cycles:%u%s",
            phase, entry.math.op_a, (u16)entry.math.op_b, entry.math.result,
            entry.math.remainder, entry.math.elapsed_cycles,
            entry.math.div_by_zero ? " [DIV0]" : "");
    else
        snprintf(buffer, size, "[SUZY] MUL %-5s $%04X * $%04X = $%08X Cycles:%u%s%s",
            phase, (u16)entry.math.op_a, (u16)entry.math.op_b, entry.math.result,
            entry.math.elapsed_cycles, entry.math.is_signed ? " [SIGN]" : "",
            entry.math.accumulate ? " [ACC]" : "");
}

static void format_sprite(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    static const char* types[] = {"BG", "BGNC", "BSHD", "BNDY", "NORM", "NCOL", "XOR", "SHDW"};
    switch (entry.sprite.event)
    {
        case TRACE_SUZY_SPRITE_ENGINE_START:
            snprintf(buffer, size, "[SUZY] ENGINE START SCB:$%04X SPRGO:$%02X SUZYBUSEN:$%02X",
                entry.sprite.scb_addr, entry.sprite.sprgo, entry.sprite.suzybusen);
            break;
        case TRACE_SUZY_SPRITE_ENGINE_END:
            snprintf(buffer, size, "[SUZY] ENGINE END Cycles:%u", entry.sprite.total_cycles);
            break;
        case TRACE_SUZY_SPRITE_SKIP:
            snprintf(buffer, size, "[SUZY] SCB SKIP $%04X Reason:%u Next:$%04X", entry.sprite.scb_addr, entry.sprite.reason, entry.sprite.scb_next);
            break;
        case TRACE_SUZY_SPRITE_COLLISION:
            snprintf(buffer, size, "[SUZY] COLLISION SCB:$%04X ID:%u Depository:$%02X EVERON:%s",
                entry.sprite.scb_addr, entry.sprite.collision_id, entry.sprite.depository,
                entry.sprite.everon ? "yes" : "no");
            break;
        case TRACE_SUZY_SPRITE_ROW:
            snprintf(buffer, size, "[SUZY] ROW SCB:$%04X Source:%u Output:%u BPP:%u Charged:%u",
                entry.sprite.scb_addr, entry.sprite.source_pixels, entry.sprite.output_pixels,
                entry.sprite.bpp, entry.sprite.charged_cycles);
            break;
        case TRACE_SUZY_SPRITE_BUS:
            snprintf(buffer, size, "[SUZY] BUS SCB:$%04X Cycles:%u Reason:%u", entry.sprite.scb_addr,
                entry.sprite.charged_cycles, entry.sprite.reason);
            break;
        default:
            snprintf(buffer, size, "[SUZY] SCB $%04X Next:$%04X (%d,%d) %uBPP %s",
                entry.sprite.scb_addr, entry.sprite.scb_next, entry.sprite.hpos,
                entry.sprite.vpos, entry.sprite.bpp, types[entry.sprite.type & 7]);
            break;
    }
}

void trace_logger_format_entry(const GLYNX_Trace_Entry& entry,
    const GLYNX_Trace_Format_Options& options, char* buffer, size_t buffer_size)
{
    char cycles[48];
    char text[GLYNX_TRACE_FORMAT_BUFFER_SIZE];
    cycles[0] = 0;
    if (options.cycles)
        trace_log_format_cycle_prefix(entry, options.previous, cycles, sizeof(cycles));

    switch (entry.type)
    {
        case TRACE_CPU:
            format_cpu(entry, options, text, sizeof(text));
            break;
        case TRACE_CPU_IRQ:
            snprintf(text, sizeof(text), "[CPU] IRQ PC:$%04X Vector:$%04X Mask:$%02X",
                entry.irq.pc, entry.irq.vector, entry.irq.irq_mask);
            break;
        case TRACE_SUZY_MATH:
            format_math(entry, text, sizeof(text));
            break;
        case TRACE_SUZY_SPRITE:
            format_sprite(entry, text, sizeof(text));
            break;
        case TRACE_SUZY_INPUT:
            snprintf(text, sizeof(text), "[SUZY] INPUT %s:$%02X",
                entry.input.is_joystick ? "JOYSTICK" : "SWITCHES", entry.input.value);
            break;
        case TRACE_MIKEY_TIMER:
            snprintf(text, sizeof(text), "[MIKEY] TIMER %u Event:%u Reg:%u Raw:$%02X BKUP:$%02X CNT:$%02X CTLA:$%02X CTLB:$%02X%s%s IRQ:$%02X",
                entry.timer.timer_id, entry.timer.event, entry.timer.reg, entry.timer.raw,
                entry.timer.backup, entry.timer.counter, entry.timer.control_a,
                entry.timer.control_b, entry.timer.reload ? " RELOAD" : "",
                entry.timer.one_shot ? " ONE-SHOT" : "", entry.timer.irq_pending);
            break;
        case TRACE_MIKEY_INTERRUPT:
            snprintf(text, sizeof(text), "[MIKEY] IRQ Event:%u Reg:%u Raw:$%02X Pending:$%02X Mask:$%02X Effective:$%02X Line:%s",
                entry.interrupt.event, entry.interrupt.reg, entry.interrupt.raw,
                entry.interrupt.pending, entry.interrupt.mask, entry.interrupt.effective,
                entry.interrupt.asserted ? "high" : "low");
            break;
        case TRACE_MIKEY_DISPLAY:
            if (entry.display.event == TRACE_MIKEY_DISPLAY_PALETTE)
                snprintf(text, sizeof(text), "[MIKEY] PALETTE Index:%u Raw:$%02X RGB444:$%03X",
                    entry.display.reg, entry.display.raw, entry.display.value & 0x0FFF);
            else
                snprintf(text, sizeof(text), "[MIKEY] DISPLAY Event:%u Reg:$%02X Raw:$%02X Effective:$%02X Addr:$%04X Aux:$%04X Line:%u",
                    entry.display.event, entry.display.reg, entry.display.raw,
                    entry.display.effective, entry.display.address, entry.display.auxiliary,
                    entry.display.line);
            break;
        case TRACE_MIKEY_AUDIO:
            snprintf(text, sizeof(text), "[MIKEY] AUDIO Event:%u Channel:%u Reg:$%02X Raw:$%02X Effective:$%02X",
                entry.audio.event, entry.audio.channel, entry.audio.reg,
                entry.audio.value, entry.audio.effective);
            break;
        case TRACE_MIKEY_UART:
            snprintf(text, sizeof(text), "[UART] Event:%u Data:$%02X Flags:$%02X Source:%u Lost:$%02X Gap:%uus Backup:$%02X CTLA:$%02X%s",
                entry.uart.event, entry.uart.data, entry.uart.flags, entry.uart.source,
                entry.uart.lost, entry.uart.gap_us, entry.uart.backup, entry.uart.control,
                entry.uart.chained ? " CHAINED" : "");
            break;
        case TRACE_REDEYE:
        {
            char payload[40] = "";
            size_t offset = 0;
            for (u8 i = 0; i < entry.redeye.len && offset + 4 < sizeof(payload); i++)
                offset += (size_t)snprintf(payload + offset, sizeof(payload) - offset,
                    "%s%02X", i ? " " : "", entry.redeye.payload[i]);
            const char* checksum = entry.redeye.event == TRACE_REDEYE_PACKET &&
                !entry.redeye.checksum_ok ? " [BAD CSUM]" : "";
            snprintf(text, sizeof(text), "[REDEYE] %s Event:%u Msg:%u Player:%u Seq:%u Size:%u Captured:%u/%u Data:%s%s Problem:%u",
                entry.redeye.dir ? "RX" : "TX", entry.redeye.event,
                entry.redeye.msg, entry.redeye.player, entry.redeye.seq,
                entry.redeye.size, entry.redeye.len, entry.redeye.total,
                payload, checksum,
                entry.redeye.problem);
            break;
        }
        case TRACE_CARTRIDGE:
            snprintf(text, sizeof(text), "[CART] Event:%u %s Bank:%u Addr:$%05X Value:$%02X Shift:$%02X Bit:%u Page:$%03X%s Op:%u",
                entry.cart.event, entry.cart.write ? "WRITE" : "READ", entry.cart.bank,
                entry.cart.address, entry.cart.value, entry.cart.addr_shift,
                entry.cart.bit, entry.cart.page, entry.cart.audin ? " AUDIN" : "",
                entry.cart.operation);
            break;
        case TRACE_DEBUG_MESSAGE:
            snprintf(text, sizeof(text), "[DEBUG] %s", entry.debug_msg.text);
            break;
        default:
            snprintf(text, sizeof(text), "[???]");
            break;
    }

    snprintf(buffer, buffer_size, "%s%s", cycles, text);
}
