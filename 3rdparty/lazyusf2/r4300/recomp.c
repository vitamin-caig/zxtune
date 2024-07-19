/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - recomp.c                                                *
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

#include <stdlib.h>
#include <string.h>

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "memory/memory_io.h"

#include "cached_interp.h"
#include "recomp.h"
#include "cp0.h"
#include "r4300.h"
#include "ops.h"
#include "tlb.h"

static void RSV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.RESERVED;
}

static void RFIN_BLOCK(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FIN_BLOCK;
}

static void RNOTCOMPILED(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NOTCOMPILED;
}

static void recompile_standard_i_type(usf_state_t * state)
{
   state->dst->f.i.rs = state->reg + ((state->src >> 21) & 0x1F);
   state->dst->f.i.rt = state->reg + ((state->src >> 16) & 0x1F);
   state->dst->f.i.immediate = state->src & 0xFFFF;
}

static void recompile_standard_j_type(usf_state_t * state)
{
   state->dst->f.j.inst_index = state->src & 0x3FFFFFF;
}

static void recompile_standard_r_type(usf_state_t * state)
{
   state->dst->f.r.rs = state->reg + ((state->src >> 21) & 0x1F);
   state->dst->f.r.rt = state->reg + ((state->src >> 16) & 0x1F);
   state->dst->f.r.rd = state->reg + ((state->src >> 11) & 0x1F);
   state->dst->f.r.sa = (state->src >>  6) & 0x1F;
}

static void recompile_standard_lf_type(usf_state_t * state)
{
   state->dst->f.lf.base = (state->src >> 21) & 0x1F;
   state->dst->f.lf.ft = (state->src >> 16) & 0x1F;
   state->dst->f.lf.offset = state->src & 0xFFFF;
}

static void recompile_standard_cf_type(usf_state_t * state)
{
   state->dst->f.cf.ft = (state->src >> 16) & 0x1F;
   state->dst->f.cf.fs = (state->src >> 11) & 0x1F;
   state->dst->f.cf.fd = (state->src >>  6) & 0x1F;
}

//-------------------------------------------------------------------------
//                                  SPECIAL                                
//-------------------------------------------------------------------------

static void RNOP(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NOP;
}

static void RSLL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLL;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRL;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRA(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRA;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSLLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLLV;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRLV;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRAV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRAV;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RJR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.JR;
   recompile_standard_i_type(state);
}

static void RJALR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.JALR;
   recompile_standard_r_type(state);
}

static void RSYSCALL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SYSCALL;
}

/* Idle loop hack from 64th Note */
static void RBREAK(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.BREAK;
}

static void RSYNC(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SYNC;
}

static void RMFHI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MFHI;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RMTHI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTHI;
   recompile_standard_r_type(state);
}

static void RMFLO(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MFLO;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RMTLO(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTLO;
   recompile_standard_r_type(state);
}

static void RDSLLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSLLV;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRLV;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRAV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRAV;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RMULT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MULT;
   recompile_standard_r_type(state);
}

static void RMULTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MULTU;
   recompile_standard_r_type(state);
}

static void RDIV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIV;
   recompile_standard_r_type(state);
}

static void RDIVU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIVU;
   recompile_standard_r_type(state);
}

static void RDMULT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMULT;
   recompile_standard_r_type(state);
}

static void RDMULTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMULTU;
   recompile_standard_r_type(state);
}

static void RDDIV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DDIV;
   recompile_standard_r_type(state);
}

static void RDDIVU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DDIVU;
   recompile_standard_r_type(state);
}

static void RADD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADD;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RADDU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADDU;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSUB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUB;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSUBU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUBU;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RAND(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.AND;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void ROR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.OR;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RXOR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.XOR;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RNOR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NOR;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSLT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLT;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSLTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLTU;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDADD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADD;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDADDU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADDU;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSUB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSUB;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSUBU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSUBU;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RTGE(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
}

static void RTGEU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
}

static void RTLT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
}

static void RTLTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
}

static void RTEQ(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TEQ;
   recompile_standard_r_type(state);
}

static void RTNE(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
}

static void RDSLL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSLL;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRL;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRA(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRA;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSLL32(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSLL32;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRL32(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRL32;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRA32(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRA32;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void (*recomp_special[64])(usf_state_t *) =
{
   RSLL , RSV   , RSRL , RSRA , RSLLV   , RSV    , RSRLV  , RSRAV  ,
   RJR  , RJALR , RSV  , RSV  , RSYSCALL, RBREAK , RSV    , RSYNC  ,
   RMFHI, RMTHI , RMFLO, RMTLO, RDSLLV  , RSV    , RDSRLV , RDSRAV ,
   RMULT, RMULTU, RDIV , RDIVU, RDMULT  , RDMULTU, RDDIV  , RDDIVU ,
   RADD , RADDU , RSUB , RSUBU, RAND    , ROR    , RXOR   , RNOR   ,
   RSV  , RSV   , RSLT , RSLTU, RDADD   , RDADDU , RDSUB  , RDSUBU ,
   RTGE , RTGEU , RTLT , RTLTU, RTEQ    , RSV    , RTNE   , RSV    ,
   RDSLL, RSV   , RDSRL, RDSRA, RDSLL32 , RSV    , RDSRL32, RDSRA32
};

//-------------------------------------------------------------------------
//                                   REGIMM                                
//-------------------------------------------------------------------------

static void RBLTZ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLTZ;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZ_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZ_OUT;
   }
}

static void RBGEZ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZ;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZ_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZ_OUT;
   }
}

static void RBLTZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLTZL;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZL_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZL_OUT;
   }
}

static void RBGEZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZL;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZL_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZL_OUT;
   }
}

static void RTGEI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
}

static void RTGEIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
}

static void RTLTI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
}

static void RTLTIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
}

static void RTEQI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
}

static void RTNEI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
}

static void RBLTZAL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLTZAL;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZAL_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZAL_OUT;
   }
}

static void RBGEZAL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZAL;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZAL_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZAL_OUT;
   }
}

static void RBLTZALL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLTZALL;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZALL_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZALL_OUT;
   }
}

static void RBGEZALL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZALL;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZALL_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZALL_OUT;
   }
}

static void (*recomp_regimm[32])(usf_state_t *) =
{
   RBLTZ  , RBGEZ  , RBLTZL  , RBGEZL  , RSV  , RSV, RSV  , RSV,
   RTGEI  , RTGEIU , RTLTI   , RTLTIU  , RTEQI, RSV, RTNEI, RSV,
   RBLTZAL, RBGEZAL, RBLTZALL, RBGEZALL, RSV  , RSV, RSV  , RSV,
   RSV    , RSV    , RSV     , RSV     , RSV  , RSV, RSV  , RSV
};

//-------------------------------------------------------------------------
//                                     TLB                                 
//-------------------------------------------------------------------------

static void RTLBR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TLBR;
}

static void RTLBWI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TLBWI;
}

static void RTLBWR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TLBWR;
}

static void RTLBP(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TLBP;
}

static void RERET(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ERET;
}

static void (*recomp_tlb[64])(usf_state_t *) =
{
   RSV  , RTLBR, RTLBWI, RSV, RSV, RSV, RTLBWR, RSV, 
   RTLBP, RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RERET, RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV
};

//-------------------------------------------------------------------------
//                                    COP0                                 
//-------------------------------------------------------------------------

static void RMFC0(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MFC0;
   recompile_standard_r_type(state);
   state->dst->f.r.rd = (int64_t*)(state->g_cp0_regs + ((state->src >> 11) & 0x1F));
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RMTC0(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTC0;
   recompile_standard_r_type(state);
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
}

static void RTLB(usf_state_t * state)
{
   recomp_tlb[(state->src & 0x3F)](state);
}

static void (*recomp_cop0[32])(usf_state_t *) =
{
   RMFC0, RSV, RSV, RSV, RMTC0, RSV, RSV, RSV,
   RSV  , RSV, RSV, RSV, RSV  , RSV, RSV, RSV,
   RTLB , RSV, RSV, RSV, RSV  , RSV, RSV, RSV,
   RSV  , RSV, RSV, RSV, RSV  , RSV, RSV, RSV
};

//-------------------------------------------------------------------------
//                                     BC                                  
//-------------------------------------------------------------------------

static void RBC1F(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BC1F;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1F_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1F_OUT;
   }
}

static void RBC1T(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BC1T;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1T_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1T_OUT;
   }
}

static void RBC1FL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BC1FL;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1FL_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1FL_OUT;
   }
}

static void RBC1TL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BC1TL;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1TL_IDLE;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1TL_OUT;
   }
}

static void (*recomp_bc[4])(usf_state_t *) =
{
   RBC1F , RBC1T ,
   RBC1FL, RBC1TL
};

//-------------------------------------------------------------------------
//                                     S                                   
//-------------------------------------------------------------------------

static void RADD_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADD_S;
   recompile_standard_cf_type(state);
}

static void RSUB_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUB_S;
   recompile_standard_cf_type(state);
}

static void RMUL_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MUL_S;
   recompile_standard_cf_type(state);
}

static void RDIV_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIV_S;
   recompile_standard_cf_type(state);
}

static void RSQRT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SQRT_S;
   recompile_standard_cf_type(state);
}

static void RABS_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ABS_S;
   recompile_standard_cf_type(state);
}

static void RMOV_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MOV_S;
   recompile_standard_cf_type(state);
}

static void RNEG_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NEG_S;
   recompile_standard_cf_type(state);
}

static void RROUND_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_L_S;
   recompile_standard_cf_type(state);
}

static void RTRUNC_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_L_S;
   recompile_standard_cf_type(state);
}

static void RCEIL_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_L_S;
   recompile_standard_cf_type(state);
}

static void RFLOOR_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_L_S;
   recompile_standard_cf_type(state);
}

static void RROUND_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_W_S;
   recompile_standard_cf_type(state);
}

static void RTRUNC_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_W_S;
   recompile_standard_cf_type(state);
}

static void RCEIL_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_W_S;
   recompile_standard_cf_type(state);
}

static void RFLOOR_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_W_S;
   recompile_standard_cf_type(state);
}

static void RCVT_D_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_D_S;
   recompile_standard_cf_type(state);
}

static void RCVT_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_W_S;
   recompile_standard_cf_type(state);
}

static void RCVT_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_L_S;
   recompile_standard_cf_type(state);
}

static void RC_F_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_F_S;
   recompile_standard_cf_type(state);
}

static void RC_UN_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UN_S;
   recompile_standard_cf_type(state);
}

static void RC_EQ_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_EQ_S;
   recompile_standard_cf_type(state);
}

static void RC_UEQ_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UEQ_S;
   recompile_standard_cf_type(state);
}

static void RC_OLT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLT_S;
   recompile_standard_cf_type(state);
}

static void RC_ULT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULT_S;
   recompile_standard_cf_type(state);
}

static void RC_OLE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLE_S;
   recompile_standard_cf_type(state);
}

static void RC_ULE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULE_S;
   recompile_standard_cf_type(state);
}

static void RC_SF_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SF_S;
   recompile_standard_cf_type(state);
}

static void RC_NGLE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGLE_S;
   recompile_standard_cf_type(state);
}

static void RC_SEQ_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SEQ_S;
   recompile_standard_cf_type(state);
}

static void RC_NGL_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGL_S;
   recompile_standard_cf_type(state);
}

static void RC_LT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LT_S;
   recompile_standard_cf_type(state);
}

static void RC_NGE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGE_S;
   recompile_standard_cf_type(state);
}

static void RC_LE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LE_S;
   recompile_standard_cf_type(state);
}

static void RC_NGT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGT_S;
   recompile_standard_cf_type(state);
}

static void (*recomp_s[64])(usf_state_t *) =
{
   RADD_S    , RSUB_S    , RMUL_S   , RDIV_S    , RSQRT_S   , RABS_S    , RMOV_S   , RNEG_S    , 
   RROUND_L_S, RTRUNC_L_S, RCEIL_L_S, RFLOOR_L_S, RROUND_W_S, RTRUNC_W_S, RCEIL_W_S, RFLOOR_W_S, 
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       , 
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       , 
   RSV       , RCVT_D_S  , RSV      , RSV       , RCVT_W_S  , RCVT_L_S  , RSV      , RSV       , 
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       , 
   RC_F_S    , RC_UN_S   , RC_EQ_S  , RC_UEQ_S  , RC_OLT_S  , RC_ULT_S  , RC_OLE_S , RC_ULE_S  , 
   RC_SF_S   , RC_NGLE_S , RC_SEQ_S , RC_NGL_S  , RC_LT_S   , RC_NGE_S  , RC_LE_S  , RC_NGT_S
};

//-------------------------------------------------------------------------
//                                     D                                   
//-------------------------------------------------------------------------

static void RADD_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADD_D;
   recompile_standard_cf_type(state);
}

static void RSUB_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUB_D;
   recompile_standard_cf_type(state);
}

static void RMUL_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MUL_D;
   recompile_standard_cf_type(state);
}

static void RDIV_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIV_D;
   recompile_standard_cf_type(state);
}

static void RSQRT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SQRT_D;
   recompile_standard_cf_type(state);
}

static void RABS_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ABS_D;
   recompile_standard_cf_type(state);
}

static void RMOV_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MOV_D;
   recompile_standard_cf_type(state);
}

static void RNEG_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NEG_D;
   recompile_standard_cf_type(state);
}

static void RROUND_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_L_D;
   recompile_standard_cf_type(state);
}

static void RTRUNC_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_L_D;
   recompile_standard_cf_type(state);
}

static void RCEIL_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_L_D;
   recompile_standard_cf_type(state);
}

static void RFLOOR_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_L_D;
   recompile_standard_cf_type(state);
}

static void RROUND_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_W_D;
   recompile_standard_cf_type(state);
}

static void RTRUNC_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_W_D;
   recompile_standard_cf_type(state);
}

static void RCEIL_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_W_D;
   recompile_standard_cf_type(state);
}

static void RFLOOR_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_W_D;
   recompile_standard_cf_type(state);
}

static void RCVT_S_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_S_D;
   recompile_standard_cf_type(state);
}

static void RCVT_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_W_D;
   recompile_standard_cf_type(state);
}

static void RCVT_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_L_D;
   recompile_standard_cf_type(state);
}

static void RC_F_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_F_D;
   recompile_standard_cf_type(state);
}

static void RC_UN_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UN_D;
   recompile_standard_cf_type(state);
}

static void RC_EQ_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_EQ_D;
   recompile_standard_cf_type(state);
}

static void RC_UEQ_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UEQ_D;
   recompile_standard_cf_type(state);
}

static void RC_OLT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLT_D;
   recompile_standard_cf_type(state);
}

static void RC_ULT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULT_D;
   recompile_standard_cf_type(state);
}

static void RC_OLE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLE_D;
   recompile_standard_cf_type(state);
}

static void RC_ULE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULE_D;
   recompile_standard_cf_type(state);
}

static void RC_SF_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SF_D;
   recompile_standard_cf_type(state);
}

static void RC_NGLE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGLE_D;
   recompile_standard_cf_type(state);
}

static void RC_SEQ_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SEQ_D;
   recompile_standard_cf_type(state);
}

static void RC_NGL_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGL_D;
   recompile_standard_cf_type(state);
}

static void RC_LT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LT_D;
   recompile_standard_cf_type(state);
}

static void RC_NGE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGE_D;
   recompile_standard_cf_type(state);
}

static void RC_LE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LE_D;

   recompile_standard_cf_type(state);
}

static void RC_NGT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGT_D;

   recompile_standard_cf_type(state);
}

static void (*recomp_d[64])(usf_state_t *) =
{
   RADD_D    , RSUB_D    , RMUL_D   , RDIV_D    , RSQRT_D   , RABS_D    , RMOV_D   , RNEG_D    ,
   RROUND_L_D, RTRUNC_L_D, RCEIL_L_D, RFLOOR_L_D, RROUND_W_D, RTRUNC_W_D, RCEIL_W_D, RFLOOR_W_D,
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       ,
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       ,
   RCVT_S_D  , RSV       , RSV      , RSV       , RCVT_W_D  , RCVT_L_D  , RSV      , RSV       ,
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       ,
   RC_F_D    , RC_UN_D   , RC_EQ_D  , RC_UEQ_D  , RC_OLT_D  , RC_ULT_D  , RC_OLE_D , RC_ULE_D  ,
   RC_SF_D   , RC_NGLE_D , RC_SEQ_D , RC_NGL_D  , RC_LT_D   , RC_NGE_D  , RC_LE_D  , RC_NGT_D
};

//-------------------------------------------------------------------------
//                                     W                                   
//-------------------------------------------------------------------------

static void RCVT_S_W(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_S_W;

   recompile_standard_cf_type(state);
}

static void RCVT_D_W(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_D_W;

   recompile_standard_cf_type(state);
}

static void (*recomp_w[64])(usf_state_t *) =
{
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RCVT_S_W, RCVT_D_W, RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV
};

//-------------------------------------------------------------------------
//                                     L                                   
//-------------------------------------------------------------------------

static void RCVT_S_L(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_S_L;

   recompile_standard_cf_type(state);
}

static void RCVT_D_L(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_D_L;

   recompile_standard_cf_type(state);
}

static void (*recomp_l[64])(usf_state_t *) =
{
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV,
   RCVT_S_L, RCVT_D_L, RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
};

//-------------------------------------------------------------------------
//                                    COP1                                 
//-------------------------------------------------------------------------

static void RMFC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MFC1;

   recompile_standard_r_type(state);
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RDMFC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMFC1;

   recompile_standard_r_type(state);
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RCFC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CFC1;

   recompile_standard_r_type(state);
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RMTC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTC1;
   recompile_standard_r_type(state);

   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
}

static void RDMTC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMTC1;
   recompile_standard_r_type(state);

   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
}

static void RCTC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CTC1;
   recompile_standard_r_type(state);

   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
}

static void RBC(usf_state_t * state)
{
   recomp_bc[((state->src >> 16) & 3)](state);
}

static void RS(usf_state_t * state)
{
   recomp_s[(state->src & 0x3F)](state);
}

static void RD(usf_state_t * state)
{
   recomp_d[(state->src & 0x3F)](state);
}

static void RW(usf_state_t * state)
{
   recomp_w[(state->src & 0x3F)](state);
}

static void RL(usf_state_t * state)
{
   recomp_l[(state->src & 0x3F)](state);
}

static void (*recomp_cop1[32])(usf_state_t *) =
{
   RMFC1, RDMFC1, RCFC1, RSV, RMTC1, RDMTC1, RCTC1, RSV,
   RBC  , RSV   , RSV  , RSV, RSV  , RSV   , RSV  , RSV,
   RS   , RD    , RSV  , RSV, RW   , RL    , RSV  , RSV,
   RSV  , RSV   , RSV  , RSV, RSV  , RSV   , RSV  , RSV
};

//-------------------------------------------------------------------------
//                                   R4300                                 
//-------------------------------------------------------------------------

static void RSPECIAL(usf_state_t * state)
{
   recomp_special[(state->src & 0x3F)](state);
}

static void RREGIMM(usf_state_t * state)
{
   recomp_regimm[((state->src >> 16) & 0x1F)](state);
}

static void RJ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.J;

   recompile_standard_j_type(state);
   target = (state->dst->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.J_IDLE;

      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.J_OUT;

   }
}

static void RJAL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.JAL;

   recompile_standard_j_type(state);
   target = (state->dst->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.JAL_IDLE;

      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.JAL_OUT;

   }
}

static void RBEQ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BEQ;

   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BEQ_IDLE;

      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BEQ_OUT;

   }
}

static void RBNE(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BNE;

   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BNE_IDLE;

      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BNE_OUT;

   }
}

static void RBLEZ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLEZ;

   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLEZ_IDLE;

      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLEZ_OUT;

   }
}

static void RBGTZ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGTZ;

   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGTZ_IDLE;

      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGTZ_OUT;

   }
}

static void RADDI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADDI;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RADDIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADDIU;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSLTI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLTI;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSLTIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLTIU;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RANDI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ANDI;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RORI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ORI;

   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RXORI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.XORI;

   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLUI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LUI;

   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RCOP0(usf_state_t * state)
{
   recomp_cop0[((state->src >> 21) & 0x1F)](state);
}

static void RCOP1(usf_state_t * state)
{
   recomp_cop1[((state->src >> 21) & 0x1F)](state);
}

static void RBEQL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BEQL;

   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BEQL_IDLE;

      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BEQL_OUT;

   }
}

static void RBNEL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BNEL;

   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BNEL_IDLE;

      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BNEL_OUT;

   }
}

static void RBLEZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLEZL;

   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLEZL_IDLE;

      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLEZL_OUT;

   }
}

static void RBGTZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGTZL;

   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGTZL_IDLE;

      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGTZL_OUT;

   }
}

static void RDADDI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADDI;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RDADDIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADDIU;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLDL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LDL;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLDR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LDR;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LB;

   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLH(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LH;

   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWL;

   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLW(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LW;

   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLBU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LBU;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLHU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LHU;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWR;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWU;

   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SB;

   recompile_standard_i_type(state);
}

static void RSH(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SH;

   recompile_standard_i_type(state);
}

static void RSWL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SWL;

   recompile_standard_i_type(state);
}

static void RSW(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SW;

   recompile_standard_i_type(state);
}

static void RSDL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SDL;

   recompile_standard_i_type(state);
}

static void RSDR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SDR;

   recompile_standard_i_type(state);
}

static void RSWR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SWR;

   recompile_standard_i_type(state);
}

static void RCACHE(usf_state_t * state)
{

   state->dst->ops = state->current_instruction_table.CACHE;
}

static void RLL(usf_state_t * state)
{

   state->dst->ops = state->current_instruction_table.LL;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWC1;

   recompile_standard_lf_type(state);
}

static void RLLD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;

   recompile_standard_i_type(state);
}

static void RLDC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LDC1;

   recompile_standard_lf_type(state);
}

static void RLD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LD;

   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSC(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SC;

   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSWC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SWC1;

   recompile_standard_lf_type(state);
}

static void RSCD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;

   recompile_standard_i_type(state);
}

static void RSDC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SDC1;

   recompile_standard_lf_type(state);
}

static void RSD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SD;

   recompile_standard_i_type(state);
}

static void (*recomp_ops[64])(usf_state_t *) =
{
   RSPECIAL, RREGIMM, RJ   , RJAL  , RBEQ , RBNE , RBLEZ , RBGTZ ,
   RADDI   , RADDIU , RSLTI, RSLTIU, RANDI, RORI , RXORI , RLUI  ,
   RCOP0   , RCOP1  , RSV  , RSV   , RBEQL, RBNEL, RBLEZL, RBGTZL,
   RDADDI  , RDADDIU, RLDL , RLDR  , RSV  , RSV  , RSV   , RSV   ,
   RLB     , RLH    , RLWL , RLW   , RLBU , RLHU , RLWR  , RLWU  ,
   RSB     , RSH    , RSWL , RSW   , RSDL , RSDR , RSWR  , RCACHE,
   RLL     , RLWC1  , RSV  , RSV   , RLLD , RLDC1, RSV   , RLD   ,
   RSC     , RSWC1  , RSV  , RSV   , RSCD , RSDC1, RSV   , RSD
};

static int get_block_length(const precomp_block *block)
{
  return (block->end-block->start)/4;
}

static size_t get_block_memsize(const precomp_block *block)
{
  int length = get_block_length(block);
  return ((length+1)+(length>>2)) * sizeof(precomp_instr);
}

/**********************************************************************
 ******************** initialize an empty block ***********************
 **********************************************************************/
void init_block(usf_state_t * state, precomp_block *block)
{
  int i, length, already_exist = 1;

  length = get_block_length(block);
   
  if (!block->block)
  {
    size_t memsize = get_block_memsize(block);
    block->block = (precomp_instr *) malloc(memsize);
    if (!block->block) {
        DebugMessage(state, M64MSG_ERROR, "Memory error: couldn't allocate memory for cached interpreter.");
        return;
    }

    memset(block->block, 0, memsize);
    already_exist = 0;
  }

  if (!already_exist)
  {
    for (i=0; i<length; i++)
    {
      state->dst = block->block + i;
      state->dst->addr = block->start + i*4;
      RNOTCOMPILED(state);
    }
  }
  else
  {
    for (i=0; i<length; i++)
    {
      state->dst = block->block + i;
      state->dst->ops = state->current_instruction_table.NOTCOMPILED;
    }
  }
   
  /* here we're marking the block as a valid code even if it's not compiled
   * yet as the game should have already set up the code correctly.
   */
  state->invalid_code[block->start>>12] = 0;
  if (block->end < 0x80000000 || block->start >= 0xc0000000)
  { 
    unsigned int paddr;
    
    paddr = virtual_to_physical_address(state, block->start, 2);
    state->invalid_code[paddr>>12] = 0;
    if (!state->blocks[paddr>>12])
    {
      state->blocks[paddr>>12] = (precomp_block *) malloc(sizeof(precomp_block));
      state->blocks[paddr>>12]->block = NULL;
      state->blocks[paddr>>12]->start = paddr & ~0xFFF;
      state->blocks[paddr>>12]->end = (paddr & ~0xFFF) + 0x1000;
    }
    init_block(state, state->blocks[paddr>>12]);
    
    paddr += block->end - block->start - 4;
    state->invalid_code[paddr>>12] = 0;
    if (!state->blocks[paddr>>12])
    {
      state->blocks[paddr>>12] = (precomp_block *) malloc(sizeof(precomp_block));
      state->blocks[paddr>>12]->block = NULL;
      state->blocks[paddr>>12]->start = paddr & ~0xFFF;
      state->blocks[paddr>>12]->end = (paddr & ~0xFFF) + 0x1000;
    }
    init_block(state, state->blocks[paddr>>12]);
  }
  else
  {
    if (block->start >= 0x80000000 && block->end < 0xa0000000 && state->invalid_code[(block->start+0x20000000)>>12])
    {
      if (!state->blocks[(block->start+0x20000000)>>12])
      {
        state->blocks[(block->start+0x20000000)>>12] = (precomp_block *) malloc(sizeof(precomp_block));
        state->blocks[(block->start+0x20000000)>>12]->block = NULL;
        state->blocks[(block->start+0x20000000)>>12]->start = (block->start+0x20000000) & ~0xFFF;
        state->blocks[(block->start+0x20000000)>>12]->end = ((block->start+0x20000000) & ~0xFFF) + 0x1000;
      }
      init_block(state, state->blocks[(block->start+0x20000000)>>12]);
    }
    if (block->start >= 0xa0000000 && block->end < 0xc0000000 && state->invalid_code[(block->start-0x20000000)>>12])
    {
      if (!state->blocks[(block->start-0x20000000)>>12])
      {
        state->blocks[(block->start-0x20000000)>>12] = (precomp_block *) malloc(sizeof(precomp_block));
        state->blocks[(block->start-0x20000000)>>12]->block = NULL;
        state->blocks[(block->start-0x20000000)>>12]->start = (block->start-0x20000000) & ~0xFFF;
        state->blocks[(block->start-0x20000000)>>12]->end = ((block->start-0x20000000) & ~0xFFF) + 0x1000;
      }
      init_block(state, state->blocks[(block->start-0x20000000)>>12]);
    }
  }
}

void free_block(usf_state_t * state, precomp_block *block)
{
    (void)state;
    if (block->block) {
        free(block->block);
        block->block = NULL;
    }
}

/**********************************************************************
 ********************* recompile a block of code **********************
 **********************************************************************/
void recompile_block(usf_state_t * state, const uint32_t* source, precomp_block *block, uint32_t pc)
{
   uint32_t i;
   int finished=0;
   const uint32_t length = (block->end - block->start) / sizeof(uint32_t);
   state->dst_block = block;
   
   block->hash = 0;
   
   for (i = (pc & TLB_OFFSET_MASK) / sizeof(uint32_t); finished != 2; i++)
   {
      if (is_mapped(block->start))
      {
          const uint32_t physical = virtual_to_physical_address(state, block->start + i * sizeof(uint32_t), 0);
          const uint32_t ph_page = physical / TLB_PAGE_SIZE;
          const uint32_t ph_offset = physical & TLB_OFFSET_MASK;
          precomp_instr* const instr = state->blocks[ph_page]->block + (ph_offset / sizeof(uint32_t));
          if (instr->ops == state->current_instruction_table.NOTCOMPILED)
          {
            instr->ops = state->current_instruction_table.NOTCOMPILED2;
          }
      }
    
    state->SRC = source + i;
    state->src = source[i];
    state->check_nop = source[i+1] == 0;
    state->dst = block->block + i;
    state->dst->addr = block->start + i*4;

    recomp_ops[((state->src >> 26) & 0x3F)](state);
    state->dst = block->block + i;

    if (i >= length-2+(length>>2)) finished = 2;
    if (i >= (length-1) && (block->start == 0xa4000000 ||
                block->start >= 0xc0000000 ||
                block->end   <  0x80000000)) finished = 2;
    if (state->dst->ops == state->current_instruction_table.ERET || finished == 1) finished = 2;
    if (/*i >= length &&*/ 
        (state->dst->ops == state->current_instruction_table.J ||
         state->dst->ops == state->current_instruction_table.J_OUT ||
         state->dst->ops == state->current_instruction_table.JR) &&
        !(i >= (length-1) && (block->start >= 0xc0000000 ||
                  block->end   <  0x80000000)))
      finished = 1;
     }

   if (i >= length)
     {
    state->dst = block->block + i;
    state->dst->addr = block->start + i*4;
    RFIN_BLOCK(state);
    i++;
    if (i < length-1+(length>>2)) // useful when last opcode is a jump
      {
         state->dst = block->block + i;
         state->dst->addr = block->start + i*4;
         RFIN_BLOCK(state);
         i++;
      }
     }

}

static int is_jump(usf_state_t * state)
{
   recomp_ops[((state->src >> 26) & 0x3F)](state);
   return
      (state->dst->ops == state->current_instruction_table.J ||
       state->dst->ops == state->current_instruction_table.J_OUT ||
       state->dst->ops == state->current_instruction_table.J_IDLE ||
       state->dst->ops == state->current_instruction_table.JAL ||
       state->dst->ops == state->current_instruction_table.JAL_OUT ||
       state->dst->ops == state->current_instruction_table.JAL_IDLE ||
       state->dst->ops == state->current_instruction_table.BEQ ||
       state->dst->ops == state->current_instruction_table.BEQ_OUT ||
       state->dst->ops == state->current_instruction_table.BEQ_IDLE ||
       state->dst->ops == state->current_instruction_table.BNE ||
       state->dst->ops == state->current_instruction_table.BNE_OUT ||
       state->dst->ops == state->current_instruction_table.BNE_IDLE ||
       state->dst->ops == state->current_instruction_table.BLEZ ||
       state->dst->ops == state->current_instruction_table.BLEZ_OUT ||
       state->dst->ops == state->current_instruction_table.BLEZ_IDLE ||
       state->dst->ops == state->current_instruction_table.BGTZ ||
       state->dst->ops == state->current_instruction_table.BGTZ_OUT ||
       state->dst->ops == state->current_instruction_table.BGTZ_IDLE ||
       state->dst->ops == state->current_instruction_table.BEQL ||
       state->dst->ops == state->current_instruction_table.BEQL_OUT ||
       state->dst->ops == state->current_instruction_table.BEQL_IDLE ||
       state->dst->ops == state->current_instruction_table.BNEL ||
       state->dst->ops == state->current_instruction_table.BNEL_OUT ||
       state->dst->ops == state->current_instruction_table.BNEL_IDLE ||
       state->dst->ops == state->current_instruction_table.BLEZL ||
       state->dst->ops == state->current_instruction_table.BLEZL_OUT ||
       state->dst->ops == state->current_instruction_table.BLEZL_IDLE ||
       state->dst->ops == state->current_instruction_table.BGTZL ||
       state->dst->ops == state->current_instruction_table.BGTZL_OUT ||
       state->dst->ops == state->current_instruction_table.BGTZL_IDLE ||
       state->dst->ops == state->current_instruction_table.JR ||
       state->dst->ops == state->current_instruction_table.JALR ||
       state->dst->ops == state->current_instruction_table.BLTZ ||
       state->dst->ops == state->current_instruction_table.BLTZ_OUT ||
       state->dst->ops == state->current_instruction_table.BLTZ_IDLE ||
       state->dst->ops == state->current_instruction_table.BGEZ ||
       state->dst->ops == state->current_instruction_table.BGEZ_OUT ||
       state->dst->ops == state->current_instruction_table.BGEZ_IDLE ||
       state->dst->ops == state->current_instruction_table.BLTZL ||
       state->dst->ops == state->current_instruction_table.BLTZL_OUT ||
       state->dst->ops == state->current_instruction_table.BLTZL_IDLE ||
       state->dst->ops == state->current_instruction_table.BGEZL ||
       state->dst->ops == state->current_instruction_table.BGEZL_OUT ||
       state->dst->ops == state->current_instruction_table.BGEZL_IDLE ||
       state->dst->ops == state->current_instruction_table.BLTZAL ||
       state->dst->ops == state->current_instruction_table.BLTZAL_OUT ||
       state->dst->ops == state->current_instruction_table.BLTZAL_IDLE ||
       state->dst->ops == state->current_instruction_table.BGEZAL ||
       state->dst->ops == state->current_instruction_table.BGEZAL_OUT ||
       state->dst->ops == state->current_instruction_table.BGEZAL_IDLE ||
       state->dst->ops == state->current_instruction_table.BLTZALL ||
       state->dst->ops == state->current_instruction_table.BLTZALL_OUT ||
       state->dst->ops == state->current_instruction_table.BLTZALL_IDLE ||
       state->dst->ops == state->current_instruction_table.BGEZALL ||
       state->dst->ops == state->current_instruction_table.BGEZALL_OUT ||
       state->dst->ops == state->current_instruction_table.BGEZALL_IDLE ||
       state->dst->ops == state->current_instruction_table.BC1F ||
       state->dst->ops == state->current_instruction_table.BC1F_OUT ||
       state->dst->ops == state->current_instruction_table.BC1F_IDLE ||
       state->dst->ops == state->current_instruction_table.BC1T ||
       state->dst->ops == state->current_instruction_table.BC1T_OUT ||
       state->dst->ops == state->current_instruction_table.BC1T_IDLE ||
       state->dst->ops == state->current_instruction_table.BC1FL ||
       state->dst->ops == state->current_instruction_table.BC1FL_OUT ||
       state->dst->ops == state->current_instruction_table.BC1FL_IDLE ||
       state->dst->ops == state->current_instruction_table.BC1TL ||
       state->dst->ops == state->current_instruction_table.BC1TL_OUT ||
       state->dst->ops == state->current_instruction_table.BC1TL_IDLE);
}

/**********************************************************************
 ************ recompile only one opcode (use for delay slot) **********
 **********************************************************************/
void recompile_opcode(usf_state_t * state)
{
   state->SRC++;
   state->src = *state->SRC;
   state->dst++;
   state->dst->addr = (state->dst-1)->addr + 4;
   if(!is_jump(state))
   {
     recomp_ops[((state->src >> 26) & 0x3F)](state);
   }
   else
   {
     RNOP(state);
   }
}

void osal_fastcall invalidate_block(usf_state_t* state, uint32_t address)
{
    const uint32_t page = address / TLB_PAGE_SIZE;
    const uint32_t offset = address & TLB_OFFSET_MASK;
    if (!state->invalid_code[page] && state->blocks[page]->block[offset / sizeof(uint32_t)].ops != state->current_instruction_table.NOTCOMPILED)
    {
        state->invalid_code[page] = 1;
    }
}
