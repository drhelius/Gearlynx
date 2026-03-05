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

#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
 */

struct retro_core_option_v2_category option_cats_us[] = {
    {
        "system",
        "System",
        "Configure console type and other system-level settings."
    },
    {
        "video",
        "Video",
        "Configure aspect ratio and screen rotation settings."
    },
    {
        "audio",
        "Audio",
        "Configure low-pass filter and channel volume settings."
    },
    {
        "input",
        "Input",
        "Configure controller behavior and other input settings."
    },
    { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {

    /* System */

    {
        "gearlynx_console_type",
        "Console Type",
        NULL,
        "Select the Atari Lynx console model to emulate. 'Auto' automatically selects the appropriate console type based on the loaded game. 'Lynx I' forces emulation of the original Lynx model. 'Lynx II' forces emulation of the revised Lynx II model.",
        NULL,
        "system",
        {
            { "Auto",   NULL },
            { "Lynx I", NULL },
            { "Lynx II", NULL },
            { NULL, NULL },
        },
        "Auto"
    },

    /* Video */

    {
        "gearlynx_aspect_ratio",
        "Aspect Ratio",
        NULL,
        "Select which aspect ratio will be presented by the core. '1:1 PAR' selects an aspect ratio that produces square pixels. '4:3 DAR' forces a 4:3 display aspect ratio. '16:9 DAR' forces a 16:9 widescreen ratio. '16:10 DAR' forces a 16:10 widescreen ratio.",
        NULL,
        "video",
        {
            { "1:1 PAR",  NULL },
            { "4:3 DAR",  NULL },
            { "16:9 DAR", NULL },
            { "16:10 DAR", NULL },
            { NULL, NULL },
        },
        "1:1 PAR"
    },
    {
        "gearlynx_rotation",
        "Screen Rotation",
        NULL,
        "Rotates the screen display. Many Lynx games were designed to be played with the console held vertically. 'Auto' automatically rotates based on the game. 'Left' rotates the screen 90 degrees counter-clockwise. 'Right' rotates the screen 90 degrees clockwise. 'Disabled' forces the screen to remain in standard horizontal orientation.",
        NULL,
        "video",
        {
            { "Auto",     NULL },
            { "Left",     NULL },
            { "Right",    NULL },
            { "Disabled", NULL },
            { NULL, NULL },
        },
        "Auto"
    },

    /* Audio */

    {
        "gearlynx_lowpass_filter",
        "Audio Low-Pass Filter (Hz)",
        "Low-Pass Filter (Hz)",
        "Configure a low-pass audio filter to reduce high-frequency noise. The value sets the cutoff frequency in Hz. Lower values produce a warmer, more muffled sound.",
        "Set the cutoff frequency. Lower values produce a warmer sound.",
        "audio",
        {
            { "500",  NULL },
            { "1000", NULL },
            { "1500", NULL },
            { "2000", NULL },
            { "2500", NULL },
            { "3000", NULL },
            { "3500", NULL },
            { "4000", NULL },
            { "4500", NULL },
            { "5000", NULL },
            { NULL, NULL },
        },
        "3500"
    },
    {
        "gearlynx_audio_ch0_volume",
        "Audio Channel 0 Volume",
        "Channel 0 Volume",
        "Set the volume level for audio channel 0. The value is a percentage from 0 to 200, where 100 is the default volume.",
        "Set volume for channel 0 (0-200%).",
        "audio",
        {
            { "0",   NULL },
            { "10",  NULL },
            { "20",  NULL },
            { "30",  NULL },
            { "40",  NULL },
            { "50",  NULL },
            { "60",  NULL },
            { "70",  NULL },
            { "80",  NULL },
            { "90",  NULL },
            { "100", NULL },
            { "110", NULL },
            { "120", NULL },
            { "130", NULL },
            { "140", NULL },
            { "150", NULL },
            { "160", NULL },
            { "170", NULL },
            { "180", NULL },
            { "190", NULL },
            { "200", NULL },
            { NULL, NULL },
        },
        "100"
    },
    {
        "gearlynx_audio_ch1_volume",
        "Audio Channel 1 Volume",
        "Channel 1 Volume",
        "Set the volume level for audio channel 1. The value is a percentage from 0 to 200, where 100 is the default volume.",
        "Set volume for channel 1 (0-200%).",
        "audio",
        {
            { "0",   NULL },
            { "10",  NULL },
            { "20",  NULL },
            { "30",  NULL },
            { "40",  NULL },
            { "50",  NULL },
            { "60",  NULL },
            { "70",  NULL },
            { "80",  NULL },
            { "90",  NULL },
            { "100", NULL },
            { "110", NULL },
            { "120", NULL },
            { "130", NULL },
            { "140", NULL },
            { "150", NULL },
            { "160", NULL },
            { "170", NULL },
            { "180", NULL },
            { "190", NULL },
            { "200", NULL },
            { NULL, NULL },
        },
        "100"
    },
    {
        "gearlynx_audio_ch2_volume",
        "Audio Channel 2 Volume",
        "Channel 2 Volume",
        "Set the volume level for audio channel 2. The value is a percentage from 0 to 200, where 100 is the default volume.",
        "Set volume for channel 2 (0-200%).",
        "audio",
        {
            { "0",   NULL },
            { "10",  NULL },
            { "20",  NULL },
            { "30",  NULL },
            { "40",  NULL },
            { "50",  NULL },
            { "60",  NULL },
            { "70",  NULL },
            { "80",  NULL },
            { "90",  NULL },
            { "100", NULL },
            { "110", NULL },
            { "120", NULL },
            { "130", NULL },
            { "140", NULL },
            { "150", NULL },
            { "160", NULL },
            { "170", NULL },
            { "180", NULL },
            { "190", NULL },
            { "200", NULL },
            { NULL, NULL },
        },
        "100"
    },
    {
        "gearlynx_audio_ch3_volume",
        "Audio Channel 3 Volume",
        "Channel 3 Volume",
        "Set the volume level for audio channel 3. The value is a percentage from 0 to 200, where 100 is the default volume.",
        "Set volume for channel 3 (0-200%).",
        "audio",
        {
            { "0",   NULL },
            { "10",  NULL },
            { "20",  NULL },
            { "30",  NULL },
            { "40",  NULL },
            { "50",  NULL },
            { "60",  NULL },
            { "70",  NULL },
            { "80",  NULL },
            { "90",  NULL },
            { "100", NULL },
            { "110", NULL },
            { "120", NULL },
            { "130", NULL },
            { "140", NULL },
            { "150", NULL },
            { "160", NULL },
            { "170", NULL },
            { "180", NULL },
            { "190", NULL },
            { "200", NULL },
            { NULL, NULL },
        },
        "100"
    },

    /* Input */

    {
        "gearlynx_up_down_allowed",
        "Allow Up+Down / Left+Right",
        NULL,
        "Allow pressing, quickly alternating, or holding both left and right (or up and down) directions at the same time. This may cause movement based glitches in certain games. It's best to keep this option disabled.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },

    { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

struct retro_core_options_v2 options_us = {
    option_cats_us,
    option_defs_us
};

/*
 ********************************
 * Functions
 ********************************
 */

static void libretro_set_core_options(retro_environment_t environ_cb,
        bool *categories_supported)
{
    unsigned version = 0;

    if (!environ_cb || !categories_supported)
        return;

    *categories_supported = false;

    if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
        version = 0;

    if (version >= 2)
    {
        *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
                &options_us);
    }
    else
    {
        size_t i, j;
        size_t option_index  = 0;
        size_t num_options   = 0;
        struct retro_core_option_definition *option_v1_defs_us = NULL;
        struct retro_variable *variables   = NULL;
        char **values_buf                  = NULL;

        while (true)
        {
            if (option_defs_us[num_options].key)
                num_options++;
            else
                break;
        }

        if (version >= 1)
        {
            option_v1_defs_us = (struct retro_core_option_definition *)
                    calloc(num_options + 1, sizeof(struct retro_core_option_definition));

            for (i = 0; i < num_options; i++)
            {
                struct retro_core_option_v2_definition *option_def_us = &option_defs_us[i];
                struct retro_core_option_value *option_values         = option_def_us->values;
                struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
                struct retro_core_option_value *option_v1_values      = option_v1_def_us->values;

                option_v1_def_us->key           = option_def_us->key;
                option_v1_def_us->desc          = option_def_us->desc;
                option_v1_def_us->info          = option_def_us->info;
                option_v1_def_us->default_value = option_def_us->default_value;

                while (option_values->value)
                {
                    option_v1_values->value = option_values->value;
                    option_v1_values->label = option_values->label;

                    option_values++;
                    option_v1_values++;
                }
            }

            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
        }
        else
        {
            variables  = (struct retro_variable *)calloc(num_options + 1,
                    sizeof(struct retro_variable));
            values_buf = (char **)calloc(num_options, sizeof(char *));

            if (!variables || !values_buf)
                goto error;

            for (i = 0; i < num_options; i++)
            {
                const char *key                        = option_defs_us[i].key;
                const char *desc                       = option_defs_us[i].desc;
                const char *default_value              = option_defs_us[i].default_value;
                struct retro_core_option_value *values  = option_defs_us[i].values;
                size_t buf_len                         = 3;
                size_t default_index                   = 0;

                values_buf[i] = NULL;

                if (desc)
                {
                    size_t num_values = 0;

                    while (true)
                    {
                        if (values[num_values].value)
                        {
                            if (default_value)
                                if (strcmp(values[num_values].value, default_value) == 0)
                                    default_index = num_values;

                            buf_len += strlen(values[num_values].value);
                            num_values++;
                        }
                        else
                            break;
                    }

                    if (num_values > 0)
                    {
                        buf_len += num_values - 1;
                        buf_len += strlen(desc);

                        values_buf[i] = (char *)calloc(buf_len, sizeof(char));
                        if (!values_buf[i])
                            goto error;

                        strcpy(values_buf[i], desc);
                        strcat(values_buf[i], "; ");

                        strcat(values_buf[i], values[default_index].value);

                        for (j = 0; j < num_values; j++)
                        {
                            if (j != default_index)
                            {
                                strcat(values_buf[i], "|");
                                strcat(values_buf[i], values[j].value);
                            }
                        }
                    }
                }

                variables[option_index].key   = key;
                variables[option_index].value = values_buf[i];
                option_index++;
            }

            environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
        }

error:
        if (option_v1_defs_us)
        {
            free(option_v1_defs_us);
            option_v1_defs_us = NULL;
        }

        if (values_buf)
        {
            for (i = 0; i < num_options; i++)
            {
                if (values_buf[i])
                {
                    free(values_buf[i]);
                    values_buf[i] = NULL;
                }
            }

            free(values_buf);
            values_buf = NULL;
        }

        if (variables)
        {
            free(variables);
            variables = NULL;
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif
