/*  Copyright (C) 2006 yopyop
    yopyop156@ifrance.com
    yopyop156.ifrance.com

	Copyright (C) 2007 shash

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef MMU_H
#define MMU_H

#include "FIFO.h"
#include "dscard.h"

#include "ARM9.h"
#include "ARM7.h"
#include "mc.h"

#include "state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* theses macros are designed for reading/writing in memory (m is a pointer to memory, like cpu->state->MMUMMU_MEM[proc], and a is an adress, like 0x04000000 */
#define MEM_8(m, a)  (((u8*)(m[((a)>>20)&0xff]))[((a)&0xfff)])

/* theses ones for reading in rom data */
#define ROM_8(m, a)  (((u8*)(m))[(a)])
 
#define IPCFIFO  0
#define MAIN_MEMORY_DISP_FIFO 2
 
extern const u32 MMU_ARM9_WAIT16[16];
extern const u32 MMU_ARM9_WAIT32[16];
extern const u32 MMU_ARM7_WAIT16[16];
extern const u32 MMU_ARM7_WAIT32[16];

typedef struct DMA_struct {
    u32 Src;
    u32 Dst;
    u32 StartTime;
    s32 Cycle;
    u32 Crt;
    BOOL Active;
} DMA_struct;

typedef struct Timer_struct {
    u16 Counter;
    s32 Mode;
    u32 On;
    u32 Run;
    u16 Reload;
} Timer_struct;

typedef struct MMU_Core_struct {
    u8** MemMap;
    u32* MemMask;
    
    Timer_struct Timers[4];
    u32 reg_IME;
    u32 reg_IE;
    u32 reg_IF;
    
    DMA_struct DMA[4];
    nds_dscard dscard;
} MMU_Core_struct;

typedef struct MMU_struct {
        ARM9_struct* ARM9Mem;
        ARM7_struct* ARM7Mem;
        
        //Shared ram
        u8* SWIRAM;//0x8000;
        
        //Card rom & ram
        u8* CART_ROM;
        u8* CART_RAM;//0x10000;

	//Unused ram
	u8 UNUSED_RAM[4];
        
        u8 ARM9_RW_MODE;
        
        FIFO* fifos;//16

        u32 DTCMRegion;
        u32 ITCMRegion;
        
        MMU_Core_struct Cores[2];
        
        memory_chip_t fw;
        memory_chip_t bupmem;
} MMU_struct;

void MMU_Init(NDS_state *);
void MMU_DeInit(NDS_state *);

void MMU_clearMem(NDS_state *);

void MMU_setRom(NDS_state *, u8 * rom, u32 mask);
void MMU_unsetRom(NDS_state *);


/**
 * Memory reading
 */
u8 FASTCALL MMU_read8(NDS_state *, u32 proc, u32 adr);
u16 FASTCALL MMU_read16(NDS_state *, u32 proc, u32 adr);
u32 FASTCALL MMU_read32(NDS_state *, u32 proc, u32 adr);

#ifdef MMU_ENABLE_ACL
	u8 FASTCALL MMU_read8_acl(NDS_state *, u32 proc, u32 adr, u32 access);
	u16 FASTCALL MMU_read16_acl(NDS_state *, u32 proc, u32 adr, u32 access);
	u32 FASTCALL MMU_read32_acl(NDS_state *, u32 proc, u32 adr, u32 access);
#else
	#define MMU_read8_acl(state,proc,adr,access)  MMU_read8(state,proc,adr)
	#define MMU_read16_acl(state,proc,adr,access)  MMU_read16(state,proc,adr)
	#define MMU_read32_acl(state,proc,adr,access)  MMU_read32(state,proc,adr)
#endif

/**
 * Memory writing
 */
void FASTCALL MMU_write8(NDS_state *, u32 proc, u32 adr, u8 val);
void FASTCALL MMU_write16(NDS_state *, u32 proc, u32 adr, u16 val);
void FASTCALL MMU_write32(NDS_state *, u32 proc, u32 adr, u32 val);

#ifdef MMU_ENABLE_ACL
	void FASTCALL MMU_write8_acl(NDS_state *, u32 proc, u32 adr, u8 val);
	void FASTCALL MMU_write16_acl(NDS_state *, u32 proc, u32 adr, u16 val);
	void FASTCALL MMU_write32_acl(NDS_state *, u32 proc, u32 adr, u32 val);
#else
	#define MMU_write8_acl MMU_write8
	#define MMU_write16_acl MMU_write16
	#define MMU_write32_acl MMU_write32
#endif
 
void FASTCALL MMU_doDMA(NDS_state *, u32 proc, u32 num);


#ifdef __cplusplus
}
#endif

#endif
