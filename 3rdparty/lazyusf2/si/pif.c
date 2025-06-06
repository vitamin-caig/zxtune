/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - pif.c                                                   *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "pif.h"
#include "n64_cic_nus_6105.h"
#include "si_controller.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "memory/memory.h"
#include "r4300/r4300_core.h"
#include "r4300/cp0.h"
#include "r4300/interupt.h"

#include <string.h>

//#define DEBUG_PIF
#ifdef DEBUG_PIF
void print_pif(usf_state_t * state, struct pif* pif)
{
    int i;
    for (i=0; i<(64/8); i++)
        DebugMessage(state, M64MSG_INFO, "%x %x %x %x | %x %x %x %x",
                     pif->ram[i*8+0], pif->ram[i*8+1],pif->ram[i*8+2], pif->ram[i*8+3],
                     pif->ram[i*8+4], pif->ram[i*8+5],pif->ram[i*8+6], pif->ram[i*8+7]);
}
#endif

static void process_cart_command(usf_state_t * state, struct pif* pif, uint8_t* cmd)
{
    switch (cmd[2])
    {
    case PIF_CMD_STATUS: break;
    case PIF_CMD_EEPROM_READ: break;
    case PIF_CMD_EEPROM_WRITE: break;
    case PIF_CMD_AF_RTC_STATUS: break;
    case PIF_CMD_AF_RTC_READ: break;
    case PIF_CMD_AF_RTC_WRITE: break;
    default:
        DebugMessage(state, M64MSG_ERROR, "unknown PIF command: %02x", cmd[2]);
    }
}

void init_pif(struct pif* pif)
{
    memset(pif->ram, 0, PIF_RAM_SIZE);
}

uint32_t read_pif_ram(void* opaque, uint32_t address)
{
    struct si_controller* si = (struct si_controller*)opaque;
    uint32_t addr = pif_ram_address(address);

    if (addr >= PIF_RAM_SIZE)
    {
        DebugMessage(si->r4300->state, M64MSG_ERROR, "Invalid PIF address: %08x", address);
        return 0;
    }

    return sl(*(uint32_t*)(&si->pif.ram[addr]));
}

void write_pif_ram(void* opaque, uint32_t address, uint32_t value, uint32_t mask)
{
    struct si_controller* si = (struct si_controller*)opaque;
    uint32_t addr = pif_ram_address(address);

    if (addr >= PIF_RAM_SIZE)
    {
        DebugMessage(si->r4300->state, M64MSG_ERROR, "Invalid PIF address: %08x", address);
        return;
    }

    masked_write((uint32_t*)(&si->pif.ram[addr]), sl(value), sl(mask));

    if ((addr == 0x3c) && (mask & 0xff))
    {
        if (si->pif.ram[0x3f] == 0x08)
        {
            si->pif.ram[0x3f] = 0;
            update_count(si->r4300->state);
            if (si->r4300->state->g_delay_si)
                add_interupt_event(si->r4300->state, SI_INT, /*0x100*/0x900);
            else
                signal_rcp_interrupt(si->r4300, SI_INT);
        }
        else
        {
            update_pif_write(si);
        }
    }
}


void update_pif_write(struct si_controller* si)
{
    struct pif* pif = &si->pif;

    char challenge[30], response[30];
    int i=0, channel=0;
    if (pif->ram[0x3F] > 1)
    {
        switch (pif->ram[0x3F])
        {
        case 0x02:
#ifdef DEBUG_PIF
            DebugMessage(si->r4300->state, M64MSG_INFO, "update_pif_write() pif_ram[0x3f] = 2 - CIC challenge");
#endif
            // format the 'challenge' message into 30 nibbles for X-Scale's CIC code
            for (i = 0; i < 15; i++)
            {
                challenge[i*2] =   (pif->ram[48+i] >> 4) & 0x0f;
                challenge[i*2+1] =  pif->ram[48+i]       & 0x0f;
            }
            // calculate the proper response for the given challenge (X-Scale's algorithm)
            n64_cic_nus_6105(challenge, response, CHL_LEN - 2);
            pif->ram[46] = 0;
            pif->ram[47] = 0;
            // re-format the 'response' into a byte stream
            for (i = 0; i < 15; i++)
            {
                pif->ram[48+i] = (response[i*2] << 4) + response[i*2+1];
            }
            // the last byte (2 nibbles) is always 0
            pif->ram[63] = 0;
            break;
        case 0x08:
#ifdef DEBUG_PIF
            DebugMessage(si->r4300->state, M64MSG_INFO, "update_pif_write() pif_ram[0x3f] = 8");
#endif
            pif->ram[0x3F] = 0;
            break;
        default:
            DebugMessage(si->r4300->state, M64MSG_ERROR, "error in update_pif_write(): %x", pif->ram[0x3F]);
        }
        return;
    }
    while (i<0x40)
    {
        switch (pif->ram[i])
        {
        case 0x00:
            channel++;
            if (channel > 6) i=0x40;
            break;
        case 0xFF:
            break;
        default:
            if (!(pif->ram[i] & 0xC0))
            {
                if (channel < 4)
                {
                    process_controller_command(&pif->controllers[channel], &pif->ram[i]);
                }
                else if (channel == 4)
                    process_cart_command(si->r4300->state, pif, &pif->ram[i]);
                else
                    DebugMessage(si->r4300->state, M64MSG_ERROR, "channel >= 4 in update_pif_write");
                i += pif->ram[i] + (pif->ram[(i+1)] & 0x3F) + 1;
                channel++;
            }
            else
                i=0x40;
        }
        i++;
    }

    //pif->ram[0x3F] = 0;
}

void update_pif_read(struct si_controller* si)
{
    struct pif* pif = &si->pif;

    int i=0, channel=0;
    while (i<0x40)
    {
        switch (pif->ram[i])
        {
        case 0x00:
            channel++;
            if (channel > 6) i=0x40;
            break;
        case 0xFE:
            i = 0x40;
            break;
        case 0xFF:
            break;
        case 0xB4:
        case 0x56:
        case 0xB8:
            break;
        default:
            if (!(pif->ram[i] & 0xC0))
            {
                if (channel < 4)
                {
                    read_controller(&pif->controllers[channel], &pif->ram[i]);
                }

                i += pif->ram[i] + (pif->ram[(i+1)] & 0x3F) + 1;
                channel++;
            }
            else
                i=0x40;
        }
        i++;
    }
}

