/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - memory.h                                                *
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

#ifndef M64P_MEMORY_MEMORY_H
#define M64P_MEMORY_MEMORY_H

#include <stdint.h>
#include "osal/preproc.h"

typedef struct usf_state usf_state_t;

enum { 
  RDRAM_MAX_SIZE = 0x800000,
  IO_SEGMENT_SIZE = 0x10000,
  TLB_PAGE_SIZE = 0x1000,
  TLB_OFFSET_MASK = TLB_PAGE_SIZE - 1,
  TLB_ADDRESS_MASK = ~TLB_OFFSET_MASK,
  
  IO_SEGMENTS_COUNT = 0x10000,
  TLB_PAGES_COUNT = 0x100000,
  
  TLB_PAGE_WRITABLE = 1,
  
  INVALID_ADDR = 0x00000000
};

typedef uint32_t (osal_fastcall *read_func) (usf_state_t* state, uint32_t addr);
typedef void (osal_fastcall *write_func) (usf_state_t* state, uint32_t addr, uint32_t value, uint32_t mask);

typedef struct io_bus
{
    //IO_SEGMENTS_COUNT entries
    read_func* read;
    //IO_SEGMENTS_COUNT entries
    write_func* write;
    //TLB_PAGES_COUNT entries
    uint32_t* tlb_LUT;
    //IO_SEGMENT_SIZE
    uint8_t* empty_space;
} io_bus;

void create_memory(usf_state_t*);

void init_memory(usf_state_t *, uint32_t rdram_size);

void shutdown_memory(usf_state_t*);

void osal_fastcall invalidate_code(usf_state_t * state, uint32_t address);
void map_code_region(usf_state_t* state, uint32_t vstart, uint32_t vend);
void unmap_code_region(usf_state_t* state, uint32_t vstart, uint32_t vend);

/* Returns a pointer to a block of contiguous memory
 * Can access RDRAM, SP_DMEM, SP_IMEM and ROM, using TLB if necessary
 * Useful for getting fast access to a zone with executable code. */
const uint32_t* osal_fastcall fast_mem_access(usf_state_t *, uint32_t address);

#endif

