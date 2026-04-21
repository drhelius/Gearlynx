import * as net from 'net';
import { EventEmitter } from 'events';
import {
    MonitorRequest,
    MonitorResponse,
    MonitorEvent,
    MonitorMessage,
    CpuRegisters,
    BreakpointInfo,
    DisasmLine,
    MemoryAreaInfo,
    DebugStatus,
} from './types';

export class DebugMonitorClient extends EventEmitter {
    private socket: net.Socket | null = null;
    private buffer: string = '';
    private requestId: number = 0;
    private pendingRequests: Map<number, {
        resolve: (value: MonitorResponse) => void;
        reject: (reason: Error) => void;
    }> = new Map();
    private connected: boolean = false;

    async connect(hostname: string, port: number, timeoutMs: number = 10000): Promise<void> {
        return new Promise((resolve, reject) => {
            const startTime = Date.now();
            const retryDelay = 100;
            let attempt = 0;

            const tryConnect = (): void => {
                if (Date.now() - startTime > timeoutMs) {
                    reject(new Error(`Connection timeout after ${timeoutMs}ms`));
                    return;
                }

                attempt++;
                const sock = new net.Socket();

                sock.once('connect', () => {
                    sock.removeAllListeners('error');
                    this.socket = sock;
                    this.connected = true;
                    this.setupSocket();
                    resolve();
                });

                sock.once('error', () => {
                    sock.destroy();
                    const delay = Math.min(retryDelay * Math.pow(2, attempt - 1), 2000);
                    setTimeout(tryConnect, delay);
                });

                sock.connect(port, hostname);
            };

            tryConnect();
        });
    }

    disconnect(): void {
        this.connected = false;
        if (this.socket) {
            this.socket.destroy();
            this.socket = null;
        }
        // Reject all pending requests
        for (const [id, pending] of this.pendingRequests) {
            pending.reject(new Error('Disconnected'));
        }
        this.pendingRequests.clear();
    }

    isConnected(): boolean {
        return this.connected;
    }

    // -- Typed command methods --

    async getRegisters(): Promise<CpuRegisters> {
        const resp = await this.sendCommand('registers_get');
        return resp.data as unknown as CpuRegisters;
    }

    async setRegister(name: string, value: number): Promise<void> {
        await this.sendCommand('registers_set', { name, value });
    }

    async getMemory(area: number, offset: number, size: number): Promise<string> {
        const resp = await this.sendCommand('memory_get', { area, offset, size });
        return resp.data['hex'] as string;
    }

    async setMemory(area: number, offset: number, hex: string): Promise<void> {
        await this.sendCommand('memory_set', { area, offset, hex });
    }

    async setBreakpoint(address: number, type: string = 'exec'): Promise<void> {
        await this.sendCommand('breakpoint_set', { address, type });
    }

    async deleteBreakpoint(address: number): Promise<void> {
        await this.sendCommand('breakpoint_delete', { address });
    }

    async listBreakpoints(): Promise<BreakpointInfo[]> {
        const resp = await this.sendCommand('breakpoint_list');
        return resp.data['breakpoints'] as unknown as BreakpointInfo[];
    }

    async continue_(): Promise<void> {
        await this.sendCommand('continue');
    }

    async pause(): Promise<void> {
        await this.sendCommand('pause');
    }

    async stepIn(): Promise<void> {
        await this.sendCommand('step_in');
    }

    async stepOver(): Promise<void> {
        await this.sendCommand('step_over');
    }

    async stepOut(): Promise<void> {
        await this.sendCommand('step_out');
    }

    async stepFrame(): Promise<void> {
        await this.sendCommand('step_frame');
    }

    async reset(): Promise<void> {
        await this.sendCommand('reset');
    }

    async getStatus(): Promise<DebugStatus> {
        const resp = await this.sendCommand('status');
        return resp.data as unknown as DebugStatus;
    }

    async getDisassembly(start: number, end: number): Promise<DisasmLine[]> {
        const resp = await this.sendCommand('disassembly_get', { start, end });
        return resp.data['lines'] as unknown as DisasmLine[];
    }

    async loadRom(path: string): Promise<boolean> {
        const resp = await this.sendCommand('load_rom', { path });
        return resp.data['ok'] as boolean;
    }

    async getCallStack(): Promise<Record<string, unknown>> {
        const resp = await this.sendCommand('call_stack');
        return resp.data;
    }

    async getMemoryAreas(): Promise<MemoryAreaInfo[]> {
        const resp = await this.sendCommand('memory_areas');
        return resp.data['areas'] as unknown as MemoryAreaInfo[];
    }

    async getHardwareStatus(): Promise<Record<string, unknown>> {
        const resp = await this.sendCommand('hardware_status');
        return resp.data;
    }

    async getSpriteCount(): Promise<number> {
        const resp = await this.sendCommand('sprite_count');
        return resp.data['count'] as number;
    }

    async getSpriteInfo(index: number): Promise<Record<string, unknown>> {
        const resp = await this.sendCommand('sprite_info', { index });
        return resp.data;
    }

    async getSpriteImage(index: number): Promise<Record<string, unknown>> {
        const resp = await this.sendCommand('sprite_image', { index });
        return resp.data;
    }

    async getPalette(): Promise<Array<Record<string, unknown>>> {
        const resp = await this.sendCommand('palette');
        return resp.data['colors'] as Array<Record<string, unknown>>;
    }

    async getScreenshot(): Promise<Record<string, unknown>> {
        const resp = await this.sendCommand('screenshot');
        return resp.data;
    }

    async getScreenshotRaw(): Promise<Record<string, unknown>> {
        const resp = await this.sendCommand('screenshot_raw');
        return resp.data;
    }

    async controllerButton(button: string, action: string): Promise<void> {
        await this.sendCommand('controller_button', { button, action });
    }

    async setTraceLog(enabled: boolean, flags: number = 0xFF): Promise<void> {
        await this.sendCommand('trace_log_set', { enabled, flags });
    }

    async getTraceLog(start: number = -1, count: number = 200): Promise<Record<string, unknown>> {
        const resp = await this.sendCommand('trace_log_get', { start, count });
        return resp.data;
    }

    // -- Low-level send/receive --

    private async sendCommand(cmd: string, params: Record<string, unknown> = {}): Promise<MonitorResponse> {
        if (!this.connected || !this.socket) {
            throw new Error('Not connected');
        }

        const id = ++this.requestId;
        const request: MonitorRequest = { id, cmd, ...params };
        const json = JSON.stringify(request);
        const frame = `Content-Length: ${Buffer.byteLength(json)}\r\n\r\n${json}`;

        return new Promise((resolve, reject) => {
            this.pendingRequests.set(id, { resolve, reject });

            const timeout = setTimeout(() => {
                this.pendingRequests.delete(id);
                reject(new Error(`Request timeout: ${cmd}`));
            }, 10000);

            this.pendingRequests.set(id, {
                resolve: (resp) => {
                    clearTimeout(timeout);
                    this.pendingRequests.delete(id);
                    resolve(resp);
                },
                reject: (err) => {
                    clearTimeout(timeout);
                    this.pendingRequests.delete(id);
                    reject(err);
                }
            });

            this.socket!.write(frame);
        });
    }

    private setupSocket(): void {
        if (!this.socket) return;

        this.socket.on('data', (data: Buffer) => {
            this.buffer += data.toString('utf-8');
            this.processBuffer();
        });

        this.socket.on('close', () => {
            this.connected = false;
            this.emit('close');
        });

        this.socket.on('error', (err: Error) => {
            this.emit('error', err);
        });
    }

    private processBuffer(): void {
        while (true) {
            // Look for Content-Length header
            const headerEnd = this.buffer.indexOf('\r\n\r\n');
            if (headerEnd === -1) break;

            const header = this.buffer.substring(0, headerEnd);
            const match = header.match(/Content-Length:\s*(\d+)/i);
            if (!match) {
                // Malformed -- skip to after the header
                this.buffer = this.buffer.substring(headerEnd + 4);
                continue;
            }

            const contentLength = parseInt(match[1], 10);
            const bodyStart = headerEnd + 4;
            const bodyEnd = bodyStart + contentLength;

            if (this.buffer.length < bodyEnd) break; // incomplete

            const body = this.buffer.substring(bodyStart, bodyEnd);
            this.buffer = this.buffer.substring(bodyEnd);

            try {
                const msg = JSON.parse(body) as MonitorMessage;
                this.handleMessage(msg);
            } catch {
                // skip malformed JSON
            }
        }
    }

    private handleMessage(msg: MonitorMessage): void {
        if ('event' in msg) {
            // Async event
            const evt = msg as MonitorEvent;
            this.emit('event', evt);

            if (evt.event === 'stopped') {
                this.emit('stopped', evt.data);
            } else if (evt.event === 'resumed') {
                this.emit('resumed');
            } else if (evt.event === 'terminated') {
                this.emit('terminated');
            }
        } else {
            // Response to a request
            const resp = msg as MonitorResponse;
            const pending = this.pendingRequests.get(resp.id);
            if (pending) {
                if (resp.success) {
                    pending.resolve(resp);
                } else {
                    pending.reject(new Error(
                        (resp.data && resp.data['error'] as string) || 'Request failed'
                    ));
                }
            }
        }
    }
}
