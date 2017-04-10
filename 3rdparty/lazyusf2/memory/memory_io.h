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

#ifndef M64P_MEMORY_MEMORY_IO_H
#define M64P_MEMORY_MEMORY_IO_H

#include "usf/usf_internal.h"
#include "r4300/tlb.h"

#define read_word_from_memory(addr) readmemw(state, (addr))
#define read_byte_from_memory(addr) readmemb(state, (addr))
#define read_hword_from_memory(addr) readmemh(state, (addr))
#define read_dword_from_memory(addr) readmemd(state, (addr))
#define write_word_to_memory(addr, value) writememw(state, (addr), (value))
#define write_byte_to_memory(addr, value) writememb(state, (addr), (value))
#define write_hword_to_memory(addr, value) writememh(state, (addr), (value))
#define write_dword_to_memory(addr, value) writememd(state, (addr), (value))
#define write_masked_word_to_memory(addr, value, mask) writememw_masked(state, (addr), (value), (mask))
#define write_masked_dword_to_memory(addr, value, mask) writememd_masked(state, (addr), (value), (mask))

#define get_read_address(addr) translate_read_address(state, addr)
#define get_write_address(addr) translate_write_address(state, addr)

static osal_inline unsigned bshift(uint32_t address)
{
    return ((address & 3) ^ 3) << 3;
}

static osal_inline unsigned hshift(uint32_t address)
{
    return ((address & 2) ^ 2) << 3;
}

static osal_inline uint32_t read_mem(usf_state_t* state, uint32_t addr)
{
    const uint32_t res = state->io.read[addr >> 16](state, addr);
#ifdef DEBUG_INFO
    if (state->debug_log)
    {
      fprintf(state->debug_log, "read(%08x) = %08x\n", addr, res);
    }
#endif    
    return res;
}

static osal_inline void write_mem(usf_state_t* state, uint32_t addr, uint32_t value, uint32_t mask)
{
#ifdef DEBUG_INFO
    if (state->debug_log)
    {
      fprintf(state->debug_log, "write(%08x, %08x & %08x)\n", addr, value, mask);
    }
#endif    
    state->io.write[addr >> 16](state, addr, value, mask);
}

static osal_inline uint32_t readmemw(usf_state_t* state, uint32_t addr)
{
    return read_mem(state, addr);
}

static osal_inline uint32_t readmemb(usf_state_t* state, uint32_t addr)
{
    const uint32_t value = read_mem(state, addr);
    const unsigned shift = bshift(addr);
    return (value >> shift) & 0xff;
}

static osal_inline uint32_t readmemh(usf_state_t* state, uint32_t addr)
{
    const uint32_t value = read_mem(state, addr);
    const unsigned shift = hshift(addr);
    return (value >> shift) & 0xffff;
}

static osal_inline uint64_t readmemd(usf_state_t* state, uint32_t addr)
{
    const uint32_t hi = read_mem(state, addr + 0);
    const uint32_t lo = read_mem(state, addr + 4);
    return ((uint64_t)hi << 32) | lo;
}

static osal_inline void writememw(usf_state_t* state, uint32_t addr, uint32_t value)
{
    return write_mem(state, addr, value, ~0U);
}

static osal_inline void writememw_masked(usf_state_t* state, uint32_t addr, uint32_t value, uint32_t mask)
{
   return write_mem(state, addr, value, mask);
}

static osal_inline void writememb(usf_state_t* state, uint32_t addr, uint32_t value)
{
    const unsigned shift = bshift(addr);
    const uint32_t val = (uint32_t)value << shift;
    const uint32_t mask = (uint32_t)0xff << shift;
    return write_mem(state, addr, val, mask);
}

static osal_inline void writememh(usf_state_t* state, uint32_t addr, uint32_t value)
{
    const unsigned shift = hshift(addr);
    const uint32_t val = (uint32_t)value << shift;
    const uint32_t mask = (uint32_t)0xffff << shift;
    return write_mem(state, addr, val, mask);
}

static osal_inline void writememd(usf_state_t* state, uint32_t addr, uint64_t value)
{
    write_mem(state, addr + 0, value >> 32, ~0U);
    write_mem(state, addr + 4, value >> 0 , ~0U);
}

static osal_inline void writememd_masked(usf_state_t* state, uint32_t addr, uint64_t value, uint64_t mask)
{
   write_mem(state, addr + 0, value >> 32, mask >> 32);
   write_mem(state, addr + 4, value >> 0 , mask >> 0 );
}

static osal_inline int is_mapped(uint32_t addr)
{
    //0x0000 0000..0x7fff ffff
    //0xc000 0000..0xffff ffff
    //return (addr & 0xc0000000) != 0x80000000;
    return addr < 0x80000000 || addr >= 0xc0000000;
}

static osal_inline uint32_t translate_read_address(usf_state_t* state, uint32_t addr)
{
    if (is_mapped(addr))
    {
        return virtual_to_physical_address(state, addr, 0);
    }
    else
    {
      return addr;
    }
}

static osal_inline uint32_t translate_write_address(usf_state_t* state, uint32_t addr)
{
    if (is_mapped(addr))
    {
        return virtual_to_physical_address(state, addr, 1);
    }
    else
    {
      return addr;
    }
}

#endif

