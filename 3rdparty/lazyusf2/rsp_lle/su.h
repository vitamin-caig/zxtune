/******************************************************************************\
* Project:  MSP Emulation Table for Scalar Unit Operations                     *
* Authors:  Iconoclast                                                         *
* Release:  2013.12.10                                                         *
* License:  CC0 Public Domain Dedication                                       *
*                                                                              *
* To the extent possible under law, the author(s) have dedicated all copyright *
* and related and neighboring rights to this software to the public domain     *
* worldwide. This software is distributed without any warranty.                *
*                                                                              *
* You should have received a copy of the CC0 Public Domain Dedication along    *
* with this software.                                                          *
* If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.             *
\******************************************************************************/
#ifndef _SU_H
#define _SU_H

/*
 * RSP virtual registers (of scalar unit)
 * The most important are the 32 general-purpose scalar registers.
 * We have the convenience of using a 32-bit machine (Win32) to emulate
 * another 32-bit machine (MIPS/N64), so the most natural way to accurately
 * emulate the scalar GPRs is to use the standard `int` type.  Situations
 * specifically requiring sign-extension or lack thereof are forcibly
 * applied as defined in the MIPS quick reference card and user manuals.
 * Remember that these are not the same "GPRs" as in the MIPS ISA and totally
 * abandon their designated purposes on the master CPU host (the VR4300),
 * hence most of the MIPS names "k0, k1, t0, t1, v0, v1 ..." no longer apply.
 */

#include "rsp.h"

NOINLINE static void res_S(struct rsp_core* sp)
{
    (void)sp;
    return;
}

#ifdef EMULATE_STATIC_PC
#define BASE_OFF    0x000
#else
#define BASE_OFF    0x004
#endif

#define SLOT_OFF    (BASE_OFF + 0x000)
#define LINK_OFF    (BASE_OFF + 0x004)
static void set_PC(struct rsp_core* sp, int address)
{
    sp->temp_PC = (address & 0xFFC);
#ifndef EMULATE_STATIC_PC
    sp->stage = 1;
#endif
    return;
}

#if ANDROID
#define MASK_SA(sa) (sa & 31)
/* Force masking in software. */
#else
#define MASK_SA(sa) (sa)
/* Let hardware architecture do the mask for us. */
#endif

#if (0)
#define ENDIAN   0
#else
#define ENDIAN  ~0
#endif
#define BES(address)    ((address) ^ ((ENDIAN) & 03))
#define HES(address)    ((address) ^ ((ENDIAN) & 02))
#define MES(address)    ((address) ^ ((ENDIAN) & 01))
#define WES(address)    ((address) ^ ((ENDIAN) & 00))
#define SR_B(s, i)      (*(byte *)(((byte *)(sp->SR + s)) + BES(i)))
#define SR_S(s, i)      (*(short *)(((byte *)(sp->SR + s)) + HES(i)))
#define SE(x, b)        (-((signed int)x & (1 << b)) | (x & ~(~0 << b)))
#define ZE(x, b)        (+(x & (1 << b)) | (x & ~(~0 << b)))

static void ULW(struct rsp_core*, int rd, uint32_t addr);
static void USW(struct rsp_core*, int rs, uint32_t addr);

/*
 * All other behaviors defined below this point in the file are specific to
 * the SGI N64 extension to the MIPS R4000 and are not entirely implemented.
 */

/*** Scalar, Coprocessor Operations (system control) ***/

static void MFC0(struct rsp_core* sp, int rt, int rd)
{
    sp->SR[rt] = *(sp->CR[rd]);
    sp->SR[0] = 0x00000000;
    if (rd == 0x7) /* SP_SEMAPHORE_REG */
    {
        if (CFG_MEND_SEMAPHORE_LOCK == 0)
            return;
        sp->regs[SP_SEMAPHORE_REG] = 0x00000001;
        sp->regs[SP_STATUS_REG] |= 0x00000001; /* temporary bit to break CPU */
        return;
    }
    if (rd == 0x4) /* SP_STATUS_REG */
    {
        if (CFG_WAIT_FOR_CPU_HOST == 0)
            return;
#ifdef WAIT_FOR_CPU_HOST
        ++sp->MFC0_count[rt];
        if (sp->MFC0_count[rt] > 07)
            sp->regs[SP_STATUS_REG] |= 0x00000001; /* Let OS restart the task. */
#endif
    }
    return;
}

static void MT_DMA_CACHE(struct rsp_core* sp, int rt)
{
    sp->regs[SP_MEM_ADDR_REG] = sp->SR[rt] & 0xFFFFFFF8; /* & 0x00001FF8 */
    return; /* Reserved upper bits are ignored during DMA R/W. */
}
static void MT_DMA_DRAM(struct rsp_core* sp, int rt)
{
    sp->regs[SP_DRAM_ADDR_REG] = sp->SR[rt] & 0xFFFFFFF8; /* & 0x00FFFFF8 */
    return; /* Let the reserved bits get sent, but the pointer is 24-bit. */
}
void SP_DMA_READ(struct rsp_core* sp);
static void MT_DMA_READ_LENGTH(struct rsp_core* sp, int rt)
{
    sp->regs[SP_RD_LEN_REG] = sp->SR[rt] | 07;
    SP_DMA_READ(sp);
    return;
}
void SP_DMA_WRITE(struct rsp_core* sp);
static void MT_DMA_WRITE_LENGTH(struct rsp_core* sp, int rt)
{
    sp->regs[SP_WR_LEN_REG] = sp->SR[rt] | 07;
    SP_DMA_WRITE(sp);
    return;
}
static void MT_SP_STATUS(struct rsp_core* sp, int rt)
{
    if (sp->SR[rt] & 0xFE000040)
        message(sp->r4300->state, "MTC0\nSP_STATUS", 2);
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00000001) <<  0);
    sp->regs[SP_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00000002) <<  0);
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00000004) <<  1);
    sp->r4300->mi.regs[MI_INTR_REG] &= ~((sp->SR[rt] & 0x00000008) >> 3); /* SP_CLR_INTR */
    sp->r4300->mi.regs[MI_INTR_REG] |=  ((sp->SR[rt] & 0x00000010) >> 4); /* SP_SET_INTR */
    sp->regs[SP_STATUS_REG] |= (sp->SR[rt] & 0x00000010) >> 4; /* int set halt */
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00000020) <<  5);
 /* sp->regs[SP_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00000040) <<  5); */
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00000080) <<  6);
    sp->regs[SP_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00000100) <<  6);
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00000200) <<  7);
    sp->regs[SP_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00000400) <<  7);
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00000800) <<  8);
    sp->regs[SP_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00001000) <<  8);
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00002000) <<  9);
    sp->regs[SP_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00004000) <<  9);
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00008000) << 10);
    sp->regs[SP_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00010000) << 10);
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00020000) << 11);
    sp->regs[SP_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00040000) << 11);
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00080000) << 12);
    sp->regs[SP_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00100000) << 12);
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00200000) << 13);
    sp->regs[SP_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00400000) << 13);
    sp->regs[SP_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00800000) << 14);
    sp->regs[SP_STATUS_REG] |=  (!!(sp->SR[rt] & 0x01000000) << 14);
    return;
}
static void MT_SP_RESERVED(struct rsp_core* sp, int rt)
{
    const uint32_t source = sp->SR[rt] & 0x00000000; /* forced (zilmar, dox) */

    sp->regs[SP_SEMAPHORE_REG] = source;
    return;
}
static void MT_CMD_START(struct rsp_core* sp, int rt)
{
    const uint32_t source = sp->SR[rt] & 0xFFFFFFF8; /* Funnelcube demo */

    if (sp->dp->dpc_regs[DPC_BUFBUSY_REG]) /* lock hazards not implemented */
        message(sp->r4300->state, "MTC0\nCMD_START", 0);
    sp->dp->dpc_regs[DPC_END_REG] = sp->dp->dpc_regs[DPC_CURRENT_REG] = sp->dp->dpc_regs[DPC_START_REG] = source;
    return;
}
static void MT_CMD_END(struct rsp_core* sp, int rt)
{
    if (sp->dp->dpc_regs[DPC_BUFBUSY_REG])
        message(sp->r4300->state, "MTC0\nCMD_END", 0); /* This is just CA-related. */
    sp->dp->dpc_regs[DPC_END_REG] = sp->SR[rt] & 0xFFFFFFF8;
    return;
}
static void MT_CMD_STATUS(struct rsp_core* sp, int rt)
{
    if (sp->SR[rt] & 0xFFFFFD80) /* unsupported or reserved bits */
        message(sp->r4300->state, "MTC0\nCMD_STATUS", 2);
    sp->dp->dpc_regs[DPC_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00000001) << 0);
    sp->dp->dpc_regs[DPC_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00000002) << 0);
    sp->dp->dpc_regs[DPC_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00000004) << 1);
    sp->dp->dpc_regs[DPC_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00000008) << 1);
    sp->dp->dpc_regs[DPC_STATUS_REG] &= ~(!!(sp->SR[rt] & 0x00000010) << 2);
    sp->dp->dpc_regs[DPC_STATUS_REG] |=  (!!(sp->SR[rt] & 0x00000020) << 2);
/* Some NUS-CIC-6105 SP tasks try to clear some zeroed DPC registers. */
    sp->dp->dpc_regs[DPC_TMEM_REG]     &= !(sp->SR[rt] & 0x00000040) * -1;
 /* sp->dp->dpc_regs[DPC_PIPEBUSY_REG] &= !(sp->SR[rt] & 0x00000080) * -1; */
 /* sp->dp->dpc_regs[DPC_BUFBUSY_REG]  &= !(sp->SR[rt] & 0x00000100) * -1; */
    sp->dp->dpc_regs[DPC_CLOCK_REG]    &= !(sp->SR[rt] & 0x00000200) * -1;
    return;
}
static void MT_CMD_CLOCK(struct rsp_core* sp, int rt)
{
    message(sp->r4300->state, "MTC0\nCMD_CLOCK", 1); /* read-only?? */
    sp->dp->dpc_regs[DPC_CLOCK_REG] = sp->SR[rt];
    return; /* Appendix says this is RW; elsewhere it says R. */
}
static void MT_READ_ONLY(struct rsp_core* sp, int rt)
{
    (void)sp;
    (void)rt;
    //char text[64];

    //sprintf(text, "MTC0\nInvalid write attempt.\nsp->SR[%i] = 0x%08X", rt, sp->SR[rt]);
    //message(state, text, 2);
    return;
}

static void (*MTC0[16])(struct rsp_core*, int) = {
MT_DMA_CACHE       ,MT_DMA_DRAM        ,MT_DMA_READ_LENGTH ,MT_DMA_WRITE_LENGTH,
MT_SP_STATUS       ,MT_READ_ONLY       ,MT_READ_ONLY       ,MT_SP_RESERVED,
MT_CMD_START       ,MT_CMD_END         ,MT_READ_ONLY       ,MT_CMD_STATUS,
MT_CMD_CLOCK       ,MT_READ_ONLY       ,MT_READ_ONLY       ,MT_READ_ONLY
};
void SP_DMA_READ(struct rsp_core* sp)
{
    // Write to RSP, read from RDRAM
    dma_sp_write(sp);
    sp->regs[SP_DMA_BUSY_REG] = 0x00000000;
    sp->regs[SP_STATUS_REG] &= ~0x00000004; /* SP_STATUS_DMABUSY */
}
void SP_DMA_WRITE(struct rsp_core* sp)
{
    // Read from RSP, write to RDRAM
    dma_sp_read(sp);
    sp->regs[SP_DMA_BUSY_REG] = 0x00000000;
    sp->regs[SP_STATUS_REG] &= ~0x00000004; /* SP_STATUS_DMABUSY */
}

/*** Scalar, Coprocessor Operations (vector unit) ***/

/*
 * Since RSP vectors are stored 100% accurately as big-endian arrays for the
 * proper vector operation math to be done, LWC2 and SWC2 emulation code will
 * have to look a little different.  zilmar's method is to distort the endian
 * using an array of unions, permitting hacked byte- and halfword-precision.
 */

/*
 * Universal byte-access macro for 16*8 halfword vectors.
 * Use this macro if you are not sure whether the element is odd or even.
 */
#define VR_B(vt,element)    (*(byte *)((byte *)(sp->VR[vt]) + MES(element)))

/*
 * Optimized byte-access macros for the vector registers.
 * Use these ONLY if you know the element is even (or odd in the second).
 */
#define VR_A(vt,element)    (*(byte *)((byte *)(sp->VR[vt]) + element + MES(0x0)))
#define VR_U(vt,element)    (*(byte *)((byte *)(sp->VR[vt]) + element - MES(0x0)))

/*
 * Optimized halfword-access macro for indexing eight-element vectors.
 * Use this ONLY if you know the element is even, not odd.
 *
 * If the four-bit element is odd, then there is no solution in one hit.
 */
#define VR_S(vt,element)    (*(short *)((byte *)(sp->VR[vt]) + element))

static unsigned short get_VCO(struct rsp_core* sp);
static unsigned short get_VCC(struct rsp_core* sp);
static unsigned char get_VCE(struct rsp_core* sp);
static void set_VCO(struct rsp_core* sp, unsigned short VCO);
static void set_VCC(struct rsp_core* sp, unsigned short VCC);
//static void set_VCE(struct rsp_core* sp, unsigned char VCE);

static unsigned short rwR_VCE(struct rsp_core* sp)
{ /* never saw a game try to read VCE out to a scalar GPR yet */
    register unsigned short ret_slot;

    ret_slot = 0x00 | (unsigned short)get_VCE(sp);
    return (ret_slot);
}
static void rwW_VCE(struct rsp_core* sp, unsigned short VCE)
{ /* never saw a game try to write VCE using a scalar GPR yet */
    register int i;

    VCE = 0x00 | (VCE & 0xFF);
    for (i = 0; i < 8; i++)
        sp->vce[i] = (VCE >> i) & 1;
    return;
}

static unsigned short (*R_VCF[32])(struct rsp_core*) = {
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
/* Hazard reaction barrier:  RD = (UINT16)(inst) >> 11, without &= 3. */
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE
};
static void (*W_VCF[32])(struct rsp_core*, unsigned short) = {
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
/* Hazard reaction barrier:  RD = (UINT16)(inst) >> 11, without &= 3. */
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE
};
static void MFC2(struct rsp_core* sp, int rt, int vs, int e)
{
    SR_B(rt, 2) = VR_B(vs, e);
    e = (e + 0x1) & 0xF;
    SR_B(rt, 3) = VR_B(vs, e);
    sp->SR[rt] = (signed short)(sp->SR[rt]);
    sp->SR[0] = 0x00000000;
    return;
}
static void MTC2(struct rsp_core* sp, int rt, int vd, int e)
{
    VR_B(vd, e+0x0) = SR_B(rt, 2);
    VR_B(vd, e+0x1) = SR_B(rt, 3);
    return; /* If element == 0xF, it does not matter; loads do not wrap over. */
}
static void CFC2(struct rsp_core* sp, int rt, int rd)
{
    sp->SR[rt] = (signed short)R_VCF[rd](sp);
    sp->SR[0] = 0x00000000;
    return;
}
static void CTC2(struct rsp_core* sp, int rt, int rd)
{
    W_VCF[rd](sp, sp->SR[rt] & 0x0000FFFF);
    return;
}

/*** Scalar, Coprocessor Operations (vector unit, scalar cache transfers) ***/
INLINE static void LBV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    addr = (sp->SR[base] + 1*offset) & 0x00000FFF;
    VR_B(vt, e) = sp->DMEM[BES(addr)];
    return;
}
INLINE static void LSV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    int correction;
    register uint32_t addr;
    const int e = element;

    if (e & 0x1)
    {
        message(sp->r4300->state, "LSV\nIllegal element.", 3);
        return;
    }
    addr = (sp->SR[base] + 2*offset) & 0x00000FFF;
    correction = addr % 0x004;
    if (correction == 0x003)
    {
        message(sp->r4300->state, "LSV\nWeird addr.", 3);
        return;
    }
    VR_S(vt, e) = *(short *)(sp->DMEM + addr - HES(0x000)*(correction - 1));
    return;
}
INLINE static void LLV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    int correction;
    register uint32_t addr;
    const int e = element;

    if (e & 0x1)
    {
        message(sp->r4300->state, "LLV\nOdd element.", 3);
        return;
    } /* Illegal (but still even) elements are used by Boss Game Studios. */
    addr = (sp->SR[base] + 4*offset) & 0x00000FFF;
    if (addr & 0x00000001)
    {
        message(sp->r4300->state, "LLV\nOdd addr.", 3);
        return;
    }
    correction = HES(0x000)*(addr%0x004 - 1);
    VR_S(vt, e+0x0) = *(short *)(sp->DMEM + addr - correction);
    addr = (addr + 0x00000002) & 0x00000FFF; /* F3DLX 1.23:  addr%4 is 0x002. */
    VR_S(vt, e+0x2) = *(short *)(sp->DMEM + addr + correction);
    return;
}
INLINE static void LDV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    if (e & 0x1)
    {
        message(sp->r4300->state, "LDV\nOdd element.", 3);
        return;
    } /* Illegal (but still even) elements are used by Boss Game Studios. */
    addr = (sp->SR[base] + 8*offset) & 0x00000FFF;
    switch (addr & 07)
    {
        case 00:
            VR_S(vt, e+0x0) = *(short *)(sp->DMEM + addr + HES(0x000));
            VR_S(vt, e+0x2) = *(short *)(sp->DMEM + addr + HES(0x002));
            VR_S(vt, e+0x4) = *(short *)(sp->DMEM + addr + HES(0x004));
            VR_S(vt, e+0x6) = *(short *)(sp->DMEM + addr + HES(0x006));
            return;
        case 01: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_S(vt, e+0x0) = *(short *)(sp->DMEM + addr + 0x000);
            VR_A(vt, e+0x2) = sp->DMEM[addr + 0x002 - BES(0x000)];
            VR_U(vt, e+0x3) = sp->DMEM[addr + 0x003 + BES(0x000)];
            VR_S(vt, e+0x4) = *(short *)(sp->DMEM + addr + 0x004);
            VR_A(vt, e+0x6) = sp->DMEM[addr + 0x006 - BES(0x000)];
            addr += 0x007 + BES(00);
            addr &= 0x00000FFF;
            VR_U(vt, e+0x7) = sp->DMEM[addr];
            return;
        case 02:
            VR_S(vt, e+0x0) = *(short *)(sp->DMEM + addr + 0x000 - HES(0x000));
            VR_S(vt, e+0x2) = *(short *)(sp->DMEM + addr + 0x002 + HES(0x000));
            VR_S(vt, e+0x4) = *(short *)(sp->DMEM + addr + 0x004 - HES(0x000));
            addr += 0x006 + HES(00);
            addr &= 0x00000FFF;
            VR_S(vt, e+0x6) = *(short *)(sp->DMEM + addr);
            return;
        case 03: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_A(vt, e+0x0) = sp->DMEM[addr + 0x000 - BES(0x000)];
            VR_U(vt, e+0x1) = sp->DMEM[addr + 0x001 + BES(0x000)];
            VR_S(vt, e+0x2) = *(short *)(sp->DMEM + addr + 0x002);
            VR_A(vt, e+0x4) = sp->DMEM[addr + 0x004 - BES(0x000)];
            addr += 0x005 + BES(00);
            addr &= 0x00000FFF;
            VR_U(vt, e+0x5) = sp->DMEM[addr];
            VR_S(vt, e+0x6) = *(short *)(sp->DMEM + addr + 0x001 - BES(0x000));
            return;
        case 04:
            VR_S(vt, e+0x0) = *(short *)(sp->DMEM + addr + HES(0x000));
            VR_S(vt, e+0x2) = *(short *)(sp->DMEM + addr + HES(0x002));
            addr += 0x004 + WES(00);
            addr &= 0x00000FFF;
            VR_S(vt, e+0x4) = *(short *)(sp->DMEM + addr + HES(0x000));
            VR_S(vt, e+0x6) = *(short *)(sp->DMEM + addr + HES(0x002));
            return;
        case 05: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_S(vt, e+0x0) = *(short *)(sp->DMEM + addr + 0x000);
            VR_A(vt, e+0x2) = sp->DMEM[addr + 0x002 - BES(0x000)];
            addr += 0x003;
            addr &= 0x00000FFF;
            VR_U(vt, e+0x3) = sp->DMEM[addr + BES(0x000)];
            VR_S(vt, e+0x4) = *(short *)(sp->DMEM + addr + 0x001);
            VR_A(vt, e+0x6) = sp->DMEM[addr + BES(0x003)];
            VR_U(vt, e+0x7) = sp->DMEM[addr + BES(0x004)];
            return;
        case 06:
            VR_S(vt, e+0x0) = *(short *)(sp->DMEM + addr - HES(0x000));
            addr += 0x002;
            addr &= 0x00000FFF;
            VR_S(vt, e+0x2) = *(short *)(sp->DMEM + addr + HES(0x000));
            VR_S(vt, e+0x4) = *(short *)(sp->DMEM + addr + HES(0x002));
            VR_S(vt, e+0x6) = *(short *)(sp->DMEM + addr + HES(0x004));
            return;
        case 07: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_A(vt, e+0x0) = sp->DMEM[addr - BES(0x000)];
            addr += 0x001;
            addr &= 0x00000FFF;
            VR_U(vt, e+0x1) = sp->DMEM[addr + BES(0x000)];
            VR_S(vt, e+0x2) = *(short *)(sp->DMEM + addr + 0x001);
            VR_A(vt, e+0x4) = sp->DMEM[addr + BES(0x003)];
            VR_U(vt, e+0x5) = sp->DMEM[addr + BES(0x004)];
            VR_S(vt, e+0x6) = *(short *)(sp->DMEM + addr + 0x005);
            return;
    }
}
INLINE static void SBV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    addr = (sp->SR[base] + 1*offset) & 0x00000FFF;
    sp->DMEM[BES(addr)] = VR_B(vt, e);
    return;
}
INLINE static void SSV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    addr = (sp->SR[base] + 2*offset) & 0x00000FFF;
    sp->DMEM[BES(addr)] = VR_B(vt, (e + 0x0));
    addr = (addr + 0x00000001) & 0x00000FFF;
    sp->DMEM[BES(addr)] = VR_B(vt, (e + 0x1) & 0xF);
    return;
}
INLINE static void SLV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    int correction;
    register uint32_t addr;
    const int e = element;

    if ((e & 0x1) || e > 0xC) /* must support illegal even elements in F3DEX2 */
    {
        message(sp->r4300->state, "SLV\nIllegal element.", 3);
        return;
    }
    addr = (sp->SR[base] + 4*offset) & 0x00000FFF;
    if (addr & 0x00000001)
    {
        message(sp->r4300->state, "SLV\nOdd addr.", 3);
        return;
    }
    correction = HES(0x000)*(addr%0x004 - 1);
    *(short *)(sp->DMEM + addr - correction) = VR_S(vt, e+0x0);
    addr = (addr + 0x00000002) & 0x00000FFF; /* F3DLX 0.95:  "Mario Kart 64" */
    *(short *)(sp->DMEM + addr + correction) = VR_S(vt, e+0x2);
    return;
}
INLINE static void SDV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    addr = (sp->SR[base] + 8*offset) & 0x00000FFF;
    if (e > 0x8 || (e & 0x1))
    { /* Illegal elements with Boss Game Studios publications. */
        register int i;

        for (i = 0; i < 8; i++)
            sp->DMEM[BES(addr &= 0x00000FFF)] = VR_B(vt, (e+i)&0xF);
        return;
    }
    switch (addr & 07)
    {
        case 00:
            *(short *)(sp->DMEM + addr + HES(0x000)) = VR_S(vt, e+0x0);
            *(short *)(sp->DMEM + addr + HES(0x002)) = VR_S(vt, e+0x2);
            *(short *)(sp->DMEM + addr + HES(0x004)) = VR_S(vt, e+0x4);
            *(short *)(sp->DMEM + addr + HES(0x006)) = VR_S(vt, e+0x6);
            return;
        case 01: /* "Tetrisphere" audio ucode */
            *(short *)(sp->DMEM + addr + 0x000) = VR_S(vt, e+0x0);
            sp->DMEM[addr + 0x002 - BES(0x000)] = VR_A(vt, e+0x2);
            sp->DMEM[addr + 0x003 + BES(0x000)] = VR_U(vt, e+0x3);
            *(short *)(sp->DMEM + addr + 0x004) = VR_S(vt, e+0x4);
            sp->DMEM[addr + 0x006 - BES(0x000)] = VR_A(vt, e+0x6);
            addr += 0x007 + BES(0x000);
            addr &= 0x00000FFF;
            sp->DMEM[addr] = VR_U(vt, e+0x7);
            return;
        case 02:
            *(short *)(sp->DMEM + addr + 0x000 - HES(0x000)) = VR_S(vt, e+0x0);
            *(short *)(sp->DMEM + addr + 0x002 + HES(0x000)) = VR_S(vt, e+0x2);
            *(short *)(sp->DMEM + addr + 0x004 - HES(0x000)) = VR_S(vt, e+0x4);
            addr += 0x006 + HES(0x000);
            addr &= 0x00000FFF;
            *(short *)(sp->DMEM + addr) = VR_S(vt, e+0x6);
            return;
        case 03: /* "Tetrisphere" audio ucode */
            sp->DMEM[addr + 0x000 - BES(0x000)] = VR_A(vt, e+0x0);
            sp->DMEM[addr + 0x001 + BES(0x000)] = VR_U(vt, e+0x1);
            *(short *)(sp->DMEM + addr + 0x002) = VR_S(vt, e+0x2);
            sp->DMEM[addr + 0x004 - BES(0x000)] = VR_A(vt, e+0x4);
            addr += 0x005 + BES(0x000);
            addr &= 0x00000FFF;
            sp->DMEM[addr] = VR_U(vt, e+0x5);
            *(short *)(sp->DMEM + addr + 0x001 - BES(0x000)) = VR_S(vt, 0x6);
            return;
        case 04:
            *(short *)(sp->DMEM + addr + HES(0x000)) = VR_S(vt, e+0x0);
            *(short *)(sp->DMEM + addr + HES(0x002)) = VR_S(vt, e+0x2);
            addr = (addr + 0x004) & 0x00000FFF;
            *(short *)(sp->DMEM + addr + HES(0x000)) = VR_S(vt, e+0x4);
            *(short *)(sp->DMEM + addr + HES(0x002)) = VR_S(vt, e+0x6);
            return;
        case 05: /* "Tetrisphere" audio ucode */
            *(short *)(sp->DMEM + addr + 0x000) = VR_S(vt, e+0x0);
            sp->DMEM[addr + 0x002 - BES(0x000)] = VR_A(vt, e+0x2);
            addr = (addr + 0x003) & 0x00000FFF;
            sp->DMEM[addr + BES(0x000)] = VR_U(vt, e+0x3);
            *(short *)(sp->DMEM + addr + 0x001) = VR_S(vt, e+0x4);
            sp->DMEM[addr + BES(0x003)] = VR_A(vt, e+0x6);
            sp->DMEM[addr + BES(0x004)] = VR_U(vt, e+0x7);
            return;
        case 06:
            *(short *)(sp->DMEM + addr - HES(0x000)) = VR_S(vt, e+0x0);
            addr = (addr + 0x002) & 0x00000FFF;
            *(short *)(sp->DMEM + addr + HES(0x000)) = VR_S(vt, e+0x2);
            *(short *)(sp->DMEM + addr + HES(0x002)) = VR_S(vt, e+0x4);
            *(short *)(sp->DMEM + addr + HES(0x004)) = VR_S(vt, e+0x6);
            return;
        case 07: /* "Tetrisphere" audio ucode */
            sp->DMEM[addr - BES(0x000)] = VR_A(vt, e+0x0);
            addr = (addr + 0x001) & 0x00000FFF;
            sp->DMEM[addr + BES(0x000)] = VR_U(vt, e+0x1);
            *(short *)(sp->DMEM + addr + 0x001) = VR_S(vt, e+0x2);
            sp->DMEM[addr + BES(0x003)] = VR_A(vt, e+0x4);
            sp->DMEM[addr + BES(0x004)] = VR_U(vt, e+0x5);
            *(short *)(sp->DMEM + addr + 0x005) = VR_S(vt, e+0x6);
            return;
    }
}

/*
 * Group II vector loads and stores:
 * PV and UV (As of RCP implementation, XV and ZV are reserved opcodes.)
 */
INLINE static void LPV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    const int e = element;

    if (e != 0x0)
    {
        message(sp->r4300->state, "LPV\nIllegal element.", 3);
        return;
    }
    addr = (sp->SR[base] + 8*offset) & 0x00000FFF;
    b = addr & 07;
    addr &= ~07;
    switch (b)
    {
        case 00:
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x007)] << 8;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x006)] << 8;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x005)] << 8;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x004)] << 8;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x003)] << 8;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x002)] << 8;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x001)] << 8;
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x000)] << 8;
            return;
        case 01: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x001)] << 8;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x002)] << 8;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x003)] << 8;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x004)] << 8;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x005)] << 8;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x006)] << 8;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x007)] << 8;
            addr += BES(0x008);
            addr &= 0x00000FFF;
            sp->VR[vt][07] = sp->DMEM[addr] << 8;
            return;
        case 02: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x002)] << 8;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x003)] << 8;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x004)] << 8;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x005)] << 8;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x006)] << 8;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x000)] << 8;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x001)] << 8;
            return;
        case 03: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x003)] << 8;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x004)] << 8;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x005)] << 8;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x006)] << 8;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x000)] << 8;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x001)] << 8;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x002)] << 8;
            return;
        case 04: /* "Resident Evil 2" in-game 3-D, F3DLX 2.08--"WWF No Mercy" */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x004)] << 8;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x005)] << 8;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x006)] << 8;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x000)] << 8;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x001)] << 8;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x002)] << 8;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x003)] << 8;
            return;
        case 05: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x005)] << 8;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x006)] << 8;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x000)] << 8;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x001)] << 8;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x002)] << 8;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x003)] << 8;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x004)] << 8;
            return;
        case 06: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x006)] << 8;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x000)] << 8;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x001)] << 8;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x002)] << 8;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x003)] << 8;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x004)] << 8;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x005)] << 8;
            return;
        case 07: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x000)] << 8;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x001)] << 8;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x002)] << 8;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x003)] << 8;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x004)] << 8;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x005)] << 8;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x006)] << 8;
            return;
    }
}
INLINE static void LUV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    int e = element;

    addr = (sp->SR[base] + 8*offset) & 0x00000FFF;
    if (e != 0x0)
    { /* "Mia Hamm Soccer 64" SP exception override (zilmar) */
        addr += -e & 0xF;
        for (b = 0; b < 8; b++)
        {
            sp->VR[vt][b] = sp->DMEM[BES(addr &= 0x00000FFF)] << 7;
            --e;
            addr -= 16 * (e == 0x0);
            ++addr;
        }
        return;
    }
    b = addr & 07;
    addr &= ~07;
    switch (b)
    {
        case 00:
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x007)] << 7;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x006)] << 7;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x005)] << 7;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x004)] << 7;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x003)] << 7;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x002)] << 7;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x001)] << 7;
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x000)] << 7;
            return;
        case 01: /* PKMN Puzzle League HVQM decoder */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x001)] << 7;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x002)] << 7;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x003)] << 7;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x004)] << 7;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x005)] << 7;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x006)] << 7;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x007)] << 7;
            addr += BES(0x008);
            addr &= 0x00000FFF;
            sp->VR[vt][07] = sp->DMEM[addr] << 7;
            return;
        case 02: /* PKMN Puzzle League HVQM decoder */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x002)] << 7;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x003)] << 7;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x004)] << 7;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x005)] << 7;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x006)] << 7;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x000)] << 7;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x001)] << 7;
            return;
        case 03: /* PKMN Puzzle League HVQM decoder */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x003)] << 7;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x004)] << 7;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x005)] << 7;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x006)] << 7;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x000)] << 7;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x001)] << 7;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x002)] << 7;
            return;
        case 04: /* PKMN Puzzle League HVQM decoder */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x004)] << 7;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x005)] << 7;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x006)] << 7;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x000)] << 7;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x001)] << 7;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x002)] << 7;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x003)] << 7;
            return;
        case 05: /* PKMN Puzzle League HVQM decoder */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x005)] << 7;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x006)] << 7;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x000)] << 7;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x001)] << 7;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x002)] << 7;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x003)] << 7;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x004)] << 7;
            return;
        case 06: /* PKMN Puzzle League HVQM decoder */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x006)] << 7;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x000)] << 7;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x001)] << 7;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x002)] << 7;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x003)] << 7;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x004)] << 7;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x005)] << 7;
            return;
        case 07: /* PKMN Puzzle League HVQM decoder */
            sp->VR[vt][00] = sp->DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->VR[vt][01] = sp->DMEM[addr + BES(0x000)] << 7;
            sp->VR[vt][02] = sp->DMEM[addr + BES(0x001)] << 7;
            sp->VR[vt][03] = sp->DMEM[addr + BES(0x002)] << 7;
            sp->VR[vt][04] = sp->DMEM[addr + BES(0x003)] << 7;
            sp->VR[vt][05] = sp->DMEM[addr + BES(0x004)] << 7;
            sp->VR[vt][06] = sp->DMEM[addr + BES(0x005)] << 7;
            sp->VR[vt][07] = sp->DMEM[addr + BES(0x006)] << 7;
            return;
    }
}
INLINE static void SPV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register int b;
    register uint32_t addr;
    const int e = element;

    if (e != 0x0)
    {
        message(sp->r4300->state, "SPV\nIllegal element.", 3);
        return;
    }
    addr = (sp->SR[base] + 8*offset) & 0x00000FFF;
    b = addr & 07;
    addr &= ~07;
    switch (b)
    {
        case 00:
            sp->DMEM[addr + BES(0x007)] = (unsigned char)(sp->VR[vt][07] >> 8);
            sp->DMEM[addr + BES(0x006)] = (unsigned char)(sp->VR[vt][06] >> 8);
            sp->DMEM[addr + BES(0x005)] = (unsigned char)(sp->VR[vt][05] >> 8);
            sp->DMEM[addr + BES(0x004)] = (unsigned char)(sp->VR[vt][04] >> 8);
            sp->DMEM[addr + BES(0x003)] = (unsigned char)(sp->VR[vt][03] >> 8);
            sp->DMEM[addr + BES(0x002)] = (unsigned char)(sp->VR[vt][02] >> 8);
            sp->DMEM[addr + BES(0x001)] = (unsigned char)(sp->VR[vt][01] >> 8);
            sp->DMEM[addr + BES(0x000)] = (unsigned char)(sp->VR[vt][00] >> 8);
            return;
        case 01: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->DMEM[addr + BES(0x001)] = (unsigned char)(sp->VR[vt][00] >> 8);
            sp->DMEM[addr + BES(0x002)] = (unsigned char)(sp->VR[vt][01] >> 8);
            sp->DMEM[addr + BES(0x003)] = (unsigned char)(sp->VR[vt][02] >> 8);
            sp->DMEM[addr + BES(0x004)] = (unsigned char)(sp->VR[vt][03] >> 8);
            sp->DMEM[addr + BES(0x005)] = (unsigned char)(sp->VR[vt][04] >> 8);
            sp->DMEM[addr + BES(0x006)] = (unsigned char)(sp->VR[vt][05] >> 8);
            sp->DMEM[addr + BES(0x007)] = (unsigned char)(sp->VR[vt][06] >> 8);
            addr += BES(0x008);
            addr &= 0x00000FFF;
            sp->DMEM[addr] = (unsigned char)(sp->VR[vt][07] >> 8);
            return;
        case 02: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->DMEM[addr + BES(0x002)] = (unsigned char)(sp->VR[vt][00] >> 8);
            sp->DMEM[addr + BES(0x003)] = (unsigned char)(sp->VR[vt][01] >> 8);
            sp->DMEM[addr + BES(0x004)] = (unsigned char)(sp->VR[vt][02] >> 8);
            sp->DMEM[addr + BES(0x005)] = (unsigned char)(sp->VR[vt][03] >> 8);
            sp->DMEM[addr + BES(0x006)] = (unsigned char)(sp->VR[vt][04] >> 8);
            sp->DMEM[addr + BES(0x007)] = (unsigned char)(sp->VR[vt][05] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->DMEM[addr + BES(0x000)] = (unsigned char)(sp->VR[vt][06] >> 8);
            sp->DMEM[addr + BES(0x001)] = (unsigned char)(sp->VR[vt][07] >> 8);
            return;
        case 03: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->DMEM[addr + BES(0x003)] = (unsigned char)(sp->VR[vt][00] >> 8);
            sp->DMEM[addr + BES(0x004)] = (unsigned char)(sp->VR[vt][01] >> 8);
            sp->DMEM[addr + BES(0x005)] = (unsigned char)(sp->VR[vt][02] >> 8);
            sp->DMEM[addr + BES(0x006)] = (unsigned char)(sp->VR[vt][03] >> 8);
            sp->DMEM[addr + BES(0x007)] = (unsigned char)(sp->VR[vt][04] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->DMEM[addr + BES(0x000)] = (unsigned char)(sp->VR[vt][05] >> 8);
            sp->DMEM[addr + BES(0x001)] = (unsigned char)(sp->VR[vt][06] >> 8);
            sp->DMEM[addr + BES(0x002)] = (unsigned char)(sp->VR[vt][07] >> 8);
            return;
        case 04: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->DMEM[addr + BES(0x004)] = (unsigned char)(sp->VR[vt][00] >> 8);
            sp->DMEM[addr + BES(0x005)] = (unsigned char)(sp->VR[vt][01] >> 8);
            sp->DMEM[addr + BES(0x006)] = (unsigned char)(sp->VR[vt][02] >> 8);
            sp->DMEM[addr + BES(0x007)] = (unsigned char)(sp->VR[vt][03] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->DMEM[addr + BES(0x000)] = (unsigned char)(sp->VR[vt][04] >> 8);
            sp->DMEM[addr + BES(0x001)] = (unsigned char)(sp->VR[vt][05] >> 8);
            sp->DMEM[addr + BES(0x002)] = (unsigned char)(sp->VR[vt][06] >> 8);
            sp->DMEM[addr + BES(0x003)] = (unsigned char)(sp->VR[vt][07] >> 8);
            return;
        case 05: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->DMEM[addr + BES(0x005)] = (unsigned char)(sp->VR[vt][00] >> 8);
            sp->DMEM[addr + BES(0x006)] = (unsigned char)(sp->VR[vt][01] >> 8);
            sp->DMEM[addr + BES(0x007)] = (unsigned char)(sp->VR[vt][02] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->DMEM[addr + BES(0x000)] = (unsigned char)(sp->VR[vt][03] >> 8);
            sp->DMEM[addr + BES(0x001)] = (unsigned char)(sp->VR[vt][04] >> 8);
            sp->DMEM[addr + BES(0x002)] = (unsigned char)(sp->VR[vt][05] >> 8);
            sp->DMEM[addr + BES(0x003)] = (unsigned char)(sp->VR[vt][06] >> 8);
            sp->DMEM[addr + BES(0x004)] = (unsigned char)(sp->VR[vt][07] >> 8);
            return;
        case 06: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->DMEM[addr + BES(0x006)] = (unsigned char)(sp->VR[vt][00] >> 8);
            sp->DMEM[addr + BES(0x007)] = (unsigned char)(sp->VR[vt][01] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->DMEM[addr + BES(0x000)] = (unsigned char)(sp->VR[vt][02] >> 8);
            sp->DMEM[addr + BES(0x001)] = (unsigned char)(sp->VR[vt][03] >> 8);
            sp->DMEM[addr + BES(0x002)] = (unsigned char)(sp->VR[vt][04] >> 8);
            sp->DMEM[addr + BES(0x003)] = (unsigned char)(sp->VR[vt][05] >> 8);
            sp->DMEM[addr + BES(0x004)] = (unsigned char)(sp->VR[vt][06] >> 8);
            sp->DMEM[addr + BES(0x005)] = (unsigned char)(sp->VR[vt][07] >> 8);
            return;
        case 07: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            sp->DMEM[addr + BES(0x007)] = (unsigned char)(sp->VR[vt][00] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->DMEM[addr + BES(0x000)] = (unsigned char)(sp->VR[vt][01] >> 8);
            sp->DMEM[addr + BES(0x001)] = (unsigned char)(sp->VR[vt][02] >> 8);
            sp->DMEM[addr + BES(0x002)] = (unsigned char)(sp->VR[vt][03] >> 8);
            sp->DMEM[addr + BES(0x003)] = (unsigned char)(sp->VR[vt][04] >> 8);
            sp->DMEM[addr + BES(0x004)] = (unsigned char)(sp->VR[vt][05] >> 8);
            sp->DMEM[addr + BES(0x005)] = (unsigned char)(sp->VR[vt][06] >> 8);
            sp->DMEM[addr + BES(0x006)] = (unsigned char)(sp->VR[vt][07] >> 8);
            return;
    }
}
INLINE static void SUV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register int b;
    register uint32_t addr;
    const int e = element;

    if (e != 0x0)
    {
        message(sp->r4300->state, "SUV\nIllegal element.", 3);
        return;
    }
    addr = (sp->SR[base] + 8*offset) & 0x00000FFF;
    b = addr & 07;
    addr &= ~07;
    switch (b)
    {
        case 00:
            sp->DMEM[addr + BES(0x007)] = (unsigned char)(sp->VR[vt][07] >> 7);
            sp->DMEM[addr + BES(0x006)] = (unsigned char)(sp->VR[vt][06] >> 7);
            sp->DMEM[addr + BES(0x005)] = (unsigned char)(sp->VR[vt][05] >> 7);
            sp->DMEM[addr + BES(0x004)] = (unsigned char)(sp->VR[vt][04] >> 7);
            sp->DMEM[addr + BES(0x003)] = (unsigned char)(sp->VR[vt][03] >> 7);
            sp->DMEM[addr + BES(0x002)] = (unsigned char)(sp->VR[vt][02] >> 7);
            sp->DMEM[addr + BES(0x001)] = (unsigned char)(sp->VR[vt][01] >> 7);
            sp->DMEM[addr + BES(0x000)] = (unsigned char)(sp->VR[vt][00] >> 7);
            return;
        case 04: /* "Indiana Jones and the Infernal Machine" in-game */
            sp->DMEM[addr + BES(0x004)] = (unsigned char)(sp->VR[vt][00] >> 7);
            sp->DMEM[addr + BES(0x005)] = (unsigned char)(sp->VR[vt][01] >> 7);
            sp->DMEM[addr + BES(0x006)] = (unsigned char)(sp->VR[vt][02] >> 7);
            sp->DMEM[addr + BES(0x007)] = (unsigned char)(sp->VR[vt][03] >> 7);
            addr += 0x008;
            addr &= 0x00000FFF;
            sp->DMEM[addr + BES(0x000)] = (unsigned char)(sp->VR[vt][04] >> 7);
            sp->DMEM[addr + BES(0x001)] = (unsigned char)(sp->VR[vt][05] >> 7);
            sp->DMEM[addr + BES(0x002)] = (unsigned char)(sp->VR[vt][06] >> 7);
            sp->DMEM[addr + BES(0x003)] = (unsigned char)(sp->VR[vt][07] >> 7);
            return;
        default: /* Completely legal, just never seen it be done. */
            message(sp->r4300->state, "SUV\nWeird addr.", 3);
            return;
    }
}

/*
 * Group III vector loads and stores:
 * HV, FV, and AV (As of RCP implementation, AV opcodes are reserved.)
 */
static void LHV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    if (e != 0x0)
    {
        message(sp->r4300->state, "LHV\nIllegal element.", 3);
        return;
    }
    addr = (sp->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x0000000E)
    {
        message(sp->r4300->state, "LHV\nIllegal addr.", 3);
        return;
    }
    addr ^= MES(00);
    sp->VR[vt][07] = sp->DMEM[addr + HES(0x00E)] << 7;
    sp->VR[vt][06] = sp->DMEM[addr + HES(0x00C)] << 7;
    sp->VR[vt][05] = sp->DMEM[addr + HES(0x00A)] << 7;
    sp->VR[vt][04] = sp->DMEM[addr + HES(0x008)] << 7;
    sp->VR[vt][03] = sp->DMEM[addr + HES(0x006)] << 7;
    sp->VR[vt][02] = sp->DMEM[addr + HES(0x004)] << 7;
    sp->VR[vt][01] = sp->DMEM[addr + HES(0x002)] << 7;
    sp->VR[vt][00] = sp->DMEM[addr + HES(0x000)] << 7;
    return;
}
NOINLINE static void LFV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    (void)sp;
    (void)vt;
    (void)element;
    (void)offset;
    (void)base;
    /* Dummy implementation only:  Do any games execute this? */
    /*char debugger[32];

    sprintf(debugger, "%s     $v%i[0x%X], 0x%03X($%i)", "LFV",
        vt, element, offset & 0xFFF, base);
    message(state, debugger, 3);*/
    return;
}
static void SHV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    if (e != 0x0)
    {
        message(sp->r4300->state, "SHV\nIllegal element.", 3);
        return;
    }
    addr = (sp->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x0000000E)
    {
        message(sp->r4300->state, "SHV\nIllegal addr.", 3);
        return;
    }
    addr ^= MES(00);
    sp->DMEM[addr + HES(0x00E)] = (unsigned char)(sp->VR[vt][07] >> 7);
    sp->DMEM[addr + HES(0x00C)] = (unsigned char)(sp->VR[vt][06] >> 7);
    sp->DMEM[addr + HES(0x00A)] = (unsigned char)(sp->VR[vt][05] >> 7);
    sp->DMEM[addr + HES(0x008)] = (unsigned char)(sp->VR[vt][04] >> 7);
    sp->DMEM[addr + HES(0x006)] = (unsigned char)(sp->VR[vt][03] >> 7);
    sp->DMEM[addr + HES(0x004)] = (unsigned char)(sp->VR[vt][02] >> 7);
    sp->DMEM[addr + HES(0x002)] = (unsigned char)(sp->VR[vt][01] >> 7);
    sp->DMEM[addr + HES(0x000)] = (unsigned char)(sp->VR[vt][00] >> 7);
    return;
}
static void SFV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    const int e = element;

    addr = (sp->SR[base] + 16*offset) & 0x00000FFF;
    addr &= 0x00000FF3;
    addr ^= BES(00);
    switch (e)
    {
        case 0x0:
            sp->DMEM[addr + 0x000] = (unsigned char)(sp->VR[vt][00] >> 7);
            sp->DMEM[addr + 0x004] = (unsigned char)(sp->VR[vt][01] >> 7);
            sp->DMEM[addr + 0x008] = (unsigned char)(sp->VR[vt][02] >> 7);
            sp->DMEM[addr + 0x00C] = (unsigned char)(sp->VR[vt][03] >> 7);
            return;
        case 0x8:
            sp->DMEM[addr + 0x000] = (unsigned char)(sp->VR[vt][04] >> 7);
            sp->DMEM[addr + 0x004] = (unsigned char)(sp->VR[vt][05] >> 7);
            sp->DMEM[addr + 0x008] = (unsigned char)(sp->VR[vt][06] >> 7);
            sp->DMEM[addr + 0x00C] = (unsigned char)(sp->VR[vt][07] >> 7);
            return;
        default:
            message(sp->r4300->state, "SFV\nIllegal element.", 3);
            return;
    }
}

/*
 * Group IV vector loads and stores:
 * QV and RV
 */
INLINE static void LQV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    const int e = element; /* Boss Game Studios illegal elements */

    if (e & 0x1)
    {
        message(sp->r4300->state, "LQV\nOdd element.", 3);
        return;
    }
    addr = (sp->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x00000001)
    {
        message(sp->r4300->state, "LQV\nOdd addr.", 3);
        return;
    }
    b = addr & 0x0000000F;
    addr &= ~0x0000000F;
    switch (b/2) /* mistake in SGI patent regarding LQV */
    {
        case 0x0/2:
            VR_S(vt,e+0x0) = *(short *)(sp->DMEM + addr + HES(0x000));
            VR_S(vt,e+0x2) = *(short *)(sp->DMEM + addr + HES(0x002));
            VR_S(vt,e+0x4) = *(short *)(sp->DMEM + addr + HES(0x004));
            VR_S(vt,e+0x6) = *(short *)(sp->DMEM + addr + HES(0x006));
            VR_S(vt,e+0x8) = *(short *)(sp->DMEM + addr + HES(0x008));
            VR_S(vt,e+0xA) = *(short *)(sp->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0xC) = *(short *)(sp->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0xE) = *(short *)(sp->DMEM + addr + HES(0x00E));
            return;
        case 0x2/2:
            VR_S(vt,e+0x0) = *(short *)(sp->DMEM + addr + HES(0x002));
            VR_S(vt,e+0x2) = *(short *)(sp->DMEM + addr + HES(0x004));
            VR_S(vt,e+0x4) = *(short *)(sp->DMEM + addr + HES(0x006));
            VR_S(vt,e+0x6) = *(short *)(sp->DMEM + addr + HES(0x008));
            VR_S(vt,e+0x8) = *(short *)(sp->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0xA) = *(short *)(sp->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0xC) = *(short *)(sp->DMEM + addr + HES(0x00E));
            return;
        case 0x4/2:
            VR_S(vt,e+0x0) = *(short *)(sp->DMEM + addr + HES(0x004));
            VR_S(vt,e+0x2) = *(short *)(sp->DMEM + addr + HES(0x006));
            VR_S(vt,e+0x4) = *(short *)(sp->DMEM + addr + HES(0x008));
            VR_S(vt,e+0x6) = *(short *)(sp->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x8) = *(short *)(sp->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0xA) = *(short *)(sp->DMEM + addr + HES(0x00E));
            return;
        case 0x6/2:
            VR_S(vt,e+0x0) = *(short *)(sp->DMEM + addr + HES(0x006));
            VR_S(vt,e+0x2) = *(short *)(sp->DMEM + addr + HES(0x008));
            VR_S(vt,e+0x4) = *(short *)(sp->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x6) = *(short *)(sp->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x8) = *(short *)(sp->DMEM + addr + HES(0x00E));
            return;
        case 0x8/2: /* "Resident Evil 2" cinematics and Boss Game Studios */
            VR_S(vt,e+0x0) = *(short *)(sp->DMEM + addr + HES(0x008));
            VR_S(vt,e+0x2) = *(short *)(sp->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x4) = *(short *)(sp->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x6) = *(short *)(sp->DMEM + addr + HES(0x00E));
            return;
        case 0xA/2: /* "Conker's Bad Fur Day" audio microcode by Rareware */
            VR_S(vt,e+0x0) = *(short *)(sp->DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x2) = *(short *)(sp->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x4) = *(short *)(sp->DMEM + addr + HES(0x00E));
            return;
        case 0xC/2: /* "Conker's Bad Fur Day" audio microcode by Rareware */
            VR_S(vt,e+0x0) = *(short *)(sp->DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x2) = *(short *)(sp->DMEM + addr + HES(0x00E));
            return;
        case 0xE/2: /* "Conker's Bad Fur Day" audio microcode by Rareware */
            VR_S(vt,e+0x0) = *(short *)(sp->DMEM + addr + HES(0x00E));
            return;
    }
}
static void LRV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    const int e = element;

    if (e != 0x0)
    {
        message(sp->r4300->state, "LRV\nIllegal element.", 3);
        return;
    }
    addr = (sp->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x00000001)
    {
        message(sp->r4300->state, "LRV\nOdd addr.", 3);
        return;
    }
    b = addr & 0x0000000F;
    addr &= ~0x0000000F;
    switch (b/2)
    {
        case 0xE/2:
            sp->VR[vt][01] = *(short *)(sp->DMEM + addr + HES(0x000));
            sp->VR[vt][02] = *(short *)(sp->DMEM + addr + HES(0x002));
            sp->VR[vt][03] = *(short *)(sp->DMEM + addr + HES(0x004));
            sp->VR[vt][04] = *(short *)(sp->DMEM + addr + HES(0x006));
            sp->VR[vt][05] = *(short *)(sp->DMEM + addr + HES(0x008));
            sp->VR[vt][06] = *(short *)(sp->DMEM + addr + HES(0x00A));
            sp->VR[vt][07] = *(short *)(sp->DMEM + addr + HES(0x00C));
            return;
        case 0xC/2:
            sp->VR[vt][02] = *(short *)(sp->DMEM + addr + HES(0x000));
            sp->VR[vt][03] = *(short *)(sp->DMEM + addr + HES(0x002));
            sp->VR[vt][04] = *(short *)(sp->DMEM + addr + HES(0x004));
            sp->VR[vt][05] = *(short *)(sp->DMEM + addr + HES(0x006));
            sp->VR[vt][06] = *(short *)(sp->DMEM + addr + HES(0x008));
            sp->VR[vt][07] = *(short *)(sp->DMEM + addr + HES(0x00A));
            return;
        case 0xA/2:
            sp->VR[vt][03] = *(short *)(sp->DMEM + addr + HES(0x000));
            sp->VR[vt][04] = *(short *)(sp->DMEM + addr + HES(0x002));
            sp->VR[vt][05] = *(short *)(sp->DMEM + addr + HES(0x004));
            sp->VR[vt][06] = *(short *)(sp->DMEM + addr + HES(0x006));
            sp->VR[vt][07] = *(short *)(sp->DMEM + addr + HES(0x008));
            return;
        case 0x8/2:
            sp->VR[vt][04] = *(short *)(sp->DMEM + addr + HES(0x000));
            sp->VR[vt][05] = *(short *)(sp->DMEM + addr + HES(0x002));
            sp->VR[vt][06] = *(short *)(sp->DMEM + addr + HES(0x004));
            sp->VR[vt][07] = *(short *)(sp->DMEM + addr + HES(0x006));
            return;
        case 0x6/2:
            sp->VR[vt][05] = *(short *)(sp->DMEM + addr + HES(0x000));
            sp->VR[vt][06] = *(short *)(sp->DMEM + addr + HES(0x002));
            sp->VR[vt][07] = *(short *)(sp->DMEM + addr + HES(0x004));
            return;
        case 0x4/2:
            sp->VR[vt][06] = *(short *)(sp->DMEM + addr + HES(0x000));
            sp->VR[vt][07] = *(short *)(sp->DMEM + addr + HES(0x002));
            return;
        case 0x2/2:
            sp->VR[vt][07] = *(short *)(sp->DMEM + addr + HES(0x000));
            return;
        case 0x0/2:
            return;
    }
}
INLINE static void SQV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    const int e = element;

    addr = (sp->SR[base] + 16*offset) & 0x00000FFF;
    if (e != 0x0)
    { /* happens with "Mia Hamm Soccer 64" */
        register int i;

        for (i = 0; i < (int)(16 - addr%16); i++)
            sp->DMEM[BES((addr + i) & 0xFFF)] = VR_B(vt, (e + i) & 0xF);
        return;
    }
    b = addr & 0x0000000F;
    addr &= ~0x0000000F;
    switch (b)
    {
        case 00:
            *(short *)(sp->DMEM + addr + HES(0x000)) = sp->VR[vt][00];
            *(short *)(sp->DMEM + addr + HES(0x002)) = sp->VR[vt][01];
            *(short *)(sp->DMEM + addr + HES(0x004)) = sp->VR[vt][02];
            *(short *)(sp->DMEM + addr + HES(0x006)) = sp->VR[vt][03];
            *(short *)(sp->DMEM + addr + HES(0x008)) = sp->VR[vt][04];
            *(short *)(sp->DMEM + addr + HES(0x00A)) = sp->VR[vt][05];
            *(short *)(sp->DMEM + addr + HES(0x00C)) = sp->VR[vt][06];
            *(short *)(sp->DMEM + addr + HES(0x00E)) = sp->VR[vt][07];
            return;
        case 02:
            *(short *)(sp->DMEM + addr + HES(0x002)) = sp->VR[vt][00];
            *(short *)(sp->DMEM + addr + HES(0x004)) = sp->VR[vt][01];
            *(short *)(sp->DMEM + addr + HES(0x006)) = sp->VR[vt][02];
            *(short *)(sp->DMEM + addr + HES(0x008)) = sp->VR[vt][03];
            *(short *)(sp->DMEM + addr + HES(0x00A)) = sp->VR[vt][04];
            *(short *)(sp->DMEM + addr + HES(0x00C)) = sp->VR[vt][05];
            *(short *)(sp->DMEM + addr + HES(0x00E)) = sp->VR[vt][06];
            return;
        case 04:
            *(short *)(sp->DMEM + addr + HES(0x004)) = sp->VR[vt][00];
            *(short *)(sp->DMEM + addr + HES(0x006)) = sp->VR[vt][01];
            *(short *)(sp->DMEM + addr + HES(0x008)) = sp->VR[vt][02];
            *(short *)(sp->DMEM + addr + HES(0x00A)) = sp->VR[vt][03];
            *(short *)(sp->DMEM + addr + HES(0x00C)) = sp->VR[vt][04];
            *(short *)(sp->DMEM + addr + HES(0x00E)) = sp->VR[vt][05];
            return;
        case 06:
            *(short *)(sp->DMEM + addr + HES(0x006)) = sp->VR[vt][00];
            *(short *)(sp->DMEM + addr + HES(0x008)) = sp->VR[vt][01];
            *(short *)(sp->DMEM + addr + HES(0x00A)) = sp->VR[vt][02];
            *(short *)(sp->DMEM + addr + HES(0x00C)) = sp->VR[vt][03];
            *(short *)(sp->DMEM + addr + HES(0x00E)) = sp->VR[vt][04];
            return;
        default:
            message(sp->r4300->state, "SQV\nWeird addr.", 3);
            return;
    }
}
static void SRV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register uint32_t addr;
    register int b;
    const int e = element;

    if (e != 0x0)
    {
        message(sp->r4300->state, "SRV\nIllegal element.", 3);
        return;
    }
    addr = (sp->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x00000001)
    {
        message(sp->r4300->state, "SRV\nOdd addr.", 3);
        return;
    }
    b = addr & 0x0000000F;
    addr &= ~0x0000000F;
    switch (b/2)
    {
        case 0xE/2:
            *(short *)(sp->DMEM + addr + HES(0x000)) = sp->VR[vt][01];
            *(short *)(sp->DMEM + addr + HES(0x002)) = sp->VR[vt][02];
            *(short *)(sp->DMEM + addr + HES(0x004)) = sp->VR[vt][03];
            *(short *)(sp->DMEM + addr + HES(0x006)) = sp->VR[vt][04];
            *(short *)(sp->DMEM + addr + HES(0x008)) = sp->VR[vt][05];
            *(short *)(sp->DMEM + addr + HES(0x00A)) = sp->VR[vt][06];
            *(short *)(sp->DMEM + addr + HES(0x00C)) = sp->VR[vt][07];
            return;
        case 0xC/2:
            *(short *)(sp->DMEM + addr + HES(0x000)) = sp->VR[vt][02];
            *(short *)(sp->DMEM + addr + HES(0x002)) = sp->VR[vt][03];
            *(short *)(sp->DMEM + addr + HES(0x004)) = sp->VR[vt][04];
            *(short *)(sp->DMEM + addr + HES(0x006)) = sp->VR[vt][05];
            *(short *)(sp->DMEM + addr + HES(0x008)) = sp->VR[vt][06];
            *(short *)(sp->DMEM + addr + HES(0x00A)) = sp->VR[vt][07];
            return;
        case 0xA/2:
            *(short *)(sp->DMEM + addr + HES(0x000)) = sp->VR[vt][03];
            *(short *)(sp->DMEM + addr + HES(0x002)) = sp->VR[vt][04];
            *(short *)(sp->DMEM + addr + HES(0x004)) = sp->VR[vt][05];
            *(short *)(sp->DMEM + addr + HES(0x006)) = sp->VR[vt][06];
            *(short *)(sp->DMEM + addr + HES(0x008)) = sp->VR[vt][07];
            return;
        case 0x8/2:
            *(short *)(sp->DMEM + addr + HES(0x000)) = sp->VR[vt][04];
            *(short *)(sp->DMEM + addr + HES(0x002)) = sp->VR[vt][05];
            *(short *)(sp->DMEM + addr + HES(0x004)) = sp->VR[vt][06];
            *(short *)(sp->DMEM + addr + HES(0x006)) = sp->VR[vt][07];
            return;
        case 0x6/2:
            *(short *)(sp->DMEM + addr + HES(0x000)) = sp->VR[vt][05];
            *(short *)(sp->DMEM + addr + HES(0x002)) = sp->VR[vt][06];
            *(short *)(sp->DMEM + addr + HES(0x004)) = sp->VR[vt][07];
            return;
        case 0x4/2:
            *(short *)(sp->DMEM + addr + HES(0x000)) = sp->VR[vt][06];
            *(short *)(sp->DMEM + addr + HES(0x002)) = sp->VR[vt][07];
            return;
        case 0x2/2:
            *(short *)(sp->DMEM + addr + HES(0x000)) = sp->VR[vt][07];
            return;
        case 0x0/2:
            return;
    }
}

/*
 * Group V vector loads and stores
 * TV and SWV (As of RCP implementation, LTWV opcode was undesired.)
 */
INLINE static void LTV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register int i;
    register uint32_t addr;
    const int e = element;

    if (e & 1)
    {
        message(sp->r4300->state, "LTV\nIllegal element.", 3);
        return;
    }
    if (vt & 07)
    {
        message(sp->r4300->state, "LTV\nUncertain case!", 3);
        return; /* For LTV I am not sure; for STV I have an idea. */
    }
    addr = (sp->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x0000000F)
    {
        message(sp->r4300->state, "LTV\nIllegal addr.", 3);
        return;
    }
    for (i = 0; i < 8; i++) /* SGI screwed LTV up on N64.  See STV instead. */
        sp->VR[vt+i][(-e/2 + i) & 07] = *(short *)(sp->DMEM + addr + HES(2*i));
    return;
}
NOINLINE static void SWV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    (void)sp;
    (void)vt;
    (void)element;
    (void)offset;
    (void)base;
    /* Dummy implementation only:  Do any games execute this? */
    /*char debugger[32];

    sprintf(debugger, "%s     $v%i[0x%X], 0x%03X($%i)", "SWV",
        vt, element, offset & 0xFFF, base);
    message(state, debugger, 3);*/
    return;
}
INLINE static void STV(struct rsp_core* sp, int vt, int element, int offset, int base)
{
    register int i;
    register uint32_t addr;
    const int e = element;

    if (e & 1)
    {
        message(sp->r4300->state, "STV\nIllegal element.", 3);
        return;
    }
    if (vt & 07)
    {
        message(sp->r4300->state, "STV\nUncertain case!", 2);
        return; /* vt &= 030; */
    }
    addr = (sp->SR[base] + 16*offset) & 0x00000FFF;
    if (addr & 0x0000000F)
    {
        message(sp->r4300->state, "STV\nIllegal addr.", 3);
        return;
    }
    for (i = 0; i < 8; i++)
        *(short *)(sp->DMEM + addr + HES(2*i)) = sp->VR[vt + (e/2 + i)%8][i];
    return;
}

/*** Modern pseudo-operations (not real instructions, but nice shortcuts) ***/
void ULW(struct rsp_core* sp, int rd, uint32_t addr)
{ /* "Unaligned Load Word" */
    union {
        unsigned char B[4];
        signed char SB[4];
        unsigned short H[2];
        signed short SH[2];
        unsigned W:  32;
    } SR_temp;
    if (addr & 0x00000001)
    {
        SR_temp.B[03] = sp->DMEM[BES(addr)];
        addr = (addr + 0x001) & 0xFFF;
        SR_temp.B[02] = sp->DMEM[BES(addr)];
        addr = (addr + 0x001) & 0xFFF;
        SR_temp.B[01] = sp->DMEM[BES(addr)];
        addr = (addr + 0x001) & 0xFFF;
        SR_temp.B[00] = sp->DMEM[BES(addr)];
    }
    else /* addr & 0x00000002 */
    {
        SR_temp.H[01] = *(short *)(sp->DMEM + addr - HES(0x000));
        addr = (addr + 0x002) & 0xFFF;
        SR_temp.H[00] = *(short *)(sp->DMEM + addr + HES(0x000));
    }
    sp->SR[rd] = SR_temp.W;
 /* sp->SR[0] = 0x00000000; */
    return;
}
void USW(struct rsp_core* sp, int rs, uint32_t addr)
{ /* "Unaligned Store Word" */
    union {
        unsigned char B[4];
        signed char SB[4];
        unsigned short H[2];
        signed short SH[2];
        unsigned W:  32;
    } SR_temp;
    SR_temp.W = sp->SR[rs];
    if (addr & 0x00000001)
    {
        sp->DMEM[BES(addr)] = SR_temp.B[03];
        addr = (addr + 0x001) & 0xFFF;
        sp->DMEM[BES(addr)] = SR_temp.B[02];
        addr = (addr + 0x001) & 0xFFF;
        sp->DMEM[BES(addr)] = SR_temp.B[01];
        addr = (addr + 0x001) & 0xFFF;
        sp->DMEM[BES(addr)] = SR_temp.B[00];
    }
    else /* addr & 0x00000002 */
    {
        *(short *)(sp->DMEM + addr - HES(0x000)) = SR_temp.H[01];
        addr = (addr + 0x002) & 0xFFF;
        *(short *)(sp->DMEM + addr + HES(0x000)) = SR_temp.H[00];
    }
    return;
}

#endif
