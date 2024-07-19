/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - recomp.h                                                *
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

#ifndef M64P_R4300_RECOMP_H
#define M64P_R4300_RECOMP_H

#include <stddef.h>

#include "osal/preproc.h"

#ifndef PRECOMP_STRUCTS
#define PRECOMP_STRUCTS

typedef struct _precomp_instr
{
   void (osal_fastcall *ops)(usf_state_t * state);
   union
     {
    struct
      {
         int64_t *rs;
         int64_t *rt;
         short immediate;
      } i;
    struct
      {
         unsigned int inst_index;
      } j;
    struct
      {
         int64_t *rs;
         int64_t *rt;
         int64_t *rd;
         unsigned char sa;
         unsigned char nrd;
      } r;
    struct
      {
         unsigned char base;
         unsigned char ft;
         short offset;
      } lf;
    struct
      {
         unsigned char ft;
         unsigned char fs;
         unsigned char fd;
      } cf;
     } f;
   unsigned int addr; /* word-aligned instruction address in r4300 address space */
} precomp_instr;

typedef struct _precomp_block
{
   precomp_instr *block;
   unsigned int start;
   unsigned int end;
   unsigned int hash;
} precomp_block;
#endif

void recompile_block(usf_state_t *, const uint32_t* source, precomp_block *block, uint32_t pc);
void init_block(usf_state_t *, precomp_block *block);
void free_block(usf_state_t *, precomp_block *block);
void recompile_opcode(usf_state_t *);

void osal_fastcall invalidate_block(usf_state_t* state, uint32_t address);

#endif /* M64P_R4300_RECOMP_H */

