#ifndef TRACE_LOGGER_FORMATTER_H
#define TRACE_LOGGER_FORMATTER_H

#include "gearlynx.h"

#define GLYNX_TRACE_FORMAT_BUFFER_SIZE 512

struct GLYNX_Trace_Format_Options
{
    bool registers;
    bool flags;
    bool bytes;
    bool cycles;
    const GLYNX_Trace_Entry* previous;
};

void trace_log_format_cpu_bytes(const GLYNX_Trace_Entry& entry, char* buffer, size_t buffer_size);
void trace_log_format_cycle_prefix(const GLYNX_Trace_Entry& entry,
    const GLYNX_Trace_Entry* previous, char* buffer, size_t buffer_size);
void trace_logger_format_entry(const GLYNX_Trace_Entry& entry,
    const GLYNX_Trace_Format_Options& options, char* buffer, size_t buffer_size);

#endif /* TRACE_LOGGER_FORMATTER_H */
