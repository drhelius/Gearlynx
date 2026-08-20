#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "trace_logger_formatter.h"
#include "m6502.h"

static void append_format(char* buffer, size_t size, size_t* offset,
    const char* format, ...)
{
    if (size == 0 || *offset >= size)
        return;

    va_list arguments;
    va_start(arguments, format);
    int written = vsnprintf(buffer + *offset, size - *offset, format, arguments);
    va_end(arguments);

    if (written < 0)
    {
        buffer[*offset] = 0;
        return;
    }

    if ((size_t)written >= size - *offset)
        *offset = size - 1;
    else
        *offset += (size_t)written;
}

static void strip_tags(const char* source, size_t source_size,
    char* destination, size_t size)
{
    if (size == 0)
        return;

    size_t output = 0;
    for (size_t input = 0; input < source_size && source[input] != 0 &&
        output + 1 < size; input++)
    {
        if (source[input] == '{')
        {
            size_t end = input + 1;
            while (end < source_size && source[end] != 0 && source[end] != '}')
                end++;
            if (end < source_size && source[end] == '}')
            {
                input = end;
                continue;
            }
        }
        destination[output++] = source[input];
    }
    destination[output] = 0;
}

static void append_named_mask(char* buffer, size_t size, size_t* offset,
    u8 value, const u8* bits, const char* const* names, size_t count)
{
    bool first = true;
    u8 known = 0;
    for (size_t i = 0; i < count; i++)
    {
        known |= bits[i];
        if ((value & bits[i]) != 0)
        {
            append_format(buffer, size, offset, "%s%s", first ? "" : "|", names[i]);
            first = false;
        }
    }

    u8 unknown = value & (u8)~known;
    if (unknown != 0)
    {
        append_format(buffer, size, offset, "%sUNKNOWN(%u)", first ? "" : "|", unknown);
        first = false;
    }

    if (first)
        append_format(buffer, size, offset, "NONE");
}

static void append_irq_sources(char* buffer, size_t size, size_t* offset, u8 mask)
{
    static const u8 bits[] = {
        TRACE_CPU_IRQ_TIMER0, TRACE_CPU_IRQ_TIMER1, TRACE_CPU_IRQ_TIMER2,
        TRACE_CPU_IRQ_TIMER3, TRACE_CPU_IRQ_UART, TRACE_CPU_IRQ_TIMER5,
        TRACE_CPU_IRQ_TIMER6, TRACE_CPU_IRQ_TIMER7
    };
    static const char* const names[] = {
        "TIMER0", "TIMER1", "TIMER2", "TIMER3", "UART", "TIMER5",
        "TIMER6", "TIMER7"
    };
    append_named_mask(buffer, size, offset, mask, bits, names,
        sizeof(bits) / sizeof(bits[0]));
}

static void append_joystick(char* buffer, size_t size, size_t* offset, u8 value)
{
    static const u8 bits[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    static const char* const names[] = {
        "UP", "DOWN", "LEFT", "RIGHT", "OPTION1", "OPTION2", "B", "A"
    };
    append_named_mask(buffer, size, offset, value, bits, names,
        sizeof(bits) / sizeof(bits[0]));
}

static void append_switches(char* buffer, size_t size, size_t* offset, u8 value)
{
    static const u8 bits[] = {0x01};
    static const char* const names[] = {"PAUSE"};
    append_named_mask(buffer, size, offset, value, bits, names,
        sizeof(bits) / sizeof(bits[0]));
}

static const char* sprite_type_name(u8 type)
{
    switch (type)
    {
        case TRACE_SUZY_SPRITE_TYPE_BACKGROUND: return "BACKGROUND";
        case TRACE_SUZY_SPRITE_TYPE_BACKGROUND_NONCOLLIDABLE: return "BACKGROUND NONCOLLIDABLE";
        case TRACE_SUZY_SPRITE_TYPE_BOUNDARY_SHADOW: return "BOUNDARY SHADOW";
        case TRACE_SUZY_SPRITE_TYPE_BOUNDARY: return "BOUNDARY";
        case TRACE_SUZY_SPRITE_TYPE_NORMAL: return "NORMAL";
        case TRACE_SUZY_SPRITE_TYPE_NONCOLLIDABLE: return "NONCOLLIDABLE";
        case TRACE_SUZY_SPRITE_TYPE_XOR: return "XOR";
        case TRACE_SUZY_SPRITE_TYPE_SHADOW: return "SHADOW";
        default: return NULL;
    }
}

static const char* sprite_skip_name(u8 reason)
{
    switch (reason)
    {
        case TRACE_SUZY_SPRITE_SKIP_NONE: return "NONE";
        case TRACE_SUZY_SPRITE_SKIP_DISABLED: return "DISABLED";
        case TRACE_SUZY_SPRITE_SKIP_STOPPED: return "STOPPED";
        case TRACE_SUZY_SPRITE_SKIP_INVALID_TERMINAL: return "INVALID TERMINAL";
        default: return NULL;
    }
}

static const char* sprite_bus_name(u8 reason)
{
    switch (reason)
    {
        case TRACE_SUZY_SPRITE_BUS_NONE: return "NONE";
        case TRACE_SUZY_SPRITE_BUS_DISPLAY_DMA: return "DISPLAY DMA";
        default: return NULL;
    }
}

static const char* timer_register_name(u8 reg)
{
    switch (reg)
    {
        case 0: return "BKUP";
        case 1: return "CTLA";
        case 2: return "CNT";
        case 3: return "CTLB";
        default: return NULL;
    }
}

static const char* display_register_name(u8 reg)
{
    switch (reg)
    {
        case 0x90: return "SDONEACK";
        case 0x91: return "CPUSLEEP";
        case 0x92: return "DISPCTL";
        case 0x93: return "PBKUP";
        case 0x94: return "DISPADRL";
        case 0x95: return "DISPADRH";
        default: return NULL;
    }
}

static const char* audio_register_name(u8 reg)
{
    switch (reg)
    {
        case 0: return "VOL";
        case 1: return "SHFTFB";
        case 2: return "OUTVAL";
        case 3: return "L8SHFT";
        case 4: return "TBACK";
        case 5: return "CTL";
        case 6: return "COUNT";
        case 7: return "MISC";
        default: return NULL;
    }
}

static const char* mixer_register_name(u8 reg)
{
    switch (reg)
    {
        case 0x40: return "ATTEN_A";
        case 0x41: return "ATTEN_B";
        case 0x42: return "ATTEN_C";
        case 0x43: return "ATTEN_D";
        case 0x44: return "MPAN";
        case 0x50: return "MSTEREO";
        default: return NULL;
    }
}

static const char* uart_source_name(u8 source)
{
    switch (source)
    {
        case TRACE_MIKEY_UART_SOURCE_LOOPBACK: return "LOOPBACK";
        case TRACE_MIKEY_UART_SOURCE_COMLYNX: return "COMLYNX";
        default: return NULL;
    }
}

static const char* redeye_message_name(u8 message)
{
    switch (message)
    {
        case TRACE_REDEYE_MESSAGE_LOGON: return "LOGON";
        case TRACE_REDEYE_MESSAGE_START: return "START";
        case TRACE_REDEYE_MESSAGE_DATA: return "DATA";
        case TRACE_REDEYE_MESSAGE_REQ: return "REQUEST";
        case TRACE_REDEYE_MESSAGE_MASTER_RESEND: return "MASTER RESEND";
        default: return NULL;
    }
}

static const char* eeprom_operation_name(u8 operation)
{
    switch (operation)
    {
        case TRACE_EEPROM_READ: return "READ";
        case TRACE_EEPROM_WRITE: return "WRITE";
        case TRACE_EEPROM_ERASE: return "ERASE";
        case TRACE_EEPROM_EWDS: return "EWDS";
        case TRACE_EEPROM_EWEN: return "EWEN";
        case TRACE_EEPROM_WRAL: return "WRAL";
        case TRACE_EEPROM_ERAL: return "ERAL";
        case TRACE_EEPROM_READY: return "READY";
        default: return NULL;
    }
}

void trace_log_format_cycle_prefix(const GLYNX_Trace_Entry& entry,
    const GLYNX_Trace_Entry* previous, char* buffer, size_t buffer_size)
{
    if (buffer_size == 0)
        return;

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
    if (buffer_size == 0)
        return;

    size_t offset = 0;
    buffer[0] = 0;
    u8 count = entry.cpu.size < sizeof(entry.cpu.opcodes) ? entry.cpu.size :
        (u8)sizeof(entry.cpu.opcodes);
    for (u8 i = 0; i < count; i++)
        append_format(buffer, buffer_size, &offset, "%02X ", entry.cpu.opcodes[i]);
}

static void format_cpu(const GLYNX_Trace_Entry& entry,
    const GLYNX_Trace_Format_Options& options, char* buffer, size_t size)
{
    char mnemonic[80] = "???";
    if (entry.cpu.name[0] != 0)
        strip_tags(entry.cpu.name, sizeof(entry.cpu.name), mnemonic, sizeof(mnemonic));

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

static void format_irq(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    size_t offset = 0;
    append_format(buffer, size, &offset, "[CPU] IRQ PC:$%04X Vector:$%04X Sources:",
        entry.irq.pc, entry.irq.vector);
    append_irq_sources(buffer, size, &offset, entry.irq.irq_mask);
    append_format(buffer, size, &offset, " Mask:$%02X", entry.irq.irq_mask);
}

static void format_math(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    size_t offset = 0;
    if (entry.math.event != TRACE_SUZY_MATH_OPERATION &&
        entry.math.event != TRACE_SUZY_MATH_COMPLETION)
    {
        append_format(buffer, size, &offset, "[SUZY] MATH UNKNOWN(%u)", entry.math.event);
        return;
    }

    const char* phase = entry.math.event == TRACE_SUZY_MATH_COMPLETION ?
        "COMPLETE" : "START";
    if (entry.math.is_divide)
    {
        append_format(buffer, size, &offset,
            "[SUZY] DIV %s Dividend:$%08X Divisor:$%04X Result:$%08X Remainder:$%04X Cycles:%u",
            phase, entry.math.op_a, (u16)entry.math.op_b, entry.math.result,
            entry.math.remainder, entry.math.elapsed_cycles);
        if (entry.math.div_by_zero)
            append_format(buffer, size, &offset, " [DIVIDE-BY-ZERO]");
    }
    else
    {
        append_format(buffer, size, &offset,
            "[SUZY] MUL %s A:$%04X B:$%04X Result:$%08X Cycles:%u",
            phase, (u16)entry.math.op_a, (u16)entry.math.op_b,
            entry.math.result, entry.math.elapsed_cycles);
        if (entry.math.is_signed)
            append_format(buffer, size, &offset, " [SIGNED]");
        if (entry.math.accumulate)
            append_format(buffer, size, &offset, " [ACCUMULATE]");
    }
}

static void append_sprite_type(char* buffer, size_t size, size_t* offset, u8 type)
{
    const char* name = sprite_type_name(type);
    if (name)
        append_format(buffer, size, offset, "%s", name);
    else
        append_format(buffer, size, offset, "UNKNOWN(%u)", type);
}

static void format_sprite(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    size_t offset = 0;
    switch (entry.sprite.event)
    {
        case TRACE_SUZY_SPRITE_ENGINE_START:
            append_format(buffer, size, &offset,
                "[SUZY] ENGINE START SCB:$%04X SPRGO:$%02X SUZYBUSEN:$%02X",
                entry.sprite.scb_addr, entry.sprite.sprgo, entry.sprite.suzybusen);
            break;
        case TRACE_SUZY_SPRITE_ENGINE_END:
            append_format(buffer, size, &offset, "[SUZY] ENGINE END Cycles:%u",
                entry.sprite.total_cycles);
            break;
        case TRACE_SUZY_SPRITE_SCB:
            append_format(buffer, size, &offset,
                "[SUZY] SCB Address:$%04X Next:$%04X Position:(%d,%d) Type:",
                entry.sprite.scb_addr, entry.sprite.scb_next,
                entry.sprite.hpos, entry.sprite.vpos);
            append_sprite_type(buffer, size, &offset, entry.sprite.type);
            append_format(buffer, size, &offset,
                " BPP:%u SPRCTL0:$%02X SPRCTL1:$%02X SPRCOLL:$%02X SPRINIT:$%02X",
                entry.sprite.bpp, entry.sprite.sprctl0, entry.sprite.sprctl1,
                entry.sprite.sprcoll, entry.sprite.sprinit);
            break;
        case TRACE_SUZY_SPRITE_SKIP:
        {
            const char* reason = sprite_skip_name(entry.sprite.reason);
            append_format(buffer, size, &offset,
                "[SUZY] SKIP SCB:$%04X Next:$%04X Cause:",
                entry.sprite.scb_addr, entry.sprite.scb_next);
            if (reason)
                append_format(buffer, size, &offset, "%s", reason);
            else
                append_format(buffer, size, &offset, "UNKNOWN(%u)", entry.sprite.reason);
            break;
        }
        case TRACE_SUZY_SPRITE_COLLISION:
            append_format(buffer, size, &offset,
                "[SUZY] COLLISION SCB:$%04X ID:%u Depository:$%02X EVERON:%s",
                entry.sprite.scb_addr, entry.sprite.collision_id,
                entry.sprite.depository, entry.sprite.everon ? "yes" : "no");
            break;
        case TRACE_SUZY_SPRITE_ROW:
            append_format(buffer, size, &offset,
                "[SUZY] ROW SCB:$%04X SourcePixels:%u OutputPixels:%u BPP:%u ChargedCycles:%u",
                entry.sprite.scb_addr, entry.sprite.source_pixels,
                entry.sprite.output_pixels, entry.sprite.bpp,
                entry.sprite.charged_cycles);
            break;
        case TRACE_SUZY_SPRITE_BUS:
        {
            const char* reason = sprite_bus_name(entry.sprite.reason);
            append_format(buffer, size, &offset,
                "[SUZY] BUS SCB:$%04X ChargedCycles:%u Cause:",
                entry.sprite.scb_addr, entry.sprite.charged_cycles);
            if (reason)
                append_format(buffer, size, &offset, "%s", reason);
            else
                append_format(buffer, size, &offset, "UNKNOWN(%u)", entry.sprite.reason);
            break;
        }
        default:
            append_format(buffer, size, &offset, "[SUZY] SPRITE UNKNOWN(%u)",
                entry.sprite.event);
            break;
    }
}

static void format_input(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    size_t offset = 0;
    if (entry.input.event != TRACE_SUZY_INPUT_READ)
    {
        append_format(buffer, size, &offset, "[SUZY] INPUT UNKNOWN(%u)", entry.input.event);
        return;
    }

    append_format(buffer, size, &offset, "[SUZY] %s READ Raw:$%02X Active:",
        entry.input.is_joystick ? "JOYSTICK" : "SWITCHES", entry.input.value);
    if (entry.input.is_joystick)
        append_joystick(buffer, size, &offset, entry.input.value);
    else
        append_switches(buffer, size, &offset, entry.input.value);
}

static void append_timer_state(const GLYNX_Trace_Entry& entry,
    char* buffer, size_t size, size_t* offset)
{
    append_format(buffer, size, offset,
        " BKUP:$%02X CNT:$%02X CTLA:$%02X CTLB:$%02X Clock:%s Mode:%s",
        entry.timer.backup, entry.timer.counter, entry.timer.control_a,
        entry.timer.control_b, entry.timer.linked ? "LINKED" : "PRESCALED",
        entry.timer.reload ? "RELOAD" : "ONE-SHOT");
}

static void append_irq_state(u8 pending, u8 mask, u8 effective,
    char* buffer, size_t size, size_t* offset)
{
    append_format(buffer, size, offset, " Pending:");
    append_irq_sources(buffer, size, offset, pending);
    append_format(buffer, size, offset, "($%02X) Mask:", pending);
    append_irq_sources(buffer, size, offset, mask);
    append_format(buffer, size, offset, "($%02X) Effective:", mask);
    append_irq_sources(buffer, size, offset, effective);
    append_format(buffer, size, offset, "($%02X)", effective);
}

static void append_timer_destination(char* buffer, size_t size,
    size_t* offset, u8 destination)
{
    if (destination < 8)
        append_format(buffer, size, offset, "TIMER%u", destination);
    else if (destination == 8)
        append_format(buffer, size, offset, "AUDIO0");
    else
        append_format(buffer, size, offset, "UNKNOWN(%u)", destination);
}

static void format_timer(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    size_t offset = 0;
    switch (entry.timer.event)
    {
        case TRACE_MIKEY_TIMER_REGISTER:
        {
            const char* reg = timer_register_name(entry.timer.reg);
            append_format(buffer, size, &offset, "[MIKEY] TIMER REGISTER WRITE Timer:%u ",
                entry.timer.timer_id);
            if (reg)
                append_format(buffer, size, &offset, "%s", reg);
            else
                append_format(buffer, size, &offset, "UNKNOWN(%u)", entry.timer.reg);
            append_format(buffer, size, &offset, " Raw:$%02X Effective:$%02X",
                entry.timer.raw, entry.timer.effective);
            append_timer_state(entry, buffer, size, &offset);
            break;
        }
        case TRACE_MIKEY_TIMER_UNDERFLOW:
            append_format(buffer, size, &offset, "[MIKEY] TIMER UNDERFLOW Timer:%u",
                entry.timer.timer_id);
            append_timer_state(entry, buffer, size, &offset);
            break;
        case TRACE_MIKEY_TIMER_IRQ:
            append_format(buffer, size, &offset, "[MIKEY] TIMER IRQ Timer:%u",
                entry.timer.timer_id);
            append_irq_state(entry.timer.irq_pending, entry.timer.irq_mask,
                entry.timer.irq_effective, buffer, size, &offset);
            break;
        case TRACE_MIKEY_TIMER_LINK:
            append_format(buffer, size, &offset, "[MIKEY] TIMER LINK Timer:%u Destination:",
                entry.timer.timer_id);
            append_timer_destination(buffer, size, &offset, entry.timer.destination);
            append_format(buffer, size, &offset, " CNT:$%02X", entry.timer.counter);
            break;
        default:
            append_format(buffer, size, &offset, "[MIKEY] TIMER UNKNOWN(%u)",
                entry.timer.event);
            break;
    }
}

static void format_interrupt(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    size_t offset = 0;
    if (entry.interrupt.event == TRACE_MIKEY_INTERRUPT_REGISTER)
    {
        if (entry.interrupt.kind == TRACE_MIKEY_INTERRUPT_CLEAR)
            append_format(buffer, size, &offset, "[MIKEY] INTRST CLEAR Raw:$%02X",
                entry.interrupt.raw);
        else if (entry.interrupt.kind == TRACE_MIKEY_INTERRUPT_SET)
            append_format(buffer, size, &offset, "[MIKEY] INTSET SET Raw:$%02X",
                entry.interrupt.raw);
        else
        {
            append_format(buffer, size, &offset, "[MIKEY] INTERRUPT UNKNOWN(%u)",
                entry.interrupt.kind);
            return;
        }
    }
    else if (entry.interrupt.event == TRACE_MIKEY_INTERRUPT_LINE)
    {
        if (entry.interrupt.kind != TRACE_MIKEY_INTERRUPT_LINE_CHANGE)
        {
            append_format(buffer, size, &offset, "[MIKEY] INTERRUPT UNKNOWN(%u)",
                entry.interrupt.kind);
            return;
        }
        append_format(buffer, size, &offset, "[MIKEY] LINE %s",
            entry.interrupt.asserted ? "ASSERTED" : "CLEARED");
    }
    else
    {
        append_format(buffer, size, &offset, "[MIKEY] INTERRUPT UNKNOWN(%u)",
            entry.interrupt.event);
        return;
    }

    append_irq_state(entry.interrupt.pending, entry.interrupt.mask,
        entry.interrupt.effective, buffer, size, &offset);
}

static void format_display(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    size_t offset = 0;
    switch (entry.display.event)
    {
        case TRACE_MIKEY_DISPLAY_REGISTER:
        {
            const char* reg = display_register_name(entry.display.reg);
            append_format(buffer, size, &offset, "[MIKEY] DISPLAY REGISTER WRITE ");
            if (reg)
                append_format(buffer, size, &offset, "%s", reg);
            else
                append_format(buffer, size, &offset, "UNKNOWN(%u)", entry.display.reg);
            append_format(buffer, size, &offset, " Raw:$%02X Effective:$%02X",
                entry.display.raw, entry.display.effective);
            if (entry.display.reg == 0x92)
                append_format(buffer, size, &offset, " Control:$%02X", entry.display.control);
            else if (entry.display.reg == 0x94 || entry.display.reg == 0x95)
                append_format(buffer, size, &offset, " Address:$%04X Latched:$%04X",
                    entry.display.address, entry.display.auxiliary);
            break;
        }
        case TRACE_MIKEY_DISPLAY_PALETTE:
            append_format(buffer, size, &offset,
                "[MIKEY] DISPLAY PALETTE WRITE Index:%u Raw:$%02X RGB444:$%03X",
                entry.display.reg, entry.display.raw, entry.display.value & 0x0FFF);
            break;
        case TRACE_MIKEY_DISPLAY_DMA_START:
            append_format(buffer, size, &offset,
                "[MIKEY] DISPLAY DMA START Address:$%04X Latched:$%04X Line:%u Control:$%02X",
                entry.display.address, entry.display.auxiliary, entry.display.line,
                entry.display.control);
            break;
        case TRACE_MIKEY_DISPLAY_DMA_LINE:
            append_format(buffer, size, &offset,
                "[MIKEY] DISPLAY DMA LINE Line:%u Address:$%04X Latched:$%04X",
                entry.display.line, entry.display.address, entry.display.auxiliary);
            break;
        case TRACE_MIKEY_DISPLAY_DMA_END:
            append_format(buffer, size, &offset,
                "[MIKEY] DISPLAY DMA END Line:%u Address:$%04X Latched:$%04X",
                entry.display.line, entry.display.address, entry.display.auxiliary);
            break;
        case TRACE_MIKEY_DISPLAY_VBLANK:
            append_format(buffer, size, &offset, "[MIKEY] DISPLAY VBLANK %s Line:%u",
                entry.display.raw ? "ENTER" : "LEAVE", entry.display.line);
            break;
        case TRACE_MIKEY_DISPLAY_FRAME:
            append_format(buffer, size, &offset, "[MIKEY] DISPLAY FRAME START Line:%u",
                entry.display.line);
            break;
        default:
            append_format(buffer, size, &offset, "[MIKEY] DISPLAY UNKNOWN(%u)",
                entry.display.event);
            break;
    }
}

static void format_audio(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    size_t offset = 0;
    switch (entry.audio.event)
    {
        case TRACE_MIKEY_AUDIO_CHANNEL:
        {
            const char* reg = audio_register_name(entry.audio.reg);
            append_format(buffer, size, &offset, "[MIKEY] AUDIO CHANNEL WRITE Channel:%u ",
                entry.audio.channel);
            if (reg)
                append_format(buffer, size, &offset, "%s", reg);
            else
                append_format(buffer, size, &offset, "UNKNOWN(%u)", entry.audio.reg);
            append_format(buffer, size, &offset, " Raw:$%02X Effective:$%02X",
                entry.audio.value, entry.audio.effective);
            break;
        }
        case TRACE_MIKEY_AUDIO_MIXER:
        {
            const char* reg = mixer_register_name(entry.audio.reg);
            append_format(buffer, size, &offset, "[MIKEY] AUDIO MIXER WRITE ");
            if (reg)
                append_format(buffer, size, &offset, "%s", reg);
            else
                append_format(buffer, size, &offset, "UNKNOWN(%u)", entry.audio.reg);
            append_format(buffer, size, &offset, " Raw:$%02X Effective:$%02X",
                entry.audio.value, entry.audio.effective);
            break;
        }
        case TRACE_MIKEY_AUDIO_CLOCK:
            append_format(buffer, size, &offset,
                "[MIKEY] AUDIO CLOCK Channel:%u Output:$%02X",
                entry.audio.channel, entry.audio.effective);
            break;
        default:
            append_format(buffer, size, &offset, "[MIKEY] AUDIO UNKNOWN(%u)",
                entry.audio.event);
            break;
    }
}

static void append_uart_source(char* buffer, size_t size, size_t* offset, u8 source)
{
    const char* name = uart_source_name(source);
    if (name)
        append_format(buffer, size, offset, "%s", name);
    else
        append_format(buffer, size, offset, "UNKNOWN(%u)", source);
}

static void append_uart_errors(char* buffer, size_t size, size_t* offset, u8 flags)
{
    static const u8 bits[] = {
        TRACE_MIKEY_UART_FLAG_PARITY_ERROR, TRACE_MIKEY_UART_FLAG_FRAMING_ERROR,
        TRACE_MIKEY_UART_FLAG_BREAK, TRACE_MIKEY_UART_FLAG_OVERRUN
    };
    static const char* const names[] = {
        "PARITY", "FRAMING", "BREAK", "OVERRUN"
    };
    append_named_mask(buffer, size, offset,
        flags & (u8)~TRACE_MIKEY_UART_FLAG_PARITY_BIT & (u8)~TRACE_MIKEY_UART_FLAG_TURBO,
        bits, names, sizeof(bits) / sizeof(bits[0]));
}

static void append_uart_config(const GLYNX_Trace_Entry& entry,
    char* buffer, size_t size, size_t* offset, bool include_status)
{
    static const u8 config_bits[] = {0x80, 0x40, 0x10, 0x04, 0x02, 0x01};
    static const char* const config_names[] = {
        "TX IRQ", "RX IRQ", "PARITY", "TX OPEN", "TX BREAK", "PARITY EVEN"
    };
    static const u8 status_bits[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    static const char* const status_names[] = {
        "TX READY", "RX READY", "TX EMPTY", "PARITY ERROR", "OVERRUN",
        "FRAMING ERROR", "RX BREAK", "PARITY BIT"
    };

    append_format(buffer, size, offset, " Config:");
    append_named_mask(buffer, size, offset, entry.uart.config,
        config_bits, config_names, sizeof(config_bits) / sizeof(config_bits[0]));
    append_format(buffer, size, offset, "($%02X) Parity:%s", entry.uart.config,
        (entry.uart.config & 0x10) ?
            ((entry.uart.config & 0x01) ? "EVEN" : "ODD") :
            ((entry.uart.config & 0x01) ? "MARK" : "SPACE"));
    if (include_status)
    {
        append_format(buffer, size, offset, " Status:");
        append_named_mask(buffer, size, offset, entry.uart.status,
            status_bits, status_names, sizeof(status_bits) / sizeof(status_bits[0]));
        append_format(buffer, size, offset, "($%02X)", entry.uart.status);
    }
    append_format(buffer, size, offset,
        " TIM4-BKUP:$%02X TIM4-CTLA:$%02X BitCycles:%u",
        entry.uart.backup, entry.uart.control,
        entry.uart.bit_cycles);
    if (entry.uart.bit_cycles != 0)
        append_format(buffer, size, offset, " Baud:%u",
            (u32)(GLYNX_MASTER_CLOCK / entry.uart.bit_cycles));
    else
        append_format(buffer, size, offset, " Baud:UNKNOWN(0)");
    if ((entry.uart.flags & TRACE_MIKEY_UART_FLAG_TURBO) != 0)
        append_format(buffer, size, offset, " [TURBO]");
}

static void format_uart(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    size_t offset = 0;
    if (entry.uart.event > TRACE_MIKEY_UART_COMLYNX)
    {
        append_format(buffer, size, &offset, "[UART] UNKNOWN(%u)", entry.uart.event);
        return;
    }

    switch (entry.uart.kind)
    {
        case TRACE_MIKEY_UART_KIND_CONFIG:
            append_format(buffer, size, &offset, "[UART] CONFIG Raw:$%02X", entry.uart.data);
            append_uart_config(entry, buffer, size, &offset, true);
            break;
        case TRACE_MIKEY_UART_KIND_SERDAT_WRITE:
            append_format(buffer, size, &offset, "[UART] SERDAT WRITE Data:$%02X",
                entry.uart.data);
            break;
        case TRACE_MIKEY_UART_KIND_TX_START:
            append_format(buffer, size, &offset,
                "[UART] TX START Data:$%02X ParityBit:%u Chained:%s",
                entry.uart.data,
                (entry.uart.flags & TRACE_MIKEY_UART_FLAG_PARITY_BIT) ? 1 : 0,
                entry.uart.chained ? "yes" : "no");
            append_uart_config(entry, buffer, size, &offset, false);
            break;
        case TRACE_MIKEY_UART_KIND_TX_END:
            append_format(buffer, size, &offset,
                "[UART] TX END Data:$%02X ParityBit:%u",
                entry.uart.data,
                (entry.uart.flags & TRACE_MIKEY_UART_FLAG_PARITY_BIT) ? 1 : 0);
            break;
        case TRACE_MIKEY_UART_KIND_RX_LATCH:
            append_format(buffer, size, &offset, "[UART] RX LATCH Data:$%02X Source:",
                entry.uart.data);
            append_uart_source(buffer, size, &offset, entry.uart.source);
            append_format(buffer, size, &offset, " ParityBit:%u Errors:",
                (entry.uart.flags & TRACE_MIKEY_UART_FLAG_PARITY_BIT) ? 1 : 0);
            append_uart_errors(buffer, size, &offset, entry.uart.flags);
            append_format(buffer, size, &offset, " Gap:%uus", entry.uart.gap_us);
            if ((entry.uart.flags & TRACE_MIKEY_UART_FLAG_OVERRUN) != 0)
                append_format(buffer, size, &offset, " Lost:$%02X", entry.uart.lost);
            break;
        case TRACE_MIKEY_UART_KIND_DATA_READ:
            append_format(buffer, size, &offset, "[UART] DATA READ Data:$%02X Gap:%uus",
                entry.uart.data, entry.uart.gap_us);
            break;
        case TRACE_MIKEY_UART_KIND_IRQ_ASSERTED:
        case TRACE_MIKEY_UART_KIND_IRQ_CLEARED:
        {
            static const u8 bits[] = {
                TRACE_MIKEY_UART_IRQ_SOURCE_TX, TRACE_MIKEY_UART_IRQ_SOURCE_RX
            };
            static const char* const names[] = {"TX", "RX"};
            append_format(buffer, size, &offset, "[UART] IRQ %s Sources:",
                entry.uart.kind == TRACE_MIKEY_UART_KIND_IRQ_ASSERTED ?
                    "ASSERTED" : "CLEARED");
            append_named_mask(buffer, size, &offset, entry.uart.flags,
                bits, names, sizeof(bits) / sizeof(bits[0]));
            append_format(buffer, size, &offset, " Status:$%02X", entry.uart.status);
            break;
        }
        case TRACE_MIKEY_UART_KIND_PROBLEM:
            append_format(buffer, size, &offset, "[UART] PROBLEM Data:$%02X Source:",
                entry.uart.data);
            append_uart_source(buffer, size, &offset, entry.uart.source);
            append_format(buffer, size, &offset, " Errors:");
            append_uart_errors(buffer, size, &offset, entry.uart.flags);
            append_format(buffer, size, &offset, " Gap:%uus", entry.uart.gap_us);
            if ((entry.uart.flags & TRACE_MIKEY_UART_FLAG_OVERRUN) != 0)
                append_format(buffer, size, &offset, " Lost:$%02X", entry.uart.lost);
            break;
        case TRACE_MIKEY_UART_KIND_TX_BREAK_ASSERTED:
        case TRACE_MIKEY_UART_KIND_TX_BREAK_CLEARED:
            append_format(buffer, size, &offset, "[UART] TX BREAK State:%s",
                entry.uart.kind == TRACE_MIKEY_UART_KIND_TX_BREAK_ASSERTED ?
                    "ASSERTED" : "CLEARED");
            break;
        case TRACE_MIKEY_UART_KIND_CABLE_CONNECTED:
            append_format(buffer, size, &offset, "[UART] CABLE CONNECTED");
            break;
        case TRACE_MIKEY_UART_KIND_CABLE_DISCONNECTED:
            append_format(buffer, size, &offset, "[UART] CABLE DISCONNECTED");
            break;
        default:
            append_format(buffer, size, &offset, "[UART] UNKNOWN(%u)", entry.uart.kind);
            break;
    }
}

static void append_redeye_prefix(char* buffer, size_t size,
    size_t* offset, u8 direction)
{
    append_format(buffer, size, offset, "[REDEYE] ");
    if (direction == 0)
        append_format(buffer, size, offset, "TX --> ");
    else if (direction == 1)
        append_format(buffer, size, offset, "RX <-- ");
    else
        append_format(buffer, size, offset, "UNKNOWN(%u) ", direction);
}

static u8 redeye_payload_size(const GLYNX_Trace_Entry& entry)
{
    return entry.redeye.size > 0 ? (u8)(entry.redeye.size - 1) : 0;
}

static void append_redeye_payload(const GLYNX_Trace_Entry& entry,
    char* buffer, size_t size, size_t* offset)
{
    u8 payload_size = redeye_payload_size(entry);
    u8 count = entry.redeye.len < sizeof(entry.redeye.payload) ? entry.redeye.len :
        (u8)sizeof(entry.redeye.payload);
    if (count > payload_size)
        count = payload_size;

    append_format(buffer, size, offset, " Len:%u", payload_size);
    if (count > 0)
        append_format(buffer, size, offset, " Data:");
    for (u8 i = 0; i < count; i++)
        append_format(buffer, size, offset, "%s%02X", i ? " " : "",
            entry.redeye.payload[i]);
    if (payload_size > count)
        append_format(buffer, size, offset, " Omitted:%u", payload_size - count);
}

static void append_redeye_players(char* buffer, size_t size,
    size_t* offset, u8 mask)
{
    append_format(buffer, size, offset, "{");
    bool first = true;
    for (u8 player = 0; player < 8; player++)
    {
        if ((mask & (1u << player)) != 0)
        {
            append_format(buffer, size, offset, "%s%u", first ? "" : ",", player);
            first = false;
        }
    }
    append_format(buffer, size, offset, "}");
}

static bool redeye_has_complete_payload(const GLYNX_Trace_Entry& entry, u8 payload_size)
{
    return entry.redeye.header_valid && entry.redeye.size == payload_size + 1u &&
        entry.redeye.total == entry.redeye.size + 2u &&
        entry.redeye.captured == entry.redeye.total &&
        entry.redeye.len >= payload_size;
}

static u8 redeye_header(const GLYNX_Trace_Entry& entry)
{
    return (u8)(entry.redeye.msg | (entry.redeye.player << 3) |
        (entry.redeye.seq << 7));
}

static void append_redeye_raw_header(const GLYNX_Trace_Entry& entry,
    char* buffer, size_t size, size_t* offset)
{
    if (entry.redeye.header_valid)
        append_format(buffer, size, offset, " P%u S%u",
            entry.redeye.player, entry.redeye.seq);
}

static void append_redeye_message(const GLYNX_Trace_Entry& entry,
    char* buffer, size_t size, size_t* offset, bool details)
{
    const char* message = redeye_message_name(entry.redeye.msg);
    switch (entry.redeye.msg)
    {
        case TRACE_REDEYE_MESSAGE_LOGON:
            append_format(buffer, size, offset, "LOGON");
            if (redeye_header(entry) == TRACE_REDEYE_MESSAGE_LOGON &&
                redeye_has_complete_payload(entry, 4))
            {
                append_format(buffer, size, offset, " Claim:P%u Players:",
                    entry.redeye.payload[0]);
                append_redeye_players(buffer, size, offset, entry.redeye.payload[1]);
                append_format(buffer, size, offset, " Game:$%04X",
                    (u16)(entry.redeye.payload[2] |
                    ((u16)entry.redeye.payload[3] << 8)));
            }
            else
            {
                append_redeye_raw_header(entry, buffer, size, offset);
                if (details)
                    append_redeye_payload(entry, buffer, size, offset);
            }
            break;
        case TRACE_REDEYE_MESSAGE_START:
            append_format(buffer, size, offset, "START");
            if (redeye_header(entry) == TRACE_REDEYE_MESSAGE_START &&
                redeye_has_complete_payload(entry, 4))
            {
                append_format(buffer, size, offset, " Countdown:%u Players:",
                    entry.redeye.payload[0]);
                append_redeye_players(buffer, size, offset, entry.redeye.payload[1]);
                append_format(buffer, size, offset, " Game:$%04X",
                    (u16)(entry.redeye.payload[2] |
                    ((u16)entry.redeye.payload[3] << 8)));
            }
            else
            {
                append_redeye_raw_header(entry, buffer, size, offset);
                if (details)
                    append_redeye_payload(entry, buffer, size, offset);
            }
            break;
        case TRACE_REDEYE_MESSAGE_DATA:
            append_format(buffer, size, offset, "DATA P%u S%u",
                entry.redeye.player, entry.redeye.seq);
            if (details)
                append_redeye_payload(entry, buffer, size, offset);
            break;
        case TRACE_REDEYE_MESSAGE_REQ:
            append_format(buffer, size, offset, "REQUEST Target:P%u S%u",
                entry.redeye.player, entry.redeye.seq);
            if (details && redeye_payload_size(entry) != 0)
                append_redeye_payload(entry, buffer, size, offset);
            break;
        case TRACE_REDEYE_MESSAGE_MASTER_RESEND:
            append_format(buffer, size, offset, "MASTER RESEND From:P%u S%u",
                entry.redeye.player, entry.redeye.seq);
            if (entry.redeye.len > 0)
            {
                append_format(buffer, size, offset, " Missing:");
                append_redeye_players(buffer, size, offset, entry.redeye.payload[0]);
                append_format(buffer, size, offset, " Mask:$%02X",
                    entry.redeye.payload[0]);
            }
            else if (details)
                append_redeye_payload(entry, buffer, size, offset);
            break;
        default:
            if (message)
                append_format(buffer, size, offset, "%s", message);
            else
                append_format(buffer, size, offset, "UNKNOWN(%u)", entry.redeye.msg);
            append_redeye_raw_header(entry, buffer, size, offset);
            if (details)
                append_redeye_payload(entry, buffer, size, offset);
            break;
    }
}

static void append_redeye_capture(const GLYNX_Trace_Entry& entry,
    char* buffer, size_t size, size_t* offset)
{
    if (entry.redeye.total > 0)
        append_format(buffer, size, offset, " Captured:%u/%u",
            entry.redeye.captured, entry.redeye.total);
    else
        append_format(buffer, size, offset, " Captured:%u", entry.redeye.captured);
}

static const char* redeye_problem_name(u8 problem)
{
    switch (problem)
    {
        case TRACE_REDEYE_PROBLEM_INVALID_SIZE: return "INVALID SIZE";
        case TRACE_REDEYE_PROBLEM_CHECKSUM: return "CHECKSUM";
        case TRACE_REDEYE_PROBLEM_TIMEOUT: return "TIMEOUT";
        case TRACE_REDEYE_PROBLEM_FRAMING: return "FRAMING";
        case TRACE_REDEYE_PROBLEM_BREAK: return "BREAK";
        case TRACE_REDEYE_PROBLEM_RESET: return "RESET";
        default: return NULL;
    }
}

static void format_redeye(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    size_t offset = 0;
    if (entry.redeye.event == TRACE_REDEYE_PACKET)
    {
        append_redeye_prefix(buffer, size, &offset, entry.redeye.dir);
        append_redeye_message(entry, buffer, size, &offset, true);
    }
    else if (entry.redeye.event == TRACE_REDEYE_PROBLEM)
    {
        const char* problem = redeye_problem_name(entry.redeye.problem);
        append_redeye_prefix(buffer, size, &offset, entry.redeye.dir);
        if (problem)
            append_format(buffer, size, &offset, "%s", problem);
        else
            append_format(buffer, size, &offset, "UNKNOWN(%u)", entry.redeye.problem);
        if (entry.redeye.problem == TRACE_REDEYE_PROBLEM_INVALID_SIZE)
            append_format(buffer, size, &offset, " Size:%u", entry.redeye.value);
        else if (entry.redeye.problem == TRACE_REDEYE_PROBLEM_CHECKSUM)
        {
            if (entry.redeye.header_valid)
            {
                append_format(buffer, size, &offset, " ");
                append_redeye_message(entry, buffer, size, &offset, false);
            }
            append_format(buffer, size, &offset, " Expected:$%02X Actual:$%02X",
                entry.redeye.checksum_expected, entry.redeye.checksum_actual);
        }
        else if (entry.redeye.problem == TRACE_REDEYE_PROBLEM_TIMEOUT ||
            entry.redeye.problem == TRACE_REDEYE_PROBLEM_RESET)
            append_redeye_capture(entry, buffer, size, &offset);
        else if (entry.redeye.problem == TRACE_REDEYE_PROBLEM_FRAMING ||
            entry.redeye.problem == TRACE_REDEYE_PROBLEM_BREAK)
        {
            if (entry.redeye.header_valid)
            {
                append_format(buffer, size, &offset, " ");
                append_redeye_message(entry, buffer, size, &offset, false);
            }
            if (entry.redeye.captured > 0)
                append_redeye_capture(entry, buffer, size, &offset);
            append_format(buffer, size, &offset, " Data:$%02X", entry.redeye.value);
        }
        else if (!problem)
            append_format(buffer, size, &offset, " Value:$%02X", entry.redeye.value);
    }
    else
        append_format(buffer, size, &offset, "[REDEYE] UNKNOWN(%u)", entry.redeye.event);
}

static void append_cart_bank(char* buffer, size_t size, size_t* offset, u8 bank)
{
    if (bank == 0)
        append_format(buffer, size, offset, "CART0");
    else if (bank == 1)
        append_format(buffer, size, offset, "CART1");
    else
        append_format(buffer, size, offset, "UNKNOWN(%u)", bank);
}

static void format_eeprom(const GLYNX_Trace_Entry& entry,
    char* buffer, size_t size, size_t* offset)
{
    const char* operation = eeprom_operation_name(entry.cart.operation);
    append_format(buffer, size, offset, "[CART] EEPROM ");
    if (operation)
        append_format(buffer, size, offset, "%s", operation);
    else
        append_format(buffer, size, offset, "UNKNOWN(%u)", entry.cart.operation);

    if (entry.cart.operation == TRACE_EEPROM_READ ||
        entry.cart.operation == TRACE_EEPROM_WRITE ||
        entry.cart.operation == TRACE_EEPROM_ERASE)
    {
        append_format(buffer, size, offset, " Address:$%04X Data:",
            (u16)entry.cart.address);
        if (entry.cart.data_bits == 8)
            append_format(buffer, size, offset, "$%02X", entry.cart.value & 0xFF);
        else
            append_format(buffer, size, offset, "$%04X", entry.cart.value);
        append_format(buffer, size, offset, " DataBits:");
        if (entry.cart.data_bits == 8 || entry.cart.data_bits == 16)
            append_format(buffer, size, offset, "%u", entry.cart.data_bits);
        else
            append_format(buffer, size, offset, "UNKNOWN(%u)", entry.cart.data_bits);
    }
    else if (entry.cart.operation == TRACE_EEPROM_READY)
        append_format(buffer, size, offset, " Address:$%04X", (u16)entry.cart.address);
}

static void format_cartridge(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    size_t offset = 0;
    switch (entry.cart.event)
    {
        case TRACE_CARTRIDGE_ADDRESS:
            append_format(buffer, size, &offset,
                "[CART] ADDRESS LATCH Shift:$%02X Bit:%u Page:$%03X",
                entry.cart.addr_shift, entry.cart.bit, entry.cart.page);
            break;
        case TRACE_CARTRIDGE_ACCESS:
            append_format(buffer, size, &offset, "[CART] ACCESS %s Bank:",
                entry.cart.write ? "WRITE" : "READ");
            append_cart_bank(buffer, size, &offset, entry.cart.bank);
            append_format(buffer, size, &offset,
                " Address:$%05X Data:$%02X Page:$%03X Shift:$%02X Bit:%u%s",
                entry.cart.address, entry.cart.value, entry.cart.page,
                entry.cart.addr_shift, entry.cart.bit,
                entry.cart.audin ? " AUDIN" : "");
            break;
        case TRACE_CARTRIDGE_EEPROM:
            format_eeprom(entry, buffer, size, &offset);
            break;
        case TRACE_CARTRIDGE_AUDIN:
            append_format(buffer, size, &offset, "[CART] AUDIN ");
            if (entry.cart.operation == 0)
                append_format(buffer, size, &offset, "IODIR");
            else if (entry.cart.operation == 1)
                append_format(buffer, size, &offset, "IODAT");
            else
                append_format(buffer, size, &offset, "UNKNOWN(%u)", entry.cart.operation);
            append_format(buffer, size, &offset,
                " Raw:$%02X Effective:%u Page:$%03X Shift:$%02X",
                entry.cart.value, entry.cart.audin ? 1 : 0,
                entry.cart.page, entry.cart.addr_shift);
            break;
        case TRACE_CARTRIDGE_STORAGE:
            append_format(buffer, size, &offset, "[CART] STORAGE %s Bank:",
                entry.cart.write ? "WRITE" : "READ");
            append_cart_bank(buffer, size, &offset, entry.cart.bank);
            append_format(buffer, size, &offset,
                " Address:$%05X Data:$%02X Page:$%03X",
                entry.cart.address, entry.cart.value, entry.cart.page);
            break;
        default:
            append_format(buffer, size, &offset, "[CART] UNKNOWN(%u)", entry.cart.event);
            break;
    }
}

static void format_debug(const GLYNX_Trace_Entry& entry, char* buffer, size_t size)
{
    snprintf(buffer, size, "[DEBUG] %.*s", (int)sizeof(entry.debug_msg.text),
        entry.debug_msg.text);
}

void trace_logger_format_entry(const GLYNX_Trace_Entry& entry,
    const GLYNX_Trace_Format_Options& options, char* buffer, size_t buffer_size)
{
    if (buffer_size == 0)
        return;

    char cycles[48] = "";
    char text[GLYNX_TRACE_FORMAT_BUFFER_SIZE] = "";
    if (options.cycles)
        trace_log_format_cycle_prefix(entry, options.previous, cycles, sizeof(cycles));

    switch (entry.type)
    {
        case TRACE_CPU:
            format_cpu(entry, options, text, sizeof(text));
            break;
        case TRACE_CPU_IRQ:
            format_irq(entry, text, sizeof(text));
            break;
        case TRACE_SUZY_MATH:
            format_math(entry, text, sizeof(text));
            break;
        case TRACE_SUZY_SPRITE:
            format_sprite(entry, text, sizeof(text));
            break;
        case TRACE_SUZY_INPUT:
            format_input(entry, text, sizeof(text));
            break;
        case TRACE_MIKEY_TIMER:
            format_timer(entry, text, sizeof(text));
            break;
        case TRACE_MIKEY_INTERRUPT:
            format_interrupt(entry, text, sizeof(text));
            break;
        case TRACE_MIKEY_DISPLAY:
            format_display(entry, text, sizeof(text));
            break;
        case TRACE_MIKEY_AUDIO:
            format_audio(entry, text, sizeof(text));
            break;
        case TRACE_MIKEY_UART:
            format_uart(entry, text, sizeof(text));
            break;
        case TRACE_REDEYE:
            format_redeye(entry, text, sizeof(text));
            break;
        case TRACE_CARTRIDGE:
            format_cartridge(entry, text, sizeof(text));
            break;
        case TRACE_DEBUG_MESSAGE:
            format_debug(entry, text, sizeof(text));
            break;
        default:
            snprintf(text, sizeof(text), "[UNKNOWN(%u)]", (u8)entry.type);
            break;
    }

    snprintf(buffer, buffer_size, "%s%s", cycles, text);
}
