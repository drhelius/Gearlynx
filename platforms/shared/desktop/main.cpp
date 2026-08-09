/*
 * Gearlynx - Lynx Emulator
 * Copyright (C) 2025  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include "gearlynx.h"
#include "application.h"
#include "application_headless.h"
#include "config.h"
#include "console_utils.h"

extern bool g_mcp_stdio_mode;
extern bool g_mcp_router_enabled;

int main(int argc, char* argv[])
{
    attach_parent_console(argc, argv);

    ApplicationParams app_params;
    bool show_usage = false;
    int ret = 0;
    bool mcp_stdio_set = false;
    bool mcp_http_set = false;
    bool headless = false;
    bool portable = false;
    bool comlynx_host_set = false;
    bool comlynx_join_set = false;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-' || strcmp(argv[i], "/?") == 0)
        {
            if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "-?") == 0) ||
                (strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "/?") == 0))
            {
                show_usage = true;
                ret = 0;
            }
            else if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--version") == 0))
            {
                printf("%s\n", GLYNX_TITLE_ASCII);
                printf("Build: %s\n", GLYNX_VERSION);
                printf("Author: Ignacio Sánchez (drhelius)\n");
                return 0;
            }
            else if ((strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--fullscreen") == 0))
            {
                app_params.force_fullscreen = true;
            }
            else if ((strcmp(argv[i], "-w") == 0) || (strcmp(argv[i], "--windowed") == 0))
            {
                app_params.force_windowed = true;
            }
            else if (strcmp(argv[i], "--mcp-stdio") == 0)
            {
                g_mcp_stdio_mode = true;  // Disable logging immediately
                mcp_stdio_set = true;
                app_params.mcp_mode = 0;
            }
            else if (strcmp(argv[i], "--mcp-http") == 0)
            {
                mcp_http_set = true;
                app_params.mcp_mode = 1;
            }
            else if ((strcmp(argv[i], "--mcp-router") == 0) || (strcmp(argv[i], "--mcp-enable-router") == 0))
            {
                g_mcp_router_enabled = true;
            }
            else if (strcmp(argv[i], "--headless") == 0)
            {
                headless = true;
            }
            else if (strcmp(argv[i], "--portable") == 0)
            {
                portable = true;
            }
            else if (strcmp(argv[i], "--mcp-http-port") == 0)
            {
                if (i + 1 >= argc || argv[i + 1][0] == '-')
                {
                    fprintf(stderr, "Missing value for --mcp-http-port\n");
                    return -1;
                }

                char* end = NULL;
                long port = strtol(argv[++i], &end, 10);
                if (!end || *end != '\0' || port <= 0 || port > 65535)
                {
                    fprintf(stderr, "Invalid port number: %s\n", argv[i]);
                    return -1;
                }
                app_params.mcp_tcp_port = (int)port;
                app_params.mcp_tcp_port_set = true;
            }
            else if (strcmp(argv[i], "--mcp-http-address") == 0)
            {
                if (i + 1 >= argc || argv[i + 1][0] == '-')
                {
                    fprintf(stderr, "Missing value for --mcp-http-address\n");
                    return -1;
                }

                app_params.mcp_http_address = argv[++i];
                app_params.mcp_http_address_set = true;
            }
            else if (strcmp(argv[i], "--debug-monitor") == 0)
            {
                app_params.debug_monitor_port = 6502;
            }
            else if (strcmp(argv[i], "--debug-monitor-port") == 0)
            {
                if (i + 1 >= argc || argv[i + 1][0] == '-')
                {
                    fprintf(stderr, "Missing value for --debug-monitor-port\n");
                    return -1;
                }

                char* end = NULL;
                long port = strtol(argv[++i], &end, 10);
                if (!end || *end != '\0' || port <= 0 || port > 65534)
                {
                    fprintf(stderr, "Invalid debug monitor port: %s\n", argv[i]);
                    return -1;
                }
                app_params.debug_monitor_port = (int)port;
            }
            else if (strcmp(argv[i], "--comlynx-host") == 0)
            {
                comlynx_host_set = true;
                app_params.comlynx_mode = 0;
            }
            else if (strcmp(argv[i], "--comlynx-join") == 0)
            {
                if (i + 1 >= argc || argv[i + 1][0] == '-')
                {
                    fprintf(stderr, "Missing value for --comlynx-join\n");
                    return -1;
                }
                comlynx_join_set = true;
                app_params.comlynx_mode = 1;
                app_params.comlynx_host = argv[++i];
            }
            else if (strcmp(argv[i], "--comlynx-port") == 0)
            {
                if (i + 1 >= argc || argv[i + 1][0] == '-')
                {
                    fprintf(stderr, "Missing value for --comlynx-port\n");
                    return -1;
                }

                char* end = NULL;
                long port = strtol(argv[++i], &end, 10);
                if (!end || *end != '\0' || port <= 0 || port > 65535)
                {
                    fprintf(stderr, "Invalid ComLynx port: %s\n", argv[i]);
                    return -1;
                }
                app_params.comlynx_port = (int)port;
                app_params.comlynx_port_set = true;
            }
            else if (strcmp(argv[i], "--comlynx-bind") == 0)
            {
                if (i + 1 >= argc || argv[i + 1][0] == '-')
                {
                    fprintf(stderr, "Missing value for --comlynx-bind\n");
                    return -1;
                }
                app_params.comlynx_bind_address = argv[++i];
                app_params.comlynx_bind_address_set = true;
            }
            else
            {
                printf("Unknown option: %s\n", argv[i]);
                show_usage = true;
                ret = -1;
            }
        }
    }

    int non_option_count = 0;
    for (int i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "--mcp-http-port") == 0) ||
            (strcmp(argv[i], "--mcp-http-address") == 0) ||
            (strcmp(argv[i], "--debug-monitor-port") == 0) ||
            (strcmp(argv[i], "--comlynx-join") == 0) ||
            (strcmp(argv[i], "--comlynx-port") == 0) ||
            (strcmp(argv[i], "--comlynx-bind") == 0))
        {
            if (i + 1 < argc)
                i++;
            continue;
        }

        if (argv[i][0] != '-' && strcmp(argv[i], "/?") != 0)
        {
            if (non_option_count == 0)
                app_params.rom_file = argv[i];
            else if (non_option_count == 1)
                app_params.symbol_file = argv[i];

            non_option_count++;

            if (non_option_count > 2)
            {
                show_usage = true;
                ret = -1;
                break;
            }
        }
    }

    if (mcp_stdio_set && mcp_http_set)
    {
        printf("Error: Cannot use both --mcp-stdio and --mcp-http at the same time\n");
        return -1;
    }

    if (comlynx_host_set && comlynx_join_set)
    {
        printf("Error: Cannot use both --comlynx-host and --comlynx-join at the same time\n");
        return -1;
    }

    if (show_usage)
    {
        printf("Usage: %s [options] [game_file] [symbol_file]\n", argv[0]);
        printf("  [game_file]         Game file: accepts ROMs (.lyx, .lnx, .o) or ZIP (.zip)\n");
        printf("\nOptions:\n");
        printf("  -f, --fullscreen      Start in fullscreen mode\n");
        printf("  -w, --windowed        Start in windowed mode with menu visible\n");
        printf("      --mcp-stdio       Auto-start MCP server with stdio transport\n");
        printf("      --mcp-http        Auto-start MCP server with HTTP transport\n");
        printf("      --mcp-router      Enable compact MCP tool routing\n");
        printf("      --mcp-http-address A HTTP bind address (default: 127.0.0.1)\n");
        printf("      --mcp-http-port N HTTP port for MCP server (default: 7777)\n");
        printf("      --debug-monitor       Start debug monitor TCP server (default port: 6502)\n");
        printf("      --debug-monitor-port N Debug monitor port, 1-65534 (default: 6502)\n");
        printf("      --comlynx-host    Host a ComLynx UDP session\n");
        printf("      --comlynx-join HOST Join a ComLynx UDP session\n");
        printf("      --comlynx-port N  ComLynx UDP port (default: %d)\n", COMLYNX_DEFAULT_PORT);
        printf("      --comlynx-bind ADDRESS ComLynx host bind address (default: 0.0.0.0)\n");
        printf("      --headless        Run without GUI (requires MCP, debug monitor, or ComLynx)\n");
        printf("      --portable        Store configuration and user data beside the application\n");
        printf("  -v, --version         Display version information\n");
        printf("  -h, --help            Display this help message\n");
        return ret;
    }

    if (app_params.force_fullscreen && app_params.force_windowed)
        app_params.force_fullscreen = false;

    config_init(portable);
    config_read();

    if (app_params.mcp_tcp_port_set)
        config_emulator.mcp_tcp_port = app_params.mcp_tcp_port;
    else
        app_params.mcp_tcp_port = config_emulator.mcp_tcp_port;

    if (app_params.mcp_http_address_set)
        config_emulator.mcp_http_address = app_params.mcp_http_address;
    else
        app_params.mcp_http_address = config_emulator.mcp_http_address;

    if (app_params.comlynx_port_set)
        config_emulator.comlynx_port = app_params.comlynx_port;
    else
        app_params.comlynx_port = config_emulator.comlynx_port;

    if (app_params.comlynx_bind_address_set)
        config_emulator.comlynx_bind_address = app_params.comlynx_bind_address;
    else
        app_params.comlynx_bind_address = config_emulator.comlynx_bind_address;

    if (app_params.comlynx_mode == 1)
        config_emulator.comlynx_host = app_params.comlynx_host;

    if (headless)
    {
        ret = application_headless_init(app_params);

        if (ret == 0)
        {
            application_headless_mainloop();
            application_headless_destroy();
        }

        config_write();
        config_destroy();

        return ret;
    }

    if (!application_check_single_instance(app_params.rom_file, app_params.symbol_file))
    {
        config_destroy();
        return 0;
    }

    ret = application_init(app_params);

    if (ret == 0)
        application_mainloop();

    application_destroy();

    config_write();
    config_destroy();

    return ret;
}