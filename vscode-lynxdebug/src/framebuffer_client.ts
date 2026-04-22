import * as net from 'net';
import { EventEmitter } from 'events';

export interface FrameData {
    width: number;
    height: number;
    pixels: number[];
}

export class FramebufferStreamClient extends EventEmitter {
    private socket: net.Socket | null = null;
    private recvBuf = Buffer.alloc(0);
    private port = 0;
    private connected = false;
    private reconnectTimer: ReturnType<typeof setTimeout> | null = null;
    private generation = 0;

    connect(port: number): void {
        this.port = port;
        this.doConnect();
    }

    disconnect(): void {
        this.port = 0;
        this.generation++;
        this.clearReconnect();
        if (this.socket) {
            this.socket.destroy();
            this.socket = null;
        }
        if (this.connected) {
            this.connected = false;
            this.emit('status', false);
        }
    }

    isConnected(): boolean {
        return this.connected;
    }

    private doConnect(): void {
        if (this.port <= 0) return;
        if (this.socket) { this.socket.destroy(); this.socket = null; }

        const gen = ++this.generation;
        this.recvBuf = Buffer.alloc(0);
        this.socket = new net.Socket();

        this.socket.connect(this.port, '127.0.0.1', () => {
            if (gen !== this.generation) return;
            this.connected = true;
            this.emit('status', true);
        });

        this.socket.on('data', (data: Buffer) => {
            if (gen !== this.generation) return;
            this.recvBuf = Buffer.concat([this.recvBuf, data]);
            this.processFrames();
        });

        this.socket.on('close', () => {
            if (gen !== this.generation) return;
            this.connected = false;
            this.emit('status', false);
            this.scheduleReconnect();
        });

        this.socket.on('error', () => {});
    }

    private processFrames(): void {
        while (this.recvBuf.length >= 8) {
            const w = this.recvBuf[0] | (this.recvBuf[1] << 8);
            const h = this.recvBuf[2] | (this.recvBuf[3] << 8);
            const size = this.recvBuf[4] | (this.recvBuf[5] << 8)
                       | (this.recvBuf[6] << 16) | (this.recvBuf[7] << 24);

            if (this.recvBuf.length < 8 + size) return;

            const pixels = this.recvBuf.subarray(8, 8 + size);
            this.recvBuf = this.recvBuf.subarray(8 + size);

            this.emit('frame', { width: w, height: h, pixels: Array.from(pixels) } as FrameData);
        }
    }

    private scheduleReconnect(): void {
        this.clearReconnect();
        if (this.port > 0) {
            this.reconnectTimer = setTimeout(() => this.doConnect(), 2000);
        }
    }

    private clearReconnect(): void {
        if (this.reconnectTimer) {
            clearTimeout(this.reconnectTimer);
            this.reconnectTimer = null;
        }
    }
}
