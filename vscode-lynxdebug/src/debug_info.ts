import * as path from 'path';
import * as fs from 'fs';
import { SourceLocation, DebugSymbol, DebugFunction, LocalVariable, DebugInfoData } from './types';
import { Cc65DebugInfo } from './debug_info_cc65';
import { SymDebugInfo } from './debug_info_sym';

export class DebugInfo {
    private data: DebugInfoData;
    private sourceRoots: string[];

    private constructor(data: DebugInfoData, sourceRoots: string[]) {
        this.data = data;
        this.sourceRoots = sourceRoots;
    }

    static load(filePath: string, sourceRoots?: string[]): DebugInfo | null {
        if (!fs.existsSync(filePath)) {
            return null;
        }

        const ext = path.extname(filePath).toLowerCase();
        const roots = sourceRoots || [];
        roots.unshift(path.dirname(filePath));

        let data: DebugInfoData | null = null;

        if (ext === '.dbg') {
            data = Cc65DebugInfo.parse(filePath, roots);
        } else if (ext === '.sym') {
            data = SymDebugInfo.parse(filePath);
        }

        if (!data) return null;
        return new DebugInfo(data, roots);
    }

    findSourceForAddress(address: number): SourceLocation | null {
        // Exact match first
        let loc = this.data.addressToSource.get(address);
        if (loc) return this.resolveLocation(loc);

        // Find nearest address <= target
        let bestAddr = -1;
        for (const [addr, _loc] of this.data.addressToSource) {
            if (addr <= address && addr > bestAddr) {
                bestAddr = addr;
            }
        }
        if (bestAddr >= 0) {
            loc = this.data.addressToSource.get(bestAddr);
            if (loc && address <= loc.addressEnd) {
                return this.resolveLocation(loc);
            }
        }
        return null;
    }

    findNearestCodeLine(sourcePath: string, line: number): SourceLocation | null {
        const normalizedPath = this.normalizePath(sourcePath);

        // Find all address mappings for this source file
        const fileMap = this.data.sourceToAddresses.get(normalizedPath);
        if (!fileMap) {
            // Try matching by basename
            for (const [key, map] of this.data.sourceToAddresses) {
                if (path.basename(key).toLowerCase() === path.basename(normalizedPath).toLowerCase()) {
                    return this.findNearestInMap(map, line, key);
                }
            }
            return null;
        }

        return this.findNearestInMap(fileMap, line, normalizedPath);
    }

    findSymbol(name: string): DebugSymbol | null {
        const lowerName = name.toLowerCase();
        return this.data.symbols.find(s =>
            s.name.toLowerCase() === lowerName ||
            s.name.toLowerCase() === '_' + lowerName
        ) || null;
    }

    findSymbolAtAddress(address: number): DebugSymbol | null {
        return this.data.symbols.find(s => s.address === address) || null;
    }

    getSymbols(): DebugSymbol[] {
        return this.data.symbols;
    }

    getFunctions(): DebugFunction[] {
        return this.data.functions;
    }

    getLocalsForAddress(pc: number): LocalVariable[] {
        return this.data.locals.filter(
            l => pc >= l.functionAddress && pc <= l.functionEndAddress
        );
    }

    getZeropageStackPointerAddr(): number {
        return this.data.zeropageStackPointerAddr;
    }

    getZeroPageSymbols(): DebugSymbol[] {
        return this.data.symbols.filter(s => s.isZeroPage);
    }

    // -- Internal helpers --

    private findNearestInMap(
        map: Map<number, number[]>,
        targetLine: number,
        sourcePath: string
    ): SourceLocation | null {
        // Exact line match
        const addrs = map.get(targetLine);
        if (addrs && addrs.length > 0) {
            const loc = this.data.addressToSource.get(addrs[0]);
            if (loc) return this.resolveLocation(loc);
        }

        // Find nearest line >= target
        let bestLine = -1;
        for (const [line] of map) {
            if (line >= targetLine && (bestLine === -1 || line < bestLine)) {
                bestLine = line;
            }
        }

        if (bestLine >= 0) {
            const nearAddrs = map.get(bestLine);
            if (nearAddrs && nearAddrs.length > 0) {
                const loc = this.data.addressToSource.get(nearAddrs[0]);
                if (loc) return this.resolveLocation(loc);
            }
        }

        return null;
    }

    private resolveLocation(loc: SourceLocation): SourceLocation | null {
        // Try to resolve the source path to an absolute path
        if (path.isAbsolute(loc.source) && fs.existsSync(loc.source)) {
            return loc;
        }

        for (const root of this.sourceRoots) {
            const resolved = path.resolve(root, loc.source);
            if (fs.existsSync(resolved)) {
                return { ...loc, source: resolved };
            }
        }

        // File not found on disk -- don't return a bad path
        return null;
    }

    private normalizePath(p: string): string {
        return p.replace(/\\/g, '/').toLowerCase();
    }
}
