/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - tlb.h                                                   *
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

#ifndef M64P_R4300_TLB_H
#define M64P_R4300_TLB_H

#include "usf/usf.h"

#ifndef TLB_STRUCTS
#define TLB_STRUCTS
typedef struct _tlb
{
   short mask;
   uint32_t vpn2;
   char g;
   unsigned char asid;
   uint32_t pfn_even;
   char c_even;
   char d_even;
   char v_even;
   uint32_t pfn_odd;
   char c_odd;
   char d_odd;
   char v_odd;
   char r;
   //int check_parity_mask;
   
   uint32_t start_even;
   uint32_t end_even;
   uint32_t phys_even;
   uint32_t start_odd;
   uint32_t end_odd;
   uint32_t phys_odd;
} tlb;
#endif

void tlb_unmap(usf_state_t *, tlb *entry);
void tlb_map(usf_state_t *, tlb *entry);

uint32_t virtual_to_physical_address(usf_state_t *, uint32_t addresse, int w);

#endif /* M64P_R4300_TLB_H */
