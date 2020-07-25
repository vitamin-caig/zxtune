/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - tlb.c                                                   *
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

#include "tlb.h"

#include "memory/memory_mmu.h"
#include "usf/usf_internal.h"
#include "exception.h"

void tlb_unmap(usf_state_t * state, tlb *entry)
{
    if (entry->v_even)
    {
        unmap_code_region(state, entry->start_even, entry->end_even);
        memory_unmap_tlb(&state->io, entry->start_even, entry->end_even);
    }

    if (entry->v_odd)
    {
        unmap_code_region(state, entry->start_odd, entry->end_odd);
        memory_unmap_tlb(&state->io, entry->start_odd, entry->end_odd);
    }
}

void tlb_map(usf_state_t * state, tlb *entry)
{
    if (entry->v_even)
    {
        memory_map_tlb(&state->io, entry->d_even, entry->start_even, entry->end_even, entry->phys_even);
        map_code_region(state, entry->start_even, entry->end_even);
    }

    if (entry->v_odd)
    {
        memory_map_tlb(&state->io, entry->d_odd, entry->start_odd, entry->end_odd, entry->phys_odd);
        map_code_region(state, entry->start_odd, entry->end_odd);
    }
}

uint32_t virtual_to_physical_address(usf_state_t * state, uint32_t addr, int w)
{
    const uint32_t tlb = state->io.tlb_LUT[addr / TLB_PAGE_SIZE];
    if (tlb != INVALID_ADDR)
    {
        const uint32_t phys = (tlb & TLB_ADDRESS_MASK) | (addr & TLB_OFFSET_MASK);
  
        if (w != 1 || (tlb & TLB_PAGE_WRITABLE))
        {
            return phys;
        }
    }
    TLB_refill_exception(state, addr, w);
    return INVALID_ADDR;
}
