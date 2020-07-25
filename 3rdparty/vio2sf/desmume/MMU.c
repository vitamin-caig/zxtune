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

//#define RENDER3D

#include <stdlib.h>
#include <math.h>
#include <string.h>

//#include "gl_vertex.h"
#include "spu_exports.h"

#include "state.h"

#include "MMU.h"

#include "debug.h"
#include "NDSSystem.h"
//#include "cflash.h"
#define cflash_read(s,a) 0
#define cflash_write(s,a,d)
#include "cp15.h"
#include "registers.h"
#include "isqrt.h"

#define ROM_MASK 3

/*
 *
 */
#define EARLY_MEMORY_ACCESS 1

#define INTERNAL_DTCM_READ 1
#define INTERNAL_DTCM_WRITE 1

#define DUP2(x)  x, x
#define DUP4(x)  x, x, x, x
#define DUP8(x)  x, x, x, x,  x, x, x, x
#define DUP16(x) x, x, x, x,  x, x, x, x,  x, x, x, x,  x, x, x, x

const u32 MMU_ARM9_WAIT16[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 1, 1, 1, 1, 1,
};

const u32 MMU_ARM9_WAIT32[16]={
	1, 1, 1, 1, 1, 2, 2, 1, 8, 8, 5, 1, 1, 1, 1, 1,
};

const u32 MMU_ARM7_WAIT16[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 1, 1, 1, 1, 1,
};

const u32 MMU_ARM7_WAIT32[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 8, 8, 5, 1, 1, 1, 1, 1,
};

/* http://problemkaputt.de/gbatek.htm#dsmemorymaps
NDS9 Memory Map
  00000000h  Instruction TCM (32KB) (not moveable) (mirror-able to 1000000h)
  0xxxx000h  Data TCM        (16KB) (moveable)
  02000000h  Main Memory     (4MB)
  03000000h  Shared WRAM     (0KB, 16KB, or 32KB can be allocated to ARM9)
  04000000h  ARM9-I/O Ports
  05000000h  Standard Palettes (2KB) (Engine A BG/OBJ, Engine B BG/OBJ)
  06000000h  VRAM - Engine A, BG VRAM  (max 512KB)
  06200000h  VRAM - Engine B, BG VRAM  (max 128KB)
  06400000h  VRAM - Engine A, OBJ VRAM (max 256KB)
  06600000h  VRAM - Engine B, OBJ VRAM (max 128KB)
  06800000h  VRAM - "LCDC"-allocated (max 656KB)
  07000000h  OAM (2KB) (Engine A, Engine B)
  08000000h  GBA Slot ROM (max 32MB)
  0A000000h  GBA Slot RAM (max 64KB)
  FFFF0000h  ARM9-BIOS (32KB) (only 3K used)
The ARM9 Exception Vectors are located at FFFF0000h. The IRQ handler redirects to [DTCM+3FFCh].
*/

static void MMU_Init_Arm9(MMU_struct *mmu) {
    int i;
    MMU_Core_struct* const core = mmu->Cores + ARMCPU_ARM9;
    
    for (i = 0; i < 0x10; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_ITCM;
        core->MemMask[i] = 0x00007FFF;
    }
    
    for (; i < 0x20; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_WRAM;
        core->MemMask[i] = 0x00FFFFFF;
    }
    
    for (; i < 0x30; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->MAIN_MEM;
        core->MemMask[i] = 0x003FFFFF;
    }

    for (; i < 0x40; ++i)
    {
        core->MemMap[i] = mmu->SWIRAM;
        core->MemMask[i] = 0x00007FFF;
    }
    
    for (; i < 0x50; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_REG;
        core->MemMask[i] = 0x00FFFFFF;
    }

    for (; i < 0x60; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_VMEM;
        core->MemMask[i] = 0x000007FF;
    }
    
    for (; i < 0x62; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_ABG;
        core->MemMask[i] = 0x0007FFFF;
    }
    
    for (; i < 0x64; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_BBG;
        core->MemMask[i] = 0x0001FFFF;
    }
    
    for (; i < 0x66; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_AOBJ;
        core->MemMask[i] = 0x0003FFFF;
    }
    
    for (; i < 0x68; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_BOBJ;
        core->MemMask[i] = 0x0001FFFF;
    }
    
    for (; i < 0x70; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_LCD;
        core->MemMask[i] = 0x000FFFFF;
    }
    
    for (; i < 0x80; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_OAM;
        core->MemMask[i] = 0x000007FF;
    }
    
    for (; i < 0xA0; ++i)
    {
        core->MemMap[i] = NULL;
        core->MemMask[i] = ROM_MASK;
    }

    for (; i < 0xB0; ++i)
    {
        core->MemMap[i] = mmu->CART_RAM;
        core->MemMask[i] = 0x0000FFFF;
    }
    
    for (; i < 0xF0; ++i)
    {
        core->MemMap[i] = mmu->UNUSED_RAM;
        core->MemMask[i] = 0x00000003;
    }
    
    for (; i < 0x100; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_BIOS;
        core->MemMask[i] = 0x00007FFF;
    }
}

/*
NDS7 Memory Map
  00000000h  ARM7-BIOS (16KB)
  02000000h  Main Memory (4MB)
  03000000h  Shared WRAM (0KB, 16KB, or 32KB can be allocated to ARM7)
  03800000h  ARM7-WRAM (64KB)
  04000000h  ARM7-I/O Ports
  04800000h  Wireless Communications Wait State 0 (8KB RAM at 4804000h)
  04808000h  Wireless Communications Wait State 1 (I/O Ports at 4808000h)
  06000000h  VRAM allocated as Work RAM to ARM7 (max 256K)
  08000000h  GBA Slot ROM (max 32MB)
  0A000000h  GBA Slot RAM (max 64KB)
*/

static void MMU_Init_Arm7(MMU_struct *mmu) {
    int i;
    MMU_Core_struct* const core = mmu->Cores + ARMCPU_ARM7;

    for (i = 0; i < 0x10; ++i)
    {
        core->MemMap[i] = mmu->ARM7Mem->ARM7_BIOS;
        core->MemMask[i] = 0x00003FFF;
    }
    
    for (; i < 0x20; ++i)
    {
        core->MemMap[i] = mmu->UNUSED_RAM;
        core->MemMask[i] = 0x00000003;
    }
    
    for (; i < 0x30; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->MAIN_MEM;
        core->MemMask[i] = 0x003FFFFF;
    }

    for (; i < 0x38; ++i)
    {
        core->MemMap[i] = mmu->SWIRAM;
        core->MemMask[i] = 0x00007FFF;
    }
    
    for (; i < 0x40; ++i)
    {
        core->MemMap[i] = mmu->ARM7Mem->ARM7_ERAM;
        core->MemMask[i] = 0x0000FFFF;
    }

    for (; i < 0x48; ++i)
    {
        core->MemMap[i] = mmu->ARM7Mem->ARM7_REG;
        core->MemMask[i] = 0x00FFFFFF;
    }
    
    for (; i < 0x50; ++i)
    {
        core->MemMap[i] = mmu->ARM7Mem->ARM7_WIRAM;
        core->MemMask[i] = 0x0000FFFF;
    }
    
    for (; i < 0x60; ++i)
    {
        core->MemMap[i] = mmu->UNUSED_RAM;
        core->MemMask[i] = 0x00000003;
    }
    
    for (; i < 0x70; ++i)
    {
        core->MemMap[i] = mmu->ARM9Mem->ARM9_ABG;
        core->MemMask[i] = 0x0003FFFF;
    }

    for (; i < 0x80; ++i)
    {
        core->MemMap[i] = mmu->UNUSED_RAM;
        core->MemMask[i] = 0x00000003;
    }
    
    for (; i < 0xA0; ++i)
    {
        core->MemMap[i] = NULL;
        core->MemMask[i] = ROM_MASK;
    }
    
    for (; i < 0xB0; ++i)
    {
        core->MemMap[i] = mmu->CART_RAM;
        core->MemMask[i] = 0x0000FFFF;
    }
    
    for (; i < 0x100; ++i)
    {
        core->MemMap[i] = mmu->UNUSED_RAM;
        core->MemMask[i] = 0x00000003;
    }
}

void MMU_Init(NDS_state *state) {
	int i;

	LOG("MMU init\n");

	memset(state->MMU, 0, sizeof(MMU_struct));
  
  state->MMU->ARM9Mem = calloc(sizeof(ARM9_struct), 1);
  state->MMU->ARM7Mem = calloc(sizeof(ARM7_struct), 1);
  
  state->MMU->SWIRAM = calloc(0x8000, 1);

	state->MMU->CART_ROM = state->MMU->UNUSED_RAM;
  state->MMU->CART_RAM = calloc(0x10000, 1);

	state->MMU->ITCMRegion = 0x00800000;

  state->MMU->fifos = calloc(16, sizeof(FIFO));
	for(i = 0;i < 16;i++)
		FIFOInit(state->MMU->fifos + i);
	
  mc_init(&state->MMU->fw, MC_TYPE_FLASH);  /* init fw device */
  mc_alloc(&state->MMU->fw, NDS_FW_SIZE_V1);

  // Init Backup Memory device, this should really be done when the rom is loaded
  mc_init(&state->MMU->bupmem, MC_TYPE_AUTODETECT);
  mc_alloc(&state->MMU->bupmem, 1);
  
  MMU_Init_Arm9(state->MMU);
  MMU_Init_Arm7(state->MMU);
}

void MMU_DeInit(NDS_state *state) {
	LOG("MMU deinit\n");
    mc_free(&state->MMU->fw);
    mc_free(&state->MMU->bupmem);
    free(state->MMU->fifos);
    state->MMU->fifos = 0;
    free(state->MMU->CART_RAM);
    state->MMU->CART_RAM = 0;
    free(state->MMU->SWIRAM);
    state->MMU->SWIRAM = 0;
    free(state->MMU->ARM7Mem);
    state->MMU->ARM7Mem = 0;
    free(state->MMU->ARM9Mem);
    state->MMU->ARM9Mem = 0;
}

static void MMU_clearCoreMem(MMU_Core_struct* mmu)
{
  memset(&mmu->Timers, 0, sizeof(mmu->Timers));
	
	memset(&mmu->reg_IME, 0, sizeof(mmu->reg_IME));
	memset(&mmu->reg_IE, 0, sizeof(mmu->reg_IE));
	memset(&mmu->reg_IF, 0, sizeof(mmu->reg_IF));
	
	memset(&mmu->DMA, 0, sizeof(mmu->DMA));
	
	memset(&mmu->dscard, 0, sizeof(mmu->dscard));
}

void MMU_clearMem(NDS_state *state)
{
	int i;

	memset(state->MMU->ARM9Mem->ARM9_ABG,  0, 0x080000);
	memset(state->MMU->ARM9Mem->ARM9_AOBJ, 0, 0x040000);
	memset(state->MMU->ARM9Mem->ARM9_BBG,  0, 0x020000);
	memset(state->MMU->ARM9Mem->ARM9_BOBJ, 0, 0x020000);
	memset(state->MMU->ARM9Mem->ARM9_DTCM, 0, 0x4000);
	memset(state->MMU->ARM9Mem->ARM9_ITCM, 0, 0x8000);
	memset(state->MMU->ARM9Mem->ARM9_LCD,  0, 0x0A4000);
	memset(state->MMU->ARM9Mem->ARM9_OAM,  0, 0x0800);
	memset(state->MMU->ARM9Mem->ARM9_REG,  0, 0x01000000);
	memset(state->MMU->ARM9Mem->ARM9_VMEM, 0, 0x0800);
	memset(state->MMU->ARM9Mem->ARM9_WRAM, 0, 0x01000000);
	memset(state->MMU->ARM9Mem->MAIN_MEM,  0, 0x400000);

	memset(state->MMU->ARM7Mem->ARM7_ERAM,     0, 0x010000);
	memset(state->MMU->ARM7Mem->ARM7_REG,      0, 0x010000);
	
	for(i = 0;i < 16;i++)
	FIFOInit(state->MMU->fifos + i);
	
	state->MMU->DTCMRegion = 0;
	state->MMU->ITCMRegion = 0x00800000;
	
  MMU_clearCoreMem(state->MMU->Cores + 0);
  MMU_clearCoreMem(state->MMU->Cores + 1);
}

void MMU_setRom(NDS_state *state, u8 * rom, u32 mask)
{
	state->MMU->CART_ROM = rom;
	
  for (int proc = 0; proc < 2; ++proc)
  {
    MMU_Core_struct* const core = state->MMU->Cores + proc;
    for(int i = 0x80; i<0xA0; ++i)
	  {
      core->MemMap[i] = rom;
      core->MemMask[i] = mask;
    }
	}
}

void MMU_unsetRom(NDS_state *state)
{
  MMU_setRom(state, state->MMU->UNUSED_RAM, ROM_MASK);
}

u8 FASTCALL MMU_read8(NDS_state *state, u32 proc, u32 adr)
{
#ifdef INTERNAL_DTCM_READ
	if((proc==ARMCPU_ARM9)&((adr&(~0x3FFF))==state->MMU->DTCMRegion))
	{
		return state->MMU->ARM9Mem->ARM9_DTCM[adr&0x3FFF];
	}
#endif

	// CFlash reading, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000))
  {
		return (unsigned char)cflash_read(state, adr);
  }

  MMU_Core_struct* const core = state->MMU->Cores + proc;
  return core->MemMap[(adr>>20)&0xFF][adr&core->MemMask[(adr>>20)&0xFF]];
}


u16 FASTCALL MMU_read16(NDS_state *state, u32 proc, u32 adr)
{    
#ifdef INTERNAL_DTCM_READ
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Returns data from DTCM (ARM9 only) */
		return T1ReadWord(state->MMU->ARM9Mem->ARM9_DTCM, adr & 0x3FFF);
	}
#endif
	
	// CFlash reading, Mic
	if ((adr>=0x08800000)&&(adr<0x09900000))
  {
	   return (unsigned short)cflash_read(state, adr);
  }

	adr &= 0x0FFFFFFF;

  MMU_Core_struct* const core = state->MMU->Cores + proc;
  
	if(adr&0x04000000)
	{
		/* Adress is an IO register */
		switch(adr)
		{
			case REG_IPCFIFORECV :               /* TODO (clear): ??? */
				state->execute = FALSE;
				return 1;
				
			case REG_IME :
				return (u16)core->reg_IME;
				
			case REG_IE :
				return (u16)core->reg_IE;
			case REG_IE + 2 :
				return (u16)(core->reg_IE>>16);
				
			case REG_IF :
				return (u16)core->reg_IF;
			case REG_IF + 2 :
				return (u16)(core->reg_IF>>16);
				
			case REG_TM0CNTL :
			case REG_TM1CNTL :
			case REG_TM2CNTL :
			case REG_TM3CNTL :
				return core->Timers[(adr&0xF)>>2].Counter;
			
			case 0x04000630 :
				LOG("vect res\r\n");	/* TODO (clear): ??? */
				//execute = FALSE;
				return 0;
                        case REG_POSTFLG :
				return 1;
			default :
				break;
		}
	}
	
    /* Returns data from memory */
	return T1ReadWord(core->MemMap[(adr >> 20) & 0xFF], adr & core->MemMask[(adr >> 20) & 0xFF]);
}

static u32 FASTCALL MMU_read32_io(NDS_state *state, u32 proc, u32 adr)
{
  MMU_Core_struct* const core = state->MMU->Cores + proc;
	switch(adr & 0xffffff)
	{
		// This is hacked due to the only current 3D core
		case 0x04000600 & 0xffffff:
          {
              u32 fifonum = IPCFIFO+proc;

			u32 gxstat =	(state->MMU->fifos[fifonum].empty<<26) |
							(1<<25) | 
							(state->MMU->fifos[fifonum].full<<24) |
							/*((NDS_nbpush[0]&1)<<13) | ((NDS_nbpush[2]&0x1F)<<8) |*/ 
							2;

			LOG ("GXSTAT: 0x%X", gxstat);

			return	gxstat;
          }

		case 0x04000640 & 0xffffff:
		case 0x04000644 & 0xffffff:
		case 0x04000648 & 0xffffff:
		case 0x0400064C & 0xffffff:
		case 0x04000650 & 0xffffff:
		case 0x04000654 & 0xffffff:
		case 0x04000658 & 0xffffff:
		case 0x0400065C & 0xffffff:
		case 0x04000660 & 0xffffff:
		case 0x04000664 & 0xffffff:
		case 0x04000668 & 0xffffff:
		case 0x0400066C & 0xffffff:
		case 0x04000670 & 0xffffff:
		case 0x04000674 & 0xffffff:
		case 0x04000678 & 0xffffff:
		case 0x0400067C & 0xffffff:
		case 0x04000680 & 0xffffff:
		case 0x04000684 & 0xffffff:
		case 0x04000688 & 0xffffff:
		case 0x0400068C & 0xffffff:
		case 0x04000690 & 0xffffff:
		case 0x04000694 & 0xffffff:
		case 0x04000698 & 0xffffff:
		case 0x0400069C & 0xffffff:
		case 0x040006A0 & 0xffffff:
		case 0x04000604 & 0xffffff:
		{
			return 0;
		}
		
		case REG_IME & 0xffffff :
			return core->reg_IME;
		case REG_IE & 0xffffff :
			return core->reg_IE;
		case REG_IF & 0xffffff :
			return core->reg_IF;
		case REG_IPCFIFORECV & 0xffffff :
		{
			u16 IPCFIFO_CNT = T1ReadWord(core->MemMap[0x40], 0x184);
			if(IPCFIFO_CNT&0x8000)
			{
			//execute = FALSE;
			u32 fifonum = IPCFIFO+proc;
			u32 val = FIFOValue(state->MMU->fifos + fifonum);
			u32 remote = (proc+1) & 1;
      MMU_Core_struct* const remote_core = state->MMU->Cores + remote;
			u16 IPCFIFO_CNT_remote = T1ReadWord(remote_core->MemMap[0x40], 0x184);
			IPCFIFO_CNT |= (state->MMU->fifos[fifonum].empty<<8) | (state->MMU->fifos[fifonum].full<<9) | (state->MMU->fifos[fifonum].error<<14);
			IPCFIFO_CNT_remote |= (state->MMU->fifos[fifonum].empty) | (state->MMU->fifos[fifonum].full<<1);
			T1WriteWord(core->MemMap[0x40], 0x184, IPCFIFO_CNT);
			T1WriteWord(remote_core->MemMap[0x40], 0x184, IPCFIFO_CNT_remote);
			if ((state->MMU->fifos[fifonum].empty) && (IPCFIFO_CNT & BIT(2)))
				NDS_makeInt(state, remote,17) ; /* remote: SEND FIFO EMPTY */
			return val;
			}
		}
		return 0;
                      case REG_TM0CNTL & 0xffffff :
                      case REG_TM1CNTL & 0xffffff :
                      case REG_TM2CNTL & 0xffffff :
                      case REG_TM3CNTL & 0xffffff :
		{
			u32 val = T1ReadWord(core->MemMap[0x40], (adr + 2) & 0xFFF);
			return core->Timers[(adr&0xF)>>2].Counter | (val<<16);
		}	
                      case REG_GCDATAIN & 0xffffff:
		{
                              u32 val;

                              if(!core->dscard.adress) return 0;

                              val = T1ReadLong(state->MMU->CART_ROM, core->dscard.adress);

			core->dscard.adress += 4;	/* increment adress */
			
			core->dscard.transfer_count--;	/* update transfer counter */
			if(core->dscard.transfer_count) /* if transfer is not ended */
			{
				return val;	/* return data */
			}
			else	/* transfer is done */
                              {                                                       
                                      T1WriteLong(core->MemMap[(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, T1ReadLong(core->MemMap[(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff) & ~(0x00800000 | 0x80000000));
				/* = 0x7f7fffff */
				
				/* if needed, throw irq for the end of transfer */
                                      if(T1ReadWord(core->MemMap[(REG_AUXSPICNT >> 20) & 0xff], REG_AUXSPICNT & 0xfff) & 0x4000)
				{
                                              NDS_makeInt(state, proc, 19);
				}
				
				return val;
			}
		}

		default :
			break;
	}
	
	/* Returns data from memory */
	return T1ReadLong(core->MemMap[(adr >> 20) & 0xFF], adr & core->MemMask[(adr >> 20) & 0xFF]);
}
	 
u32 FASTCALL MMU_read32(NDS_state *state, u32 proc, u32 adr)
{
#ifdef INTERNAL_DTCM_READ
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Returns data from DTCM (ARM9 only) */
		return T1ReadLong(state->MMU->ARM9Mem->ARM9_DTCM, adr & 0x3FFF);
	}
#endif
	
	// CFlash reading, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000))
  {
	   return (unsigned long)cflash_read(state, adr);
  }
		
	adr &= 0x0FFFFFFF;

	if((adr >> 24) == 4)
	{
		/* Adress is an IO register */
    return MMU_read32_io(state, proc, adr);
  }
  else
  {
    MMU_Core_struct* const core = state->MMU->Cores + proc;
    return T1ReadLong(core->MemMap[(adr >> 20) & 0xFF], adr & core->MemMask[(adr >> 20) & 0xFF]);
  }
}
	
void FASTCALL MMU_write8(NDS_state *state, u32 proc, u32 adr, u8 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Writes data in DTCM (ARM9 only) */
		state->MMU->ARM9Mem->ARM9_DTCM[adr&0x3FFF] = val;
		return ;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000)) {
		cflash_write(state,adr,val);
		return;
	}

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteByte(state, adr, val);
              return;
           }
        }

	if ((adr & 0xFF800000) == 0x04800000)
	{
		/* is wifi hardware, dont intermix with regular hardware registers */
		/* FIXME handle 8 bit writes */
		return ;
	}
  MMU_Core_struct* const core = state->MMU->Cores + proc;
	core->MemMap[(adr>>20)&0xFF][adr & core->MemMask[(adr>>20)&0xFF]]=val;
}

void FASTCALL MMU_write16(NDS_state *state, u32 proc, u32 adr, u16 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Writes in DTCM (ARM9 only) */
		T1WriteWord(state->MMU->ARM9Mem->ARM9_DTCM, adr & 0x3FFF, val);
		return;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x08800000)&&(adr<0x09900000))
	{
		cflash_write(state,adr,val);
		return;
	}

	if ((proc==ARMCPU_ARM7) && (adr>=0x04800000)&&(adr<0x05000000))
		return ;

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteWord(state, adr, val);
              return;
           }
        }

  MMU_Core_struct* const core = state->MMU->Cores + proc;
	if((adr >> 24) == 4)
	{
		/* Adress is an IO register */
		switch(adr)
		{
      case REG_POWCNT1 :
				T1WriteWord(core->MemMap[0x40], 0x304, val);
				return;

                        case REG_AUXSPICNT:
                                T1WriteWord(core->MemMap[(REG_AUXSPICNT >> 20) & 0xff], REG_AUXSPICNT & 0xfff, val);
                                state->AUX_SPI_CNT = val;

                                if (val == 0)
                                   mc_reset_com(&state->MMU->bupmem);     /* reset backup memory device communication */
				return;
				
                        case REG_AUXSPIDATA:
                                if(val!=0)
                                {
                                   state->AUX_SPI_CMD = val & 0xFF;
                                }

                                T1WriteWord(core->MemMap[(REG_AUXSPIDATA >> 20) & 0xff], REG_AUXSPIDATA & 0xfff, bm_transfer(&state->MMU->bupmem, (u8)val));
				return;

			case REG_SPICNT :
				if(proc == ARMCPU_ARM7)
				{
                                  int reset_firmware = 1;

                                  if ( ((state->SPI_CNT >> 8) & 0x3) == 1) {
                                    if ( ((val >> 8) & 0x3) == 1) {
                                      if ( BIT11(state->SPI_CNT)) {
                                        /* select held */
                                        reset_firmware = 0;
                                      }
                                    }
                                  }
					
                                        //MMU->fw.com == 0; /* reset fw device communication */
                                    if ( reset_firmware) {
                                      /* reset fw device communication */
                                      mc_reset_com(&state->MMU->fw);
                                    }
                                    state->SPI_CNT = val;
                                }
				
				T1WriteWord(core->MemMap[(REG_SPICNT >> 20) & 0xff], REG_SPICNT & 0xfff, val);
				return;
				
			case REG_SPIDATA :
				if(proc==ARMCPU_ARM7)
				{
                                        u16 spicnt;

					if(val!=0)
					{
						state->SPI_CMD = val;
					}
			
                                        spicnt = T1ReadWord(core->MemMap[(REG_SPICNT >> 20) & 0xff], REG_SPICNT & 0xfff);
					
                                        switch((spicnt >> 8) & 0x3)
					{
                                                case 0 :
							break;
							
                                                case 1 : /* firmware memory device */
                                                        if((spicnt & 0x3) != 0)      /* check SPI baudrate (must be 4mhz) */
							{
								T1WriteWord(core->MemMap[(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, 0);
								break;
							}
							T1WriteWord(core->MemMap[(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, fw_transfer(&state->MMU->fw, (u8)val));

							return;
							
                                                case 2 :
							switch(state->SPI_CMD & 0x70)
							{
								case 0x00 :
									val = 0;
									break;
								case 0x10 :
									//execute = FALSE;
									if(state->SPI_CNT&(1<<11))
									{
										if(state->partie)
										{
											val = ((state->nds->touchY<<3)&0x7FF);
											state->partie = 0;
											//execute = FALSE;
											break;
										}
										val = (state->nds->touchY>>5);
                                                                                state->partie = 1;
										break;
									}
									val = ((state->nds->touchY<<3)&0x7FF);
									state->partie = 1;
									break;
								case 0x20 :
									val = 0;
									break;
								case 0x30 :
									val = 0;
									break;
								case 0x40 :
									val = 0;
									break;
								case 0x50 :
                                                                        if(spicnt & 0x800)
									{
										if(state->partie)
										{
											val = ((state->nds->touchX<<3)&0x7FF);
											state->partie = 0;
											break;
										}
										val = (state->nds->touchX>>5);
										state->partie = 1;
										break;
									}
									val = ((state->nds->touchX<<3)&0x7FF);
									state->partie = 1;
									break;
								case 0x60 :
									val = 0;
									break;
								case 0x70 :
									val = 0;
									break;
							}
							break;
							
                                                case 3 :
							/* NOTICE: Device 3 of SPI is reserved (unused and unusable) */
							break;
					}
				}
				
				T1WriteWord(core->MemMap[(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, val);
				return;
				
				/* NOTICE: Perhaps we have to use gbatek-like reg names instead of libnds-like ones ...*/
				
                        case REG_DISPA_BG0CNT :
				T1WriteWord(core->MemMap[0x40], 0x8, val);
				return;
                        case REG_DISPA_BG1CNT :
				T1WriteWord(core->MemMap[0x40], 0xA, val);
				return;
                        case REG_DISPA_BG2CNT :
				T1WriteWord(core->MemMap[0x40], 0xC, val);
				return;
                        case REG_DISPA_BG3CNT :
				T1WriteWord(core->MemMap[0x40], 0xE, val);
				return;
                        case REG_DISPB_BG0CNT :
				T1WriteWord(core->MemMap[0x40], 0x1008, val);
				return;
                        case REG_DISPB_BG1CNT :
				T1WriteWord(core->MemMap[0x40], 0x100A, val);
				return;
                        case REG_DISPB_BG2CNT :
				T1WriteWord(core->MemMap[0x40], 0x100C, val);
				return;
                        case REG_DISPB_BG3CNT :
				T1WriteWord(core->MemMap[0x40], 0x100E, val);
				return;
                        case REG_IME : {
			        u32 old_val = core->reg_IME;
				u32 new_val = val & 1;
				core->reg_IME = new_val;
				T1WriteLong(core->MemMap[0x40], 0x208, val);
				if ( new_val && old_val != new_val) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( core->reg_IE & core->reg_IF) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			}
			case REG_VRAMCNTA:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTC:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTE:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTG:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTI:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				return ;

			case REG_IE :
				core->reg_IE = (core->reg_IE&0xFFFF0000) | val;
				if ( core->reg_IME) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( core->reg_IE & core->reg_IF) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			case REG_IE + 2 :
				state->execute = FALSE;
				core->reg_IE = (core->reg_IE&0xFFFF) | (((u32)val)<<16);
				return;
				
			case REG_IF :
				state->execute = FALSE;
				core->reg_IF &= (~((u32)val));
				return;
			case REG_IF + 2 :
				state->execute = FALSE;
				core->reg_IF &= (~(((u32)val)<<16));
				return;
				
                        case REG_IPCSYNC :
				{
				u32 remote = (proc+1)&1;
        MMU_Core_struct* const remote_core = state->MMU->Cores + remote;
				u16 IPCSYNC_remote = T1ReadWord(remote_core->MemMap[0x40], 0x180);
				T1WriteWord(core->MemMap[0x40], 0x180, (val&0xFFF0)|((IPCSYNC_remote>>8)&0xF));
				T1WriteWord(remote_core->MemMap[0x40], 0x180, (IPCSYNC_remote&0xFFF0)|((val>>8)&0xF));
				remote_core->reg_IF |= ((IPCSYNC_remote & (1<<14))<<2) & ((val & (1<<13))<<3);// & (MMU->reg_IME[remote] << 16);// & (MMU->reg_IE[remote] & (1<<16));//
				//execute = FALSE;
				}
				return;
                        case REG_IPCFIFOCNT :
				{
    			u32 remote = (proc+1)&1;
          MMU_Core_struct* const remote_core = state->MMU->Cores + remote;
					u32 cnt_l = T1ReadWord(core->MemMap[0x40], 0x184) ;
					u32 cnt_r = T1ReadWord(remote_core->MemMap[0x40], 0x184) ;
					if ((val & 0x8000) && !(cnt_l & 0x8000))
					{
						/* this is the first init, the other side didnt init yet */
						/* so do a complete init */
						FIFOInit(state->MMU->fifos + (IPCFIFO+proc));
						T1WriteWord(core->MemMap[0x40], 0x184,0x8101) ;
						/* and then handle it as usual */
					}

				if(val & 0x4008)
				{
					FIFOInit(state->MMU->fifos + (IPCFIFO+((proc+1)&1)));
					T1WriteWord(core->MemMap[0x40], 0x184, (cnt_l & 0x0301) | (val & 0x8404) | 1);
					T1WriteWord(remote_core->MemMap[0x40], 0x184, (cnt_r & 0xC507) | 0x100);
					core->reg_IF |= ((val & 4)<<15);// & (MMU->reg_IME[proc]<<17);// & (MMU->reg_IE[proc]&0x20000);//
					return;
				}
				T1WriteWord(core->MemMap[0x40], 0x184, T1ReadWord(core->MemMap[0x40], 0x184) | (val & 0xBFF4));
				}
				return;
                        case REG_TM0CNTL :
                        case REG_TM1CNTL :
                        case REG_TM2CNTL :
                        case REG_TM3CNTL :
				core->Timers[(adr>>2)&3].Reload = val;
				return;
                        case REG_TM0CNTH :
                        case REG_TM1CNTH :
                        case REG_TM2CNTH :
                        case REG_TM3CNTH :
				if(val&0x80)
				{
				  core->Timers[((adr-2)>>2)&0x3].Counter = core->Timers[((adr-2)>>2)&0x3].Reload;
				}
				core->Timers[((adr-2)>>2)&0x3].On = val & 0x80;
				switch(val&7)
				{
				case 0 :
					core->Timers[((adr-2)>>2)&0x3].Mode = 0+1;//proc;
					break;
				case 1 :
					core->Timers[((adr-2)>>2)&0x3].Mode = 6+1;//proc;
					break;
				case 2 :
					core->Timers[((adr-2)>>2)&0x3].Mode = 8+1;//proc;
					break;
				case 3 :
					core->Timers[((adr-2)>>2)&0x3].Mode = 10+1;//proc;
					break;
				default :
					core->Timers[((adr-2)>>2)&0x3].Mode = 0xFFFF;
					break;
				}
				if(!(val & 0x80))
				core->Timers[((adr-2)>>2)&0x3].Run = FALSE;
				T1WriteWord(core->MemMap[0x40], adr & 0xFFF, val);
				return;
                        case REG_DISPA_DISPCNT+2 : 
				{
				//execute = FALSE;
				u32 v = (T1ReadLong(core->MemMap[0x40], 0) & 0xFFFF) | ((u32) val << 16);
				T1WriteLong(core->MemMap[0x40], 0, v);
				}
				return;
                        case REG_DISPA_DISPCNT :
				if(proc == ARMCPU_ARM9)
				{
				u32 v = (T1ReadLong(core->MemMap[0x40], 0) & 0xFFFF0000) | val;
				T1WriteLong(core->MemMap[0x40], 0, v);
				}
				return;
                        case REG_DISPA_DISPCAPCNT :
				return;
                        case REG_DISPB_DISPCNT+2 : 
				if(proc == ARMCPU_ARM9)
				{
				//execute = FALSE;
				u32 v = (T1ReadLong(core->MemMap[0x40], 0x1000) & 0xFFFF) | ((u32) val << 16);
				T1WriteLong(core->MemMap[0x40], 0x1000, v);
				}
				return;
                        case REG_DISPB_DISPCNT :
				{
				u32 v = (T1ReadLong(core->MemMap[0x40], 0x1000) & 0xFFFF0000) | val;
				T1WriteLong(core->MemMap[0x40], 0x1000, v);
				}
				return;

        case REG_DMA0CNTH :
        case REG_DMA1CNTH :
        case REG_DMA2CNTH :
        case REG_DMA3CNTH :
        {
          const u32 regIdx = adr & 0xff;
          const u32 chanId = (regIdx - (REG_DMA0CNTH & 0xff)) / 12;
          T1WriteWord(core->MemMap[0x40], regIdx, val);
          core->DMA[chanId].Src = T1ReadLong(core->MemMap[0x40], regIdx - REG_DMA0CNTH + REG_DMA0SAD);
          core->DMA[chanId].Dst = T1ReadLong(core->MemMap[0x40], regIdx - REG_DMA0CNTH + REG_DMA0DAD);
          const u32 v = T1ReadLong(core->MemMap[0x40], regIdx - REG_DMA0CNTH + REG_DMA0CNTL);
          core->DMA[chanId].StartTime = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
          core->DMA[chanId].Crt = v;
          if(core->DMA[chanId].StartTime == 0)
              MMU_doDMA(state, proc, chanId);
        }
        return;
			default :
				T1WriteWord(core->MemMap[0x40], adr & core->MemMask[(adr>>20)&0xFF], val);
				return;
		}
	}
	T1WriteWord(core->MemMap[(adr>>20)&0xFF], adr & core->MemMask[(adr>>20)&0xFF], val);
} 


void FASTCALL MMU_write32(NDS_state *state, u32 proc, u32 adr, u32 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc==ARMCPU_ARM9)&((adr&(~0x3FFF))==state->MMU->DTCMRegion))
	{
		T1WriteLong(state->MMU->ARM9Mem->ARM9_DTCM, adr & 0x3FFF, val);
		return ;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000)) {
	   cflash_write(state,adr,val);
	   return;
	}

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteLong(state, adr, val);
              return;
           }
        }

		if ((adr & 0xFF800000) == 0x04800000) {
		/* access to non regular hw registers */
		/* return to not overwrite valid data */
			return ;
		} ;


  MMU_Core_struct* const core = state->MMU->Cores + proc;
	if((adr>>24)==4)
	{
		if (adr >= 0x04000400 && adr < 0x04000440)
		{
			// Geometry commands (aka Dislay Lists) - Parameters:X
			((u32 *)(core->MemMap[0x40]))[0x400>>2] = val;
		}
		else
		switch(adr)
		{
			case REG_DISPA_WININ: 	 
			case REG_DISPB_WININ:
			case REG_DISPA_BLDCNT:
			case REG_DISPB_BLDCNT:
				break;
      case REG_DISPA_DISPCNT :
				T1WriteLong(core->MemMap[0x40], 0, val);
				return;
				
      case REG_DISPB_DISPCNT : 
				T1WriteLong(core->MemMap[0x40], 0x1000, val);
				return;
			case REG_VRAMCNTA:
			case REG_VRAMCNTE:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				MMU_write8(state,proc,adr+2,val >> 16) ;
				MMU_write8(state,proc,adr+3,val >> 24) ;
				return ;
			case REG_VRAMCNTI:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				return ;

                        case REG_IME : {
			        u32 old_val = core->reg_IME;
				u32 new_val = val & 1;
				core->reg_IME = new_val;
				T1WriteLong(core->MemMap[0x40], 0x208, val);
				if ( new_val && old_val != new_val) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( core->reg_IE & core->reg_IF) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			}
				
			case REG_IE :
				core->reg_IE = val;
				if ( core->reg_IME) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( core->reg_IE & core->reg_IF) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			
			case REG_IF :
				core->reg_IF &= (~val);
				return;
                        case REG_TM0CNTL :
                        case REG_TM1CNTL :
                        case REG_TM2CNTL :
                        case REG_TM3CNTL :
				core->Timers[(adr>>2)&0x3].Reload = (u16)val;
				if(val&0x800000)
				{
					core->Timers[(adr>>2)&0x3].Counter = core->Timers[(adr>>2)&0x3].Reload;
				}
				core->Timers[(adr>>2)&0x3].On = val & 0x800000;
				switch((val>>16)&7)
				{
					case 0 :
					core->Timers[(adr>>2)&0x3].Mode = 0+1;//proc;
					break;
					case 1 :
					core->Timers[(adr>>2)&0x3].Mode = 6+1;//proc;
					break;
					case 2 :
					core->Timers[(adr>>2)&0x3].Mode = 8+1;//proc;
					break;
					case 3 :
					core->Timers[(adr>>2)&0x3].Mode = 10+1;//proc;
					break;
					default :
					core->Timers[(adr>>2)&0x3].Mode = 0xFFFF;
					break;
				}
				if(!(val & 0x800000))
				{
					core->Timers[(adr>>2)&0x3].Run = FALSE;
				}
				T1WriteLong(core->MemMap[0x40], adr & 0xFFF, val);
				return;
                        case REG_DIVDENOM :
				{
                                        u16 cnt;
					s64 num = 0;
					s64 den = 1;
					s64 res;
					s64 mod;
					T1WriteLong(core->MemMap[0x40], 0x298, val);
                                        cnt = T1ReadWord(core->MemMap[0x40], 0x280);
					switch(cnt&3)
					{
					case 0:
					{
						num = (s64) (s32) T1ReadLong(core->MemMap[0x40], 0x290);
						den = (s64) (s32) T1ReadLong(core->MemMap[0x40], 0x298);
					}
					break;
					case 1:
					{
						num = (s64) T1ReadQuad(core->MemMap[0x40], 0x290);
						den = (s64) (s32) T1ReadLong(core->MemMap[0x40], 0x298);
					}
					break;
					case 2:
					{
						return;
					}
					break;
					default: 
						break;
					}
					if(den==0)
					{
						res = 0;
						mod = 0;
						cnt |= 0x4000;
						cnt &= 0x7FFF;
					}
					else
					{
						res = num / den;
						mod = num % den;
						cnt &= 0x3FFF;
					}
					DIVLOG("BOUT1 %08X%08X / %08X%08X = %08X%08X\r\n", (u32)(num>>32), (u32)num, 
											(u32)(den>>32), (u32)den, 
											(u32)(res>>32), (u32)res);
					T1WriteLong(core->MemMap[0x40], 0x2A0, (u32) res);
					T1WriteLong(core->MemMap[0x40], 0x2A4, (u32) (res >> 32));
					T1WriteLong(core->MemMap[0x40], 0x2A8, (u32) mod);
					T1WriteLong(core->MemMap[0x40], 0x2AC, (u32) (mod >> 32));
					T1WriteLong(core->MemMap[0x40], 0x280, cnt);
				}
				return;
                        case REG_DIVDENOM+4 :
			{
                                u16 cnt;
				s64 num = 0;
				s64 den = 1;
				s64 res;
				s64 mod;
				T1WriteLong(core->MemMap[0x40], 0x29C, val);
                                cnt = T1ReadWord(core->MemMap[0x40], 0x280);
				switch(cnt&3)
				{
				case 0:
				{
					return;
				}
				break;
				case 1:
				{
					return;
				}
				break;
				case 2:
				{
					num = (s64) T1ReadQuad(core->MemMap[0x40], 0x290);
					den = (s64) T1ReadQuad(core->MemMap[0x40], 0x298);
				}
				break;
				default: 
					break;
				}
				if(den==0)
				{
					res = 0;
					mod = 0;
					cnt |= 0x4000;
					cnt &= 0x7FFF;
				}
				else
				{
					res = num / den;
					mod = num % den;
					cnt &= 0x3FFF;
				}
				DIVLOG("BOUT2 %08X%08X / %08X%08X = %08X%08X\r\n", (u32)(num>>32), (u32)num, 
										(u32)(den>>32), (u32)den, 
										(u32)(res>>32), (u32)res);
				T1WriteLong(core->MemMap[0x40], 0x2A0, (u32) res);
				T1WriteLong(core->MemMap[0x40], 0x2A4, (u32) (res >> 32));
				T1WriteLong(core->MemMap[0x40], 0x2A8, (u32) mod);
				T1WriteLong(core->MemMap[0x40], 0x2AC, (u32) (mod >> 32));
				T1WriteLong(core->MemMap[0x40], 0x280, cnt);
			}
			return;
                        case REG_SQRTPARAM :
				{
                                        u16 cnt;
					u64 v = 1;
					//execute = FALSE;
					T1WriteLong(core->MemMap[0x40], 0x2B8, val);
                                        cnt = T1ReadWord(core->MemMap[0x40], 0x2B0);
					switch(cnt&1)
					{
					case 0:
						v = (u64) T1ReadLong(core->MemMap[0x40], 0x2B8);
						break;
					case 1:
						return;
					}
					T1WriteLong(core->MemMap[0x40], 0x2B4, (u32) isqrt64(v));
					T1WriteLong(core->MemMap[0x40], 0x2B0, cnt & 0x7FFF);
					SQRTLOG("BOUT1 sqrt(%08X%08X) = %08X\r\n", (u32)(v>>32), (u32)v, 
										T1ReadLong(core->MemMap[0x40], 0x2B4));
				}
				return;
                        case REG_SQRTPARAM+4 :
				{
                                        u16 cnt;
					u64 v = 1;
					T1WriteLong(core->MemMap[0x40], 0x2BC, val);
                                        cnt = T1ReadWord(core->MemMap[0x40], 0x2B0);
					switch(cnt&1)
					{
					case 0:
						return;
						//break;
					case 1:
						v = T1ReadQuad(core->MemMap[0x40], 0x2B8);
						break;
					}
					T1WriteLong(core->MemMap[0x40], 0x2B4, (u32) isqrt64(v));
					T1WriteLong(core->MemMap[0x40], 0x2B0, cnt & 0x7FFF);
					SQRTLOG("BOUT2 sqrt(%08X%08X) = %08X\r\n", (u32)(v>>32), (u32)v, 
										T1ReadLong(core->MemMap[0x40], 0x2B4));
				}
				return;
                        case REG_IPCSYNC :
				{
					//execute=FALSE;
					u32 remote = (proc+1)&1;
          MMU_Core_struct* const remote_core = state->MMU->Cores + remote;
					u32 IPCSYNC_remote = T1ReadLong(remote_core->MemMap[0x40], 0x180);
					T1WriteLong(core->MemMap[0x40], 0x180, (val&0xFFF0)|((IPCSYNC_remote>>8)&0xF));
					T1WriteLong(remote_core->MemMap[0x40], 0x180, (IPCSYNC_remote&0xFFF0)|((val>>8)&0xF));
					remote_core->reg_IF |= ((IPCSYNC_remote & (1<<14))<<2) & ((val & (1<<13))<<3);// & (MMU->reg_IME[remote] << 16);// & (MMU->reg_IE[remote] & (1<<16));//
				}
				return;
                        case REG_IPCFIFOCNT :
							{
  				u32 remote = (proc+1)&1;
          MMU_Core_struct* const remote_core = state->MMU->Cores + remote;
					u32 cnt_l = T1ReadWord(core->MemMap[0x40], 0x184) ;
					u32 cnt_r = T1ReadWord(remote_core->MemMap[0x40], 0x184) ;
					if ((val & 0x8000) && !(cnt_l & 0x8000))
					{
						/* this is the first init, the other side didnt init yet */
						/* so do a complete init */
						FIFOInit(state->MMU->fifos + (IPCFIFO+proc));
						T1WriteWord(core->MemMap[0x40], 0x184,0x8101) ;
						/* and then handle it as usual */
					}
				if(val & 0x4008)
				{
					FIFOInit(state->MMU->fifos + (IPCFIFO+((proc+1)&1)));
					T1WriteWord(core->MemMap[0x40], 0x184, (cnt_l & 0x0301) | (val & 0x8404) | 1);
					T1WriteWord(remote_core->MemMap[0x40], 0x184, (cnt_r & 0xC507) | 0x100);
					core->reg_IF |= ((val & 4)<<15);// & (MMU->reg_IME[proc]<<17);// & (MMU->reg_IE[proc]&0x20000);//
					return;
				}
				T1WriteWord(core->MemMap[0x40], 0x184, val & 0xBFF4);
				//execute = FALSE;
				return;
							}
                        case REG_IPCFIFOSEND :
				{
					u16 IPCFIFO_CNT = T1ReadWord(core->MemMap[0x40], 0x184);
					if(IPCFIFO_CNT&0x8000)
					{
					//if(val==43) execute = FALSE;
					u32 remote = (proc+1)&1;
          MMU_Core_struct* const remote_core = state->MMU->Cores + remote;
					u32 fifonum = IPCFIFO+remote;
                                        u16 IPCFIFO_CNT_remote;
					FIFOAdd(state->MMU->fifos + fifonum, val);
					IPCFIFO_CNT = (IPCFIFO_CNT & 0xFFFC) | (state->MMU->fifos[fifonum].full<<1);
                                        IPCFIFO_CNT_remote = T1ReadWord(remote_core->MemMap[0x40], 0x184);
					IPCFIFO_CNT_remote = (IPCFIFO_CNT_remote & 0xFCFF) | (state->MMU->fifos[fifonum].full<<10);
					T1WriteWord(core->MemMap[0x40], 0x184, IPCFIFO_CNT);
					T1WriteWord(remote_core->MemMap[0x40], 0x184, IPCFIFO_CNT_remote);
					remote_core->reg_IF |= ((IPCFIFO_CNT_remote & (1<<10))<<8);// & (MMU->reg_IME[remote] << 18);// & (MMU->reg_IE[remote] & 0x40000);//
					//execute = FALSE;
					}
				}
				return;
  		case REG_DMA0CNTL :
  		case REG_DMA1CNTL :
  		case REG_DMA2CNTL :
  		case REG_DMA3CNTL :
        {
           const u32 regIdx = adr & 0xff;
           const u32 chanId = (regIdx - (REG_DMA0CNTL & 0xff)) / 12;
           core->DMA[chanId].Src = T1ReadLong(core->MemMap[0x40], regIdx - REG_DMA0CNTL + REG_DMA0SAD);
           core->DMA[chanId].Dst = T1ReadLong(core->MemMap[0x40], regIdx - REG_DMA0CNTL + REG_DMA0DAD);
           core->DMA[chanId].StartTime = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
           core->DMA[chanId].Crt = val;
           T1WriteLong(core->MemMap[0x40], regIdx, val);
           if (core->DMA[chanId].StartTime == 0 || core->DMA[chanId].StartTime == 7)		// Start Immediately
             MMU_doDMA(state, proc, chanId);
        }
        return;
     case REG_GCROMCTRL :
				{
					int i;

                                        if(MEM_8(core->MemMap, REG_GCCMDOUT) == 0xB7)
					{
                                                core->dscard.adress = (MEM_8(core->MemMap, REG_GCCMDOUT+1) << 24) | (MEM_8(core->MemMap, REG_GCCMDOUT+2) << 16) | (MEM_8(core->MemMap, REG_GCCMDOUT+3) << 8) | (MEM_8(core->MemMap, REG_GCCMDOUT+4));
						core->dscard.transfer_count = 0x80;// * ((val>>24)&7));
					}
                                        else if (MEM_8(core->MemMap, REG_GCCMDOUT) == 0xB8)
                                        {
                                                // Get ROM chip ID
                                                val |= 0x800000; // Data-Word Status
                                                T1WriteLong(core->MemMap[(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
                                                core->dscard.adress = 0;
                                        }
					else
					{
                                                LOG("CARD command: %02X\n", MEM_8(core->MemMap, REG_GCCMDOUT));
					}
					
					//CARDLOG("%08X : %08X %08X\r\n", adr, val, adresse[proc]);
                    val |= 0x00800000;
					
					if(core->dscard.adress == 0)
					{
                                                val &= ~0x80000000; 
                                                T1WriteLong(core->MemMap[(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
						return;
					}
                                        T1WriteLong(core->MemMap[(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
										
					/* launch DMA if start flag was set to "DS Cart" */
					if(proc == ARMCPU_ARM7) i = 2;
					else i = 5;
					
					if(proc == ARMCPU_ARM9 && core->DMA[0].StartTime == i)	/* dma0/1 on arm7 can't start on ds cart event */
					{
						MMU_doDMA(state, proc, 0);
						return;
					}
					else if(proc == ARMCPU_ARM9 && core->DMA[1].StartTime == i)
					{
						MMU_doDMA(state, proc, 1);
						return;
					}
					else if(core->DMA[2].StartTime == i)
					{
						MMU_doDMA(state, proc, 2);
						return;
					}
					else if(core->DMA[3].StartTime == i)
					{
						MMU_doDMA(state, proc, 3);
						return;
					}
					return;

				}
				return;
								case REG_DISPA_DISPCAPCNT :
				if(proc == ARMCPU_ARM9)
				{
					T1WriteLong(state->MMU->ARM9Mem->ARM9_REG, 0x64, val);
				}
				return;
			case REG_DISPA_DISPMMEMFIFO:
			{
				// NOTE: right now, the capture unit is not taken into account,
				//       I don't know is it should be handled here or 
			
				FIFOAdd(state->MMU->fifos + MAIN_MEMORY_DISP_FIFO, val);
				break;
			}
			//case 0x21FDFF0 :  if(val==0) execute = FALSE;
			//case 0x21FDFB0 :  if(val==0) execute = FALSE;
			default :
				T1WriteLong(core->MemMap[0x40], adr & core->MemMask[(adr>>20)&0xFF], val);
				return;
		}
	}
	T1WriteLong(core->MemMap[(adr>>20)&0xFF], adr & core->MemMask[(adr>>20)&0xFF], val);
}


void FASTCALL MMU_doDMA(NDS_state *state, u32 proc, u32 num)
{
  MMU_Core_struct* const core = state->MMU->Cores + proc;
  
	u32 src = core->DMA[num].Src;
	u32 dst = core->DMA[num].Dst;
        u32 taille;

	if(src==dst)
	{
		T1WriteLong(core->MemMap[0x40], 0xB8 + (0xC*num), T1ReadLong(core->MemMap[0x40], 0xB8 + (0xC*num)) & 0x7FFFFFFF);
		return;
	}
	
	if((!(core->DMA[num].Crt&(1<<31)))&&(!(core->DMA[num].Crt&(1<<25))))
	{       /* not enabled and not to be repeated */
		core->DMA[num].StartTime = 0;
		core->DMA[num].Cycle = 0;
		//MMU->DMA[proc].Channels[num].Active = FALSE;
		return;
	}
	
	
	/* word count */
	taille = (core->DMA[num].Crt&0xFFFF);
	
	// If we are in "Main memory display" mode just copy an entire 
	// screen (256x192 pixels). 
	//    Reference:  http://nocash.emubase.de/gbatek.htm#dsvideocaptureandmainmemorydisplaymode
	//       (under DISP_MMEM_FIFO)
	if ((core->DMA[num].StartTime==4) &&		// Must be in main memory display mode
		(taille==4) &&							// Word must be 4
		(((core->DMA[num].Crt>>26)&1) == 1))	// Transfer mode must be 32bit wide
		taille = 256*192/2;
	
	if(core->DMA[num].StartTime == 5)
		taille *= 0x80;
	
	core->DMA[num].Cycle = taille + state->nds->cycles;
	core->DMA[num].Active = TRUE;
	
	DMALOG("proc %d, dma %d src %08X dst %08X start %d taille %d repeat %s %08X\r\n",
		proc, num, src, dst, core->DMA[num].StartTime, taille,
		(core->DMA[num].Crt&(1<<25))?"on":"off",core->DMA[num].Crt);
	
	if(!(core->DMA[num].Crt&(1<<25)))
		core->DMA[num].StartTime = 0;
	
	// transfer
	{
		u32 i=0;
		// 32 bit or 16 bit transfer ?
		int sz = ((core->DMA[num].Crt>>26)&1)? 4 : 2;
		int dstinc,srcinc;
		int u=(core->DMA[num].Crt>>21);
		switch(u & 0x3) {
			case 0 :  dstinc =  sz; break;
			case 1 :  dstinc = -sz; break;
			case 2 :  dstinc =   0; break;
			case 3 :  dstinc =  sz; break; //reload
		}
		switch((u >> 2)&0x3) {
			case 0 :  srcinc =  sz; break;
			case 1 :  srcinc = -sz; break;
			case 2 :  srcinc =   0; break;
			case 3 :  // reserved
				return;
		}
		if ((core->DMA[num].Crt>>26)&1)
			for(; i < taille; ++i)
			{
				MMU_write32(state, proc, dst, MMU_read32(state, proc, src));
				dst += dstinc;
				src += srcinc;
			}
		else
			for(; i < taille; ++i)
			{
				MMU_write16(state, proc, dst, MMU_read16(state, proc, src));
				dst += dstinc;
				src += srcinc;
			}
	}
}

#ifdef MMU_ENABLE_ACL

INLINE void check_access(NDS_state *state, u32 adr, u32 access) {
	/* every other mode: sys */
	access |= 1;
	if ((state->NDS_ARM9->CPSR.val & 0x1F) == 0x10) {
		/* is user mode access */
		access ^= 1 ;
	}
	if (armcp15_isAccessAllowed((armcp15_t *)state->NDS_ARM9->coproc[15],adr,access)==FALSE) {
		state->execute = FALSE ;
	}
}
INLINE void check_access_write(NDS_State *state, u32 adr) {
	u32 access = CP15_ACCESS_WRITE;
	check_access(state, adr, access)
}

u8 FASTCALL MMU_read8_acl(NDS_state *state, u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(state, adr, access);
	return MMU_read8(state,proc,adr);
}
u16 FASTCALL MMU_read16_acl(NDS_state *state, u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(state, adr, access);
	return MMU_read16(state,proc,adr);
}
u32 FASTCALL MMU_read32_acl(NDS_state *state, u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(state, adr, access);
	return MMU_read32(state,proc,adr);
}

void FASTCALL MMU_write8_acl(NDS_state *state, u32 proc, u32 adr, u8 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(state,adr);
	MMU_write8(state,proc,adr,val);
}
void FASTCALL MMU_write16_acl(NDS_state *state, u32 proc, u32 adr, u16 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(state,adr);
	MMU_write16(state,proc,adr,val) ;
}
void FASTCALL MMU_write32_acl(NDS_state *state, u32 proc, u32 adr, u32 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(state,adr);
	MMU_write32(state,proc,adr,val) ;
}
#endif
