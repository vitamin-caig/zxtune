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

#include "usf/barray.h"

#include "rdram.h"

#include "memory/memory.h"

#include <string.h>

void connect_rdram(struct rdram* rdram,
                   size_t dram_size)
{
    rdram->dram_size = dram_size;
}

void init_rdram(struct rdram* rdram)
{
    memset(rdram->regs, 0, RDRAM_REGS_COUNT*sizeof(uint32_t));
    memset(rdram->dram, 0, rdram->dram_size);
}


uint32_t read_rdram_regs(void* opaque, uint32_t address)
{
    struct rdram* rdram = (struct rdram*)opaque;
    uint32_t reg = rdram_reg(address);

    return rdram->regs[reg];
}

void write_rdram_regs(void* opaque, uint32_t address, uint32_t value, uint32_t mask)
{
    struct rdram* rdram = (struct rdram*)opaque;
    uint32_t reg = rdram_reg(address);

    masked_write(&rdram->regs[reg], value, mask);
}


uint32_t read_rdram_dram(void* opaque, uint32_t address)
{
    struct rdram* rdram = (struct rdram*)opaque;
    uint32_t addr = rdram_dram_address(address);

    return rdram->dram[addr];
}

void write_rdram_dram(void* opaque, uint32_t address, uint32_t value, uint32_t mask)
{
    struct rdram* rdram = (struct rdram*)opaque;
    uint32_t addr = rdram_dram_address(address);

    masked_write(&rdram->dram[addr], value, mask);
}

#ifndef NO_TRIMMING
uint32_t read_rdram_dram_tracked(void* opaque, uint32_t address)
{
    usf_state_t* state = (usf_state_t*) opaque;
    struct rdram* rdram = &state->g_rdram;
    uint32_t addr = rdram_dram_address(address);

    if (!bit_array_test(state->barray_ram_written_first, addr))
        bit_array_set(state->barray_ram_read, addr);

    return rdram->dram[addr];
}

void write_rdram_dram_tracked(void* opaque, uint32_t address, uint32_t value, uint32_t mask)
{
    usf_state_t* state = (usf_state_t*) opaque;
    struct rdram* rdram = &state->g_rdram;
    uint32_t addr = rdram_dram_address(address);

    if (mask == 0xFFFFFFFFU && !bit_array_test(state->barray_ram_read, addr))
        bit_array_set(state->barray_ram_written_first, addr);
    
    masked_write(&rdram->dram[addr], value, mask);
}
#endif

