/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rdram.c                                                 *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
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

#include "rdram.h"
#include "ri_controller.h"

#include <string.h>

void connect_rdram(struct rdram* rdram,
                   uint32_t* dram,
                   size_t dram_size)
{
    rdram->dram = dram;
    rdram->dram_size = dram_size;
}

void init_rdram(struct rdram* rdram)
{
    memset(rdram->regs, 0, RDRAM_REGS_COUNT*sizeof(uint32_t));
    memset(rdram->dram, 0, rdram->dram_size);
}

static osal_inline uint32_t rdram_reg(uint32_t address)
{
    return (address & 0x3ff) >> 2;
}

uint32_t read_rdram_regs(struct rdram* rdram, uint32_t address)
{
    const uint32_t reg = rdram_reg(address);

    return rdram->regs[reg];
}

void write_rdram_regs(struct rdram* rdram, uint32_t address, uint32_t value, uint32_t mask)
{
    const uint32_t reg = rdram_reg(address);

    masked_write(&rdram->regs[reg], value, mask);
}
