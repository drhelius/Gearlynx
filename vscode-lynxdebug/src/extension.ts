import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import { LynxDebugSession } from './lynx_debug_session';

export function activate(context: vscode.ExtensionContext): void {
    const factory = new LynxDebugAdapterFactory();
    context.subscriptions.push(
        vscode.debug.registerDebugAdapterDescriptorFactory('lynx', factory)
    );

    const provider = new LynxConfigurationProvider();
    context.subscriptions.push(
        vscode.debug.registerDebugConfigurationProvider('lynx', provider)
    );
}

export function deactivate(): void {
    // nothing to clean up
}

class LynxDebugAdapterFactory implements vscode.DebugAdapterDescriptorFactory {
    createDebugAdapterDescriptor(
        _session: vscode.DebugSession,
        _executable: vscode.DebugAdapterExecutable | undefined
    ): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
        return new vscode.DebugAdapterInlineImplementation(new LynxDebugSession());
    }
}

class LynxConfigurationProvider implements vscode.DebugConfigurationProvider {
    resolveDebugConfiguration(
        _folder: vscode.WorkspaceFolder | undefined,
        config: vscode.DebugConfiguration,
        _token?: vscode.CancellationToken
    ): vscode.ProviderResult<vscode.DebugConfiguration> {
        if (!config.type && !config.request && !config.name) {
            // "Just press F5" with no launch.json
            const editor = vscode.window.activeTextEditor;
            if (editor) {
                config.type = 'lynx';
                config.name = 'Launch Lynx';
                config.request = 'launch';
                config.rom = '${workspaceFolder}/game.lnx';
                config.stopOnEntry = true;
            }
        }

        // Fill in default port from settings
        if (!config.port) {
            const settings = vscode.workspace.getConfiguration('lynxDebug');
            config.port = settings.get<number>('defaultPort', 6502);
        }

        // Fill in gearlynx path from settings if not in launch config
        if (config.request === 'launch' && !config.gearlynxPath) {
            const settings = vscode.workspace.getConfiguration('lynxDebug');
            const globalPath = settings.get<string>('gearlynxPath', '');
            if (globalPath) {
                config.gearlynxPath = globalPath;
            }
        }

        return config;
    }

    resolveDebugConfigurationWithSubstitutedVariables(
        _folder: vscode.WorkspaceFolder | undefined,
        config: vscode.DebugConfiguration,
        _token?: vscode.CancellationToken
    ): vscode.ProviderResult<vscode.DebugConfiguration> {
        // Auto-detect debug file after ${workspaceFolder} etc. are resolved
        if (config.rom && !config.debugFile) {
            const baseName = config.rom.replace(/\.[^.]+$/, '');
            // Try common naming patterns:
            // game.dbg, game.lnx.dbg, game.sym, game.lnx.sym
            const candidates = [
                baseName + '.dbg',
                config.rom + '.dbg',
                baseName + '.sym',
                config.rom + '.sym',
            ];
            for (const debugPath of candidates) {
                if (fs.existsSync(debugPath)) {
                    config.debugFile = debugPath;
                    break;
                }
            }
        }

        return config;
    }
}
