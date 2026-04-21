import * as fs from 'fs';
import { DebugSymbol, DebugInfoData } from './types';

export class SymDebugInfo {
    static parse(filePath: string): DebugInfoData | null {
        let src: string;
        try {
            src = fs.readFileSync(filePath, 'utf-8');
        } catch {
            return null;
        }

        const symbols: DebugSymbol[] = [];
        const lines = src.split('\n');

        for (const line of lines) {
            const trimmed = line.trim();
            if (trimmed.length === 0 || trimmed.startsWith(';') || trimmed.startsWith('#')) {
                continue;
            }

            // Common formats:
            // "ADDRESS LABEL" (cc65 .sym)
            // "LABEL = $ADDRESS" (ca65 exports)
            // "al ADDRESS .LABEL" (VICE label format)

            let address: number | undefined;
            let name: string | undefined;

            // Try "al CXXXX .label" (VICE format)
            const viceMatch = trimmed.match(/^al\s+C?([0-9a-fA-F]{4,6})\s+\.(.+)$/);
            if (viceMatch) {
                address = parseInt(viceMatch[1], 16);
                name = viceMatch[2];
            }

            // Try "ADDRESS LABEL" format
            if (!name) {
                const simpleMatch = trimmed.match(/^([0-9a-fA-F]{2,6})\s+(.+)$/);
                if (simpleMatch) {
                    address = parseInt(simpleMatch[1], 16);
                    name = simpleMatch[2].trim();
                }
            }

            // Try "LABEL = $ADDRESS"
            if (!name) {
                const eqMatch = trimmed.match(/^(\S+)\s*=\s*\$([0-9a-fA-F]{2,6})$/);
                if (eqMatch) {
                    name = eqMatch[1];
                    address = parseInt(eqMatch[2], 16);
                }
            }

            if (name && address !== undefined && !isNaN(address)) {
                symbols.push({
                    name: name,
                    address: address,
                    isGlobal: true,
                    isZeroPage: address < 0x100,
                    isCVariable: false,
                    segment: '',
                });
            }
        }

        return {
            addressToSource: new Map(),
            sourceToAddresses: new Map(),
            symbols: symbols,
            functions: [],
            locals: [],
            zeropageStackPointerAddr: 0x02,
            overlayGroups: [],
            segmentForAddress: new Map(),
            segments: [],
        };
    }
}
