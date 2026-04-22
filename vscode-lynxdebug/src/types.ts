// Types shared across the LynxDebug extension

export interface MonitorRequest {
    id: number;
    cmd: string;
    [key: string]: unknown;
}

export interface MonitorResponse {
    id: number;
    success: boolean;
    data: Record<string, unknown>;
}

export interface MonitorEvent {
    id: 0;
    event: string;
    data?: Record<string, unknown>;
}

export type MonitorMessage = MonitorResponse | MonitorEvent;

export interface CpuRegisters {
    pc: number;
    a: number;
    x: number;
    y: number;
    s: number;
    p: number;
    cycles: number;
    halted: boolean;
}

export interface BreakpointInfo {
    address1: number;
    address2: number;
    enabled: boolean;
    read: boolean;
    write: boolean;
    execute: boolean;
    range: boolean;
}

export interface DisasmLine {
    address: number;
    name: string;
    bytes: string;
    size: number;
    jump: boolean;
    jump_address: number;
    subroutine: boolean;
}

export interface MemoryAreaInfo {
    id: number;
    name: string;
    size: number;
    cpu_offset: number;
}

export interface DebugStatus {
    paused: boolean;
    idle: boolean;
    empty: boolean;
    pc: number;
    run_state: string;
    stop_reason: string;
}

// Debug info types for source-level debugging

export interface SourceLocation {
    source: string;
    line: number;
    address: number;
    addressEnd: number;
    segmentId: number;
}

export interface DebugSymbol {
    name: string;
    address: number;
    isGlobal: boolean;
    isZeroPage: boolean;
    isCVariable: boolean;
    segment: string;
}

export interface DebugFunction {
    name: string;
    address: number;
    addressEnd: number;
    source: string;
    line: number;
}

export interface LocalVariable {
    name: string;
    scopeId: number;
    functionAddress: number;
    functionEndAddress: number;
    stackOffset: number | undefined;
    stackPointerOffset: number;
}

export interface DebugInfoData {
    addressToSource: Map<number, SourceLocation>;
    sourceToAddresses: Map<string, Map<number, number[]>>;
    symbols: DebugSymbol[];
    functions: DebugFunction[];
    locals: LocalVariable[];
    zeropageStackPointerAddr: number;
    overlayGroups: OverlayGroup[];
    segments: SegmentInfo[];
}

export interface SegmentInfo {
    name: string;
    start: number;
    size: number;
    type: string;
}

export interface OverlayGroup {
    segmentIds: number[];
    segmentNames: string[];
}
