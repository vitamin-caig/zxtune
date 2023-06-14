/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - memory.c                                                *
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

#include "memory.h"
#include "memory_io.h"
#include "memory_mmu.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"

#include "main/main.h"
#include "main/rom.h"

#include "r4300/r4300.h"
#include "r4300/r4300_core.h"
#include "r4300/cached_interp.h"
#include "r4300/ops.h"
#include "r4300/tlb.h"

#include "rdp/rdp_core.h"
#include "rsp/rsp_core.h"

#include "ai/ai_controller.h"
#include "pi/pi_controller.h"
#include "ri/ri_controller.h"
#include "si/si_controller.h"
#include "vi/vi_controller.h"

#ifdef DBG
#include "debugger/dbg_types.h"
#include "debugger/dbg_memory.h"
#include "debugger/dbg_breakpoints.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "osal/preproc.h"

//MurmurHash2
static unsigned int Hash(const void* blob, unsigned len)
{
  const unsigned int m = 0x5bd1e995;
  const unsigned int seed = 0;
  const int r = 24;

  unsigned int h = seed ^ len;

  const unsigned char* data = (const unsigned char*)blob;
  while (len >= 4)
  {
      unsigned int k;
      k  = data[0];
      k |= data[1] << 8;
      k |= data[2] << 16;
      k |= data[3] << 24;

      k *= m;
      k ^= k >> r;
      k *= m;

      h *= m;
      h ^= k;

      data += 4;
      len -= 4;
    }

  switch (len)
  {
  case 3:
      h ^= data[2] << 16;
  case 2:
      h ^= data[1] << 8;
  case 1:
      h ^= data[0];
      h *= m;
  };

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

void osal_fastcall invalidate_code(usf_state_t* state, uint32_t address)
{
#ifdef DEBUG_INFO
    if (state->r4300emu != CORE_PURE_INTERPRETER)
#endif
    {
       invalidate_block(state, address);
    }
}

#include <assert.h>

void map_code_region(usf_state_t* state, uint32_t vstart, uint32_t vend)
{
    uint32_t addr;
#ifdef DEBUG_INFO
    if (state->r4300emu == CORE_PURE_INTERPRETER)
    {
        return;
    }
#endif
    for (addr = vstart; addr < vend; addr += TLB_PAGE_SIZE)
    {
        const uint32_t page = addr / TLB_PAGE_SIZE;
        const precomp_block* const block = state->blocks[page];
        if (block && block->hash)
        {
            const uint32_t ph_start = state->io.tlb_LUT[page] & TLB_ADDRESS_MASK;
            assert(ph_start >= 0x80000000 && ph_start < 0x80000000 + RDRAM_MAX_SIZE - TLB_PAGE_SIZE);
            const uint32_t ram_offset = ph_start & (RDRAM_MAX_SIZE - 1);
            if (block->hash == Hash(state->g_rdram.dram + ram_offset / sizeof(uint32_t), TLB_PAGE_SIZE))
            {
                state->invalid_code[page] = 0;
            }
        }
    }
}

void unmap_code_region(usf_state_t* state, uint32_t vstart, uint32_t vend)
{
    uint32_t addr;
#ifdef DEBUG_INFO
    if (state->r4300emu == CORE_PURE_INTERPRETER)
    {
        return;
    }
#endif
    for (addr = vstart; addr < vend; addr += TLB_PAGE_SIZE)
    {
        const uint32_t page = addr / TLB_PAGE_SIZE;
        const uint32_t ph_start = state->io.tlb_LUT[page] & TLB_ADDRESS_MASK;
        const uint32_t ph_page = ph_start / TLB_PAGE_SIZE;
        if (!state->invalid_code[page] && (state->invalid_code[ph_page] || state->invalid_code[ph_page | (0x20000000 / TLB_PAGE_SIZE)]))
        {
            state->invalid_code[page] = 1;
        }
        if (!state->invalid_code[page])
        {
            assert(ph_start >= 0x80000000 && ph_start < 0x80000000 + RDRAM_MAX_SIZE - TLB_PAGE_SIZE);
            const uint32_t ram_offset = ph_start & (RDRAM_MAX_SIZE - 1);
            state->blocks[page]->hash = Hash(state->g_rdram.dram + ram_offset / sizeof(uint32_t), TLB_PAGE_SIZE);
                
            state->invalid_code[page] = 1;
        }
        else if (state->blocks[page])
        {
            state->blocks[page]->hash = 0;
        }
    }
}


//nothing
static uint32_t osal_fastcall io_read_nothing(usf_state_t* state, uint32_t addr)
{
    (void)state;
    (void)addr;
    return 0;
}

static void osal_fastcall io_write_nothing(usf_state_t* state, uint32_t addr, uint32_t value, uint32_t mask)
{
    (void)state;
    (void)addr;
    (void)value;
    (void)mask;
}

#define DECLARE_IO(member, suffix) \
static uint32_t osal_fastcall io_read_ ## suffix (usf_state_t* state, uint32_t addr) \
{ \
    return read_ ## suffix (&state-> member, addr); \
} \
\
static void osal_fastcall io_write_ ## suffix (usf_state_t* state, uint32_t addr, uint32_t value, uint32_t mask) \
{ \
    return write_ ## suffix (&state-> member, addr, value, mask); \
}

//ri
DECLARE_IO(g_rdram, rdram_dram)
DECLARE_IO(g_rdram, rdram_regs)
DECLARE_IO(g_ri, ri_regs)
DECLARE_IO(g_sp, rsp_mem)
DECLARE_IO(g_sp, rsp_regs)
DECLARE_IO(g_sp, rsp_regs2)
DECLARE_IO(g_dp, dpc_regs)
DECLARE_IO(g_dp, dps_regs)
DECLARE_IO(g_r4300, mi_regs)
DECLARE_IO(g_vi, vi_regs)
DECLARE_IO(g_ai, ai_regs)
DECLARE_IO(g_pi, pi_regs)
DECLARE_IO(g_pi, cart_rom)
DECLARE_IO(g_si, si_regs)
DECLARE_IO(g_si, pif_ram)

#undef DECLARE_IO

/* HACK: just to get F-Zero to boot
 * TODO: implement a real DD module
 */
static uint32_t osal_fastcall io_read_dd_regs(usf_state_t* state, uint32_t addr)
{
    (void)state;
    return (addr == 0xa5000508)
           ? 0xffffffff
           : 0x00000000;
}

static void osal_fastcall io_write_dd_regs(usf_state_t* state, uint32_t addr, uint32_t value, uint32_t mask)
{
    (void)state;
    (void)addr;
    (void)value;
    (void)mask;
}

typedef struct
{
    uint32_t first;
    uint32_t last;
    int type;
    read_func read;
    write_func write;
} memory_region;

static void map_single_region(io_bus* io, const memory_region* region)
{
    uint32_t addr;
    for (addr = region->first; addr <= region->last; addr += IO_SEGMENT_SIZE)
    {
        const uint32_t idx = addr / IO_SEGMENT_SIZE;
        io->read[idx] = region->read;
        io->write[idx] = region->write;
    }
}

static void map_mirrored_region(io_bus* io, const memory_region* region)
{
    uint32_t addr;
    for (addr = region->first; addr <= region->last; addr += IO_SEGMENT_SIZE)
    {
        const uint32_t idx = (addr | 0x20000000) / IO_SEGMENT_SIZE;
        io->read[idx] = region->read;
        io->write[idx] = region->write;
    }
}

static void map_regions(io_bus* io, const memory_region* region)
{
    while (region->read && region->write)
    {
        map_single_region(io, region);
        if (region->type != M64P_MEM_NOMEM)
        {
            map_mirrored_region(io, region);
        }
        ++region;
    }
}

#define R(x) io_read_ ## x
#define W(x) io_write_ ## x
#define RW(x) R(x), W(x)

static const memory_region MEMORY_MAP[] =
{
      //generic, not mirrored
      //{0x00000000, 0x7fffffff, M64P_MEM_NOMEM, RW(nomem)},
      //{0xc0000000, 0xffffffff, M64P_MEM_NOMEM, RW(nomem)},
      //other, mirrored
      //nothing
      {0x80000000, 0x9fffffff, M64P_MEM_NOTHING, RW(nothing)},
      //rdram regs
      {0x83f00000, 0x83f0ffff, M64P_MEM_RDRAMREG, RW(rdram_regs)},
      //rsp mem
      {0x84000000, 0x8400ffff, M64P_MEM_RSPMEM, RW(rsp_mem)},
      //rsp regs
      {0x84040000, 0x8404ffff, M64P_MEM_RSPREG, RW(rsp_regs)},
      {0x84080000, 0x8408ffff, M64P_MEM_RSP, RW(rsp_regs2)},
      //dpc regs
      {0x84100000, 0x8410ffff, M64P_MEM_DP, RW(dpc_regs)},
      //dps regs
      {0x84200000, 0x8420ffff, M64P_MEM_DPS, RW(dps_regs)},
      //mi regs
      {0x84300000, 0x8430ffff, M64P_MEM_MI, RW(mi_regs)},
      //vi regs
      {0x84400000, 0x8440ffff, M64P_MEM_VI, RW(vi_regs)},
      //ai regs
      {0x84500000, 0x8450ffff, M64P_MEM_AI, RW(ai_regs)},
      //pi regs
      {0x84600000, 0x8460ffff, M64P_MEM_PI, RW(pi_regs)},
      //ri regs
      {0x84700000, 0x8470ffff, M64P_MEM_RI, RW(ri_regs)},
      //si regs
      {0x84800000, 0x8480ffff, M64P_MEM_SI, RW(si_regs)},
      //dd regs
      {0x85000000, 0x8500ffff, M64P_MEM_NOTHING, RW(dd_regs)},
      //pif ram
      {0x9fc00000, 0x9fc0ffff, M64P_MEM_PIF, RW(pif_ram)},
      //end
      {0, 0, 0, 0, 0}
};

void create_memory(usf_state_t* state)
{
    uint32_t offset;
    state->g_rdram.dram = (uint32_t*) malloc(RDRAM_MAX_SIZE);
    state->g_rdram.dram_size = RDRAM_MAX_SIZE;
    state->io.read = (read_func*) calloc(sizeof(read_func), IO_SEGMENTS_COUNT);
    state->io.write = (write_func*) calloc(sizeof(write_func), IO_SEGMENTS_COUNT);
    state->io.tlb_LUT = (uint32_t*) calloc(sizeof(uint32_t), TLB_PAGES_COUNT);
    state->io.empty_space = (uint8_t*) malloc(IO_SEGMENT_SIZE + TLB_PAGE_SIZE);//to ensure that page is available
    for (offset = 0; offset < IO_SEGMENT_SIZE; offset += sizeof(uint32_t))
    {
        state->io.empty_space[offset] = (uint32_t)((offset << 16) | offset);
    }
}

void init_memory(usf_state_t * state, uint32_t rdram_size)
{
    if (rdram_size > RDRAM_MAX_SIZE)
    {
        rdram_size = RDRAM_MAX_SIZE;
    }

    const memory_region ram_rom[] =
    {
        {0x80000000, 0x80000000 + rdram_size - 1, M64P_MEM_RDRAM, RW(rdram_dram)},
        {0x90000000, 0x90000000 + state->g_rom_size - 1, M64P_MEM_ROM, RW(cart_rom)},
        {0, 0, 0, 0, 0}
    };

    map_regions(&state->io, MEMORY_MAP);
    map_regions(&state->io, ram_rom);
    state->g_rdram.dram_size = rdram_size;
    //fixup cart_rom
    const memory_region rom = {0x90000000, 0x90000000 + state->g_rom_size - 1, M64P_MEM_ROM, R(cart_rom), W(nothing)};
    map_single_region(&state->io, &rom);
    
    memory_reset_tlb(&state->io);
    
    if (state->g_rom && state->g_rom_size >= 0xfc0)
        init_cic_using_ipl3(state, &state->g_si.pif.cic, state->g_rom + 0x40);

    init_r4300(&state->g_r4300);
    init_rdp(&state->g_dp);
    init_rsp(&state->g_sp);
    init_rdram(&state->g_rdram);
    init_ai(&state->g_ai);
    init_pi(&state->g_pi);
    init_ri(&state->g_ri);
    init_si(&state->g_si);
    init_vi(&state->g_vi);

    DebugMessage(state, M64MSG_VERBOSE, "Memory initialized");
}

#define RELEASE(p) if (p) {free(p); p = 0;}

void shutdown_memory(usf_state_t* state)
{
    RELEASE(state->io.read);
    RELEASE(state->io.write);
    RELEASE(state->io.tlb_LUT);
    RELEASE(state->io.empty_space);
    RELEASE(state->g_rdram.dram);
}

void memory_reset_tlb(io_bus* io)
{
    memset(io->tlb_LUT, 0, sizeof(uint32_t) * TLB_PAGES_COUNT);
}

void memory_map_tlb(io_bus* io, int writable, uint32_t vstart, uint32_t vend, uint32_t pstart)
{
    uint32_t addr, phys;
    if (pstart < 0x20000000 && vstart < vend && is_mapped(vstart) && is_mapped(vend))
    {
        phys = 0x80000000 | (pstart & TLB_ADDRESS_MASK);
        if (writable)
        {
          phys |= TLB_PAGE_WRITABLE;
        }
        for (addr = vstart; addr < vend; phys += TLB_PAGE_SIZE, addr += TLB_PAGE_SIZE)
        {
            const uint32_t page = addr / TLB_PAGE_SIZE;
            io->tlb_LUT[page] = phys;
        }
    }
}

void memory_unmap_tlb(io_bus* io, uint32_t vstart, uint32_t vend)
{
    uint32_t addr;
    for (addr = vstart; addr < vend; addr += TLB_PAGE_SIZE)
    {
        const uint32_t page = addr / TLB_PAGE_SIZE;
        io->tlb_LUT[page] = INVALID_ADDR;
    }
}

const uint32_t* osal_fastcall fast_mem_access(usf_state_t * state, uint32_t address)
{
    /* This code is performance critical, specially on pure interpreter mode.
     * Removing error checking saves some time, but the emulator may crash. */
    if (is_mapped(address))
        address = virtual_to_physical_address(state, address, 2);

    address &= 0x1ffffffc;
        
    if (address < RDRAM_MAX_SIZE)
        return (const uint32_t*)((const uint8_t*)state->g_rdram.dram + address);
    else if (address >= 0x10000000)
    {
        if ((address - 0x10000000) < state->g_rom_size)
            return (const uint32_t*)((const uint8_t*)state->g_rom + address - 0x10000000);
        else
            return (const uint32_t*)(state->io.empty_space + (address & 0xFFFF));
    }
    else if ((address & 0xffffe000) == 0x04000000)
        return (const uint32_t*)((const uint8_t*)state->g_sp.mem + (address & 0x1ffc));
    else
        return NULL;
}
