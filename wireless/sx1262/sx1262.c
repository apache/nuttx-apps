/******************************************************************************
 * apps/examples/sx126x_demo/sx126x_demo.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ******************************************************************************/

/******************************************************************************
 * Included Files
 ******************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <debug.h>
#include <poll.h>
#include <fcntl.h>

#include <nuttx/wireless/lpwan/sx126x.h>
#include <nuttx/wireless/ioctl.h>

/******************************************************************************
 * Definitions
 ******************************************************************************/

#define SX126X_DEMO_DEVPATH "/dev/lpw0"
#define SX126X_DEMO_DEF_FREQ  869250000
#define SX126X_DEMO_DEF_PWR   22
#define SX126X_DEMO_DEF_SF    10
#define SX126X_DEMO_DEF_BW    125

/******************************************************************************
 * Data types
 ******************************************************************************/

enum sx126x_demo_mode_e
{
    SX126X_DEMO_TX,
    SX126X_DEMO_RX,
    SX126X_DEMO_ECHO,
    SX126X_DEMO_TXRX
};

struct sx126x_demo_config_s
{
    enum sx126x_lora_sf_e sf;
    enum sx126x_lora_bw_e bw;
    int32_t pwr;
    uint32_t frequency_hz;
    char *payload;
    size_t payload_size;
    enum sx126x_demo_mode_e mode;
};

/******************************************************************************
 * Globals
 ******************************************************************************/

/******************************************************************************
 * Public Functions
 ******************************************************************************/

enum sx126x_lora_bw_e to_nearest_lora_bw(int bw)
{
    static const int16_t possible_bws[] =
            {
                    7, 10, 15, 20, 31, 41, 62, 125, 250, 500
            };

    /* Find nearest bandwidth */

    int16_t record_deviation = INT16_MAX;
    size_t record_index     = 0;

    for (size_t i = 0; i < sizeof(possible_bws) / sizeof(possible_bws[0]); i++)
    {
        /* Compare bandwidth */

        int16_t possible_bw = possible_bws[i];
        int16_t deviation = abs(bw - possible_bw);

        /* Is this a better value? */

        if (deviation < record_deviation)
        {
            /* New record deviation */

            record_deviation = deviation;
            record_index     = i;

            if (deviation == 0)
            {
                /* It is perfect */

                break;
            }
        }
    }

    int16_t nearest_bw = possible_bws[record_index];

    /* Did not find the perfect match, but got nearest */

    if (record_deviation != 0)
    {
        printf("Bandwidth %dkHz is not available. \
             Using nearest available: %dkHz\n",
               bw, nearest_bw);
    }

    /* Convert to bandwidth code enum */

    static const enum sx126x_lora_bw_e bw_enum_map[] =
            {
                    SX126X_LORA_BW_7, SX126X_LORA_BW_10, SX126X_LORA_BW_15,
                    SX126X_LORA_BW_20, SX126X_LORA_BW_31, SX126X_LORA_BW_41,
                    SX126X_LORA_BW_62, SX126X_LORA_BW_125, SX126X_LORA_BW_250,
                    SX126X_LORA_BW_500
            };

    return bw_enum_map[record_index];
}

void get_flags(int argc, FAR char *argv[], struct sx126x_demo_config_s *config)
{
    /* For every arg, get flag */

    for (int i = 0; i < argc; i++)
    {
        char *arg = argv[i];

        /* Flag must start with - */

        if (arg[0] != '-')
        {
            continue;
        }

        /* Read flag */

        char flag = arg[1];
        switch (flag)
        {
            /* Is terminator */

            case '\0':
            {
                break;
            }

                /* Is frequency */

            case 'f':
            {
                /* Get value */

                if (i < argc - 1)
                {
                    uint32_t freq;
                    freq = strtoul(argv[++i], NULL, 0);
                    config->frequency_hz = freq;
                }
                break;
            }

                /* Is spreading factor */

            case 's':
            {
                /* Get value */

                if (i < argc - 1)
                {
                    uint32_t sf;
                    sf = strtoul(argv[++i], NULL, 0);
                    config->sf = sf;
                }
                break;
            }

                /* Is power */

            case 'p':
            {
                /* Get value */

                if (i < argc - 1)
                {
                    int32_t power;
                    power = strtol(argv[++i], NULL, 0);
                    config->pwr = power;
                }
                break;
            }

                /* Is bandwidth */

            case 'b':
            {
                /* Get value */

                if (i < argc - 1)
                {
                    int bw;
                    bw = strtol(argv[++i], NULL, 0);
                    config->bw = to_nearest_lora_bw(bw);
                }
                break;
            }

                /* Transmit */

            case 't':
            {
                /* Get string */

                if (i < argc - 1)
                {
                    config->mode = SX126X_DEMO_TX;
                    config->payload = argv[++i];
                    config->payload_size = strlen(config->payload);
                }
                break;
            }

                /* Receive */

            case 'r':
            {
                /* Enable */

                config->mode = SX126X_DEMO_RX;
                break;
            }

                /* Transmit and Receive */

            case 'x':
            {
                /* Get string */

                if (i < argc - 1)
                {
                    config->mode = SX126X_DEMO_TXRX;
                    config->payload = argv[++i];
                    config->payload_size = strlen(config->payload);
                }
                break;
            }
                /* Echo back */

            case 'e':
            {
                /* Enable */

                config->mode = SX126X_DEMO_ECHO;
                break;
            }

                /* Unknown flag */

            default:
            {
                printf("Unknown flag \"%c\"\n", flag);
                break;
            }
        }
    }
}

void set_sx_defaults(struct sx126x_demo_config_s *config)
{
    config->bw = to_nearest_lora_bw(SX126X_DEMO_DEF_BW);
    config->sf = SX126X_DEMO_DEF_SF;
    config->frequency_hz = SX126X_DEMO_DEF_FREQ;
    config->mode = SX126X_DEMO_TX;
}

void list_config(struct sx126x_demo_config_s *config)
{
    printf("Frequency: %u\n", config->frequency_hz);
    printf("Spreading factor: %d\n", config->sf);
    printf("Bandwidth 0x%X\n", config->bw);
}

int open_sx(struct sx126x_demo_config_s *config)
{
    int fd;

    fd = open(SX126X_DEMO_DEVPATH, O_RDWR);
    if (fd < 0)
    {
        int errcode = errno;
        printf("ERROR: Failed to open device %s: %d\n",
               SX126X_DEMO_DEVPATH, errcode);
        return -1;
    }

    /* Config */

    uint32_t freq = config->frequency_hz;
    ioctl(fd, WLIOC_SETRADIOFREQ, &freq);

    int pwr = config->pwr;
    ioctl(fd, WLIOC_SETTXPOWER, &pwr);

    struct sx126x_lora_config_s deviceconfig =
            {
                    .modulation =
                            {
                                    .bandwidth                   = config->bw,
                                    .coding_rate                 = SX126X_LORA_CR_4_8,
                                    .low_datarate_optimization   = false,
                                    .spreading_factor            = config->sf
                            },
                    .packet =
                            {
                                    .crc_enable                  = true,
                                    .fixed_length_header         = false,
                                    .payload_length              = 0xff,
                                    .invert_iq                   = false,
                                    .preambles                   = 8
                            }
            };

    ioctl(fd, SX126XIOC_LORACONFIGSET, &deviceconfig);

    return fd;
}

void demo_tx(int fd, struct sx126x_demo_config_s *config)
{
    printf("Transmitting...");
    write(fd, config->payload, config->payload_size);
    printf("\n\r");
}

void demo_rx(int fd, struct sx126x_demo_config_s *config)
{
    struct sx126x_read_header_s r_hdr;
    printf("Receiving...");
    int success = read(fd, &r_hdr, sizeof(r_hdr));

    if (success < 0)
    {
        printf("RX error\n");
        return;
    }

    if (r_hdr.crc_error)
    {
        printf("CRC error\n");
        return;
    }

    printf("Received\n");
    printf("Got %u bytes, RSSI %d, SNR %d\n", r_hdr.payload_length, r_hdr.rssi_db, r_hdr.snr);

    for (size_t i=0;i<r_hdr.payload_length;i++)
    {
        printf("%c", r_hdr.payload[i]);
    }
    printf("\n");

}

void demo_echo(int fd, struct sx126x_demo_config_s *config)
{
    for (;;)
    {
        struct sx126x_read_header_s r_hdr;
        printf("Receiving...");
        int success = read(fd, &r_hdr, sizeof(r_hdr));
        if (success < 0)
        {
            printf("RX error\n");
            return;
        }

        if (r_hdr.crc_error)
        {
            printf("CRC error\n");
            continue;
        }

        printf("Received\n");
        printf("Got %u bytes, RSSI %d, SNR %d\n", r_hdr.payload_length, r_hdr.rssi_db, r_hdr.snr);

        for (size_t i=0;i<r_hdr.payload_length;i++)
        {
            printf("%c", r_hdr.payload[i]);
        }
        printf("\nEcho TX...");

        write(fd, r_hdr.payload, r_hdr.payload_length);

        if (success < 0)
        {
            printf("TX error\n");
            return;
        }

        printf("Done\n");
    }
}

int main(int argc, FAR char *argv[])
{
    struct sx126x_demo_config_s config;

    /* Prepare */

    get_flags(argc, argv, &config);
    list_config(&config);

    /* Open device */

    int fd = open_sx(&config);

    switch (config.mode)
    {
        case SX126X_DEMO_TX:
        {
            demo_tx(fd, &config);
            break;
        }

        case SX126X_DEMO_RX:
        {
            demo_rx(fd, &config);
            break;
        }

        case SX126X_DEMO_TXRX:
        {
            demo_tx(fd, &config);
            demo_rx(fd, &config);
            break;
        }

        case SX126X_DEMO_ECHO:
        {
            demo_echo(fd, &config);
            break;
        }
    }

    /* Close */

    close(fd);

    return 0;
}