/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rsp_core.h                                              *
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

#ifndef M64P_RSP_RSP_CORE_H
#define M64P_RSP_RSP_CORE_H

#include <stdint.h>

struct r4300_core;
struct rdp_core;
struct rdram;

enum { SP_MEM_SIZE = 0x2000 };

enum sp_registers
{
    SP_MEM_ADDR_REG,
    SP_DRAM_ADDR_REG,
    SP_RD_LEN_REG,
    SP_WR_LEN_REG,
    SP_STATUS_REG,
    SP_DMA_FULL_REG,
    SP_DMA_BUSY_REG,
    SP_SEMAPHORE_REG,
    SP_REGS_COUNT
};

enum sp_registers2
{
    SP_PC_REG,
    SP_IBIST_REG,
    SP_REGS2_COUNT
};

enum
{
    SP_STATUS_INTR_BREAK = 0x040
};

#ifndef RCPREG_DEFINED
#define RCPREG_DEFINED
typedef uint32_t RCPREG;
#endif

struct rsp_core
{
    uint32_t mem[SP_MEM_SIZE/4];
    uint32_t regs[SP_REGS_COUNT];
    uint32_t regs2[SP_REGS2_COUNT];

    int32_t stage; // unused since EMULATE_STATIC_PC is defined by default in rsp/config.h
    int32_t temp_PC;
    int16_t MFC0_count[32];
    
    unsigned char * DMEM;
    unsigned char * IMEM;
    
    // RSP virtual registers, also needs alignment
    int32_t SR[32];
    RCPREG* CR[16];

    // RSP vector registers, need to be aligned to 16 bytes
    // when SSE2 or SSSE3 is enabled, or for any hope of
    // auto vectorization

    // usf_clear takes care of aligning the structure within
    // the memory block passed into it, treating the pointer
    // as usf_state_helper, and storing an offset from the
    // pointer to the actual usf_state structure. The size
    // which is indicated for allocation accounts for this
    // with two pages of padding.
    int16_t VR[32][8];
    int16_t VACC[3][8];

    // rsp/vu/cf.h, all need alignment
    int16_t ne[8]; /* $vco:  high byte "NOTEQUAL" */
    int16_t co[8]; /* $vco:  low byte "carry/borrow in/out" */
    int16_t clip[8]; /* $vcc:  high byte (clip tests:  VCL, VCH, VCR) */
    int16_t comp[8]; /* $vcc:  low byte (VEQ, VNE, VLT, VGE, VCL, VCH, VCR) */
    int16_t vce[8]; /* $vce:  vector compare extension register */

    // rsp/vu/divrom.h
    int32_t DivIn; /* buffered numerator of division read from vector file */
    int32_t DivOut; /* global division result set by VRCP/VRCPL/VRSQ/VRSQH */
#if (0)
    int32_t MovIn; /* We do not emulate this register (obsolete, for VMOV). */
#endif
    int32_t DPH;
    
    struct r4300_core* r4300;
    struct rdp_core* dp;
    struct rdram* rdram;
};

#include "osal/preproc.h"

void connect_rsp(struct rsp_core* sp,
                 struct r4300_core* r4300,
                 struct rdp_core* dp,
                 struct rdram* rdram);

void init_rsp(struct rsp_core* sp);

uint32_t read_rsp_mem(struct rsp_core* sp, uint32_t address);
void write_rsp_mem(struct rsp_core* sp, uint32_t address, uint32_t value, uint32_t mask);

uint32_t read_rsp_regs(struct rsp_core* sp, uint32_t address);
void write_rsp_regs(struct rsp_core* sp, uint32_t address, uint32_t value, uint32_t mask);

uint32_t read_rsp_regs2(struct rsp_core* sp, uint32_t address);
void write_rsp_regs2(struct rsp_core* sp, uint32_t address, uint32_t value, uint32_t mask);

void do_SP_Task(struct rsp_core* sp);

void rsp_interrupt_event(struct rsp_core* sp);

// For use by the LLE RSP
void dma_sp_write(struct rsp_core* sp);
void dma_sp_read(struct rsp_core* sp);

#endif
