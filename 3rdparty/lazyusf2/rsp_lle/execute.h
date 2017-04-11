/******************************************************************************\
* Authors:  Iconoclast                                                         *
* Release:  2013.12.11                                                         *
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
#include "rsp.h"

#include "su.h"
#include "vu/vu.h"

#include "r4300/interupt.h"

#define FIT_IMEM(PC)    (PC & 0xFFF & 0xFFC)

NOINLINE void run_task(struct rsp_core* sp)
{
    register int PC;
    int wrap_count = 0;
#ifdef SP_EXECUTE_LOG
    int last_PC;
#endif

    if (CFG_WAIT_FOR_CPU_HOST != 0)
    {
        register int i;

        for (i = 0; i < 32; i++)
            sp->MFC0_count[i] = 0;
    }
    PC = FIT_IMEM(sp->regs2[SP_PC_REG]);
    while ((sp->regs[SP_STATUS_REG] & 0x00000001) == 0x00000000)
    {
        register uint32_t inst;

        inst = *(uint32_t *)(sp->IMEM + FIT_IMEM(PC));
#ifdef SP_EXECUTE_LOG
        last_PC = PC;
#endif
#ifdef EMULATE_STATIC_PC
        PC = (PC + 0x004);
        if ( FIT_IMEM(PC) == 0 && ++wrap_count == 32 )
        {
            message( sp->r4300->state, "RSP execution presumably caught in an infinite loop", 3 );
            break;
        }
EX:
#endif
#ifdef SP_EXECUTE_LOG
        step_SP_commands(last_PC, inst);
#endif
        if (inst >> 25 == 0x25) /* is a VU instruction */
        {
            const int opcode = inst % 64; /* inst.R.func */
            const int vd = (inst & 0x000007FF) >> 6; /* inst.R.sa */
            const int vs = (unsigned short)(inst) >> 11; /* inst.R.rd */
            const int vt = (inst >> 16) & 31; /* inst.R.rt */
            const int e  = (inst >> 21) & 0xF; /* rs & 0xF */

            COP2_C2[opcode](sp, vd, vs, vt, e);
        }
        else
        {
            const int op = inst >> 26;
            const int rs = inst >> 21; /* &= 31 */
            const int rt = (inst >> 16) & 31;
            const int rd = (unsigned short)(inst) >> 11;
            const int element = (inst & 0x000007FF) >> 7;
            const int base = (inst >> 21) & 31;

#if (0)
            sp->SR[0] = 0x00000000; /* already handled on per-instruction basis */
#endif
            switch (op)
            {
                signed int offset;
                register uint32_t addr;

                case 000: /* SPECIAL */
                    switch (inst % 64)
                    {
                        case 000: /* SLL */
                            sp->SR[rd] = sp->SR[rt] << MASK_SA(inst >> 6);
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        case 002: /* SRL */
                            sp->SR[rd] = (unsigned)(sp->SR[rt]) >> MASK_SA(inst >> 6);
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        case 003: /* SRA */
                            sp->SR[rd] = (signed)(sp->SR[rt]) >> MASK_SA(inst >> 6);
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        case 004: /* SLLV */
                            sp->SR[rd] = sp->SR[rt] << MASK_SA(sp->SR[rs]);
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        case 006: /* SRLV */
                            sp->SR[rd] = (unsigned)(sp->SR[rt]) >> MASK_SA(sp->SR[rs]);
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        case 007: /* SRAV */
                            sp->SR[rd] = (signed)(sp->SR[rt]) >> MASK_SA(sp->SR[rs]);
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        case 011: /* JALR */
                            sp->SR[rd] = (PC + LINK_OFF) & 0x00000FFC;
                            sp->SR[0] = 0x00000000;
                        case 010: /* JR */
                            set_PC(sp, sp->SR[rs]);
                            JUMP
                        case 015: /* BREAK */
                            sp->regs[SP_STATUS_REG] |= 0x00000003; /* BROKE | HALT */
                            if (sp->regs[SP_STATUS_REG] & 0x00000040)
                            { /* SP_STATUS_INTR_BREAK */
                                sp->r4300->mi.regs[MI_INTR_REG] |= 0x00000001;
                                //check_interupt(state);
                            }
                            CONTINUE
                        case 040: /* ADD */
                        case 041: /* ADDU */
                            sp->SR[rd] = sp->SR[rs] + sp->SR[rt];
                            sp->SR[0] = 0x00000000; /* needed for Rareware ucodes */
                            CONTINUE
                        case 042: /* SUB */
                        case 043: /* SUBU */
                            sp->SR[rd] = sp->SR[rs] - sp->SR[rt];
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        case 044: /* AND */
                            sp->SR[rd] = sp->SR[rs] & sp->SR[rt];
                            sp->SR[0] = 0x00000000; /* needed for Rareware ucodes */
                            CONTINUE
                        case 045: /* OR */
                            sp->SR[rd] = sp->SR[rs] | sp->SR[rt];
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        case 046: /* XOR */
                            sp->SR[rd] = sp->SR[rs] ^ sp->SR[rt];
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        case 047: /* NOR */
                            sp->SR[rd] = ~(sp->SR[rs] | sp->SR[rt]);
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        case 052: /* SLT */
                            sp->SR[rd] = ((signed)(sp->SR[rs]) < (signed)(sp->SR[rt]));
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        case 053: /* SLTU */
                            sp->SR[rd] = ((unsigned)(sp->SR[rs]) < (unsigned)(sp->SR[rt]));
                            sp->SR[0] = 0x00000000;
                            CONTINUE
                        default:
                            res_S(sp);
                            CONTINUE
                    }
                    CONTINUE
                case 001: /* REGIMM */
                    switch (rt)
                    {
                        case 020: /* BLTZAL */
                            sp->SR[31] = (PC + LINK_OFF) & 0x00000FFC;
                        case 000: /* BLTZ */
                            if (!(sp->SR[base] < 0))
                                CONTINUE
                            set_PC(sp, PC + 4*inst + SLOT_OFF);
                            JUMP
                        case 021: /* BGEZAL */
                            sp->SR[31] = (PC + LINK_OFF) & 0x00000FFC;
                        case 001: /* BGEZ */
                            if (!(sp->SR[base] >= 0))
                                CONTINUE
                            set_PC(sp, PC + 4*inst + SLOT_OFF);
                            JUMP
                        default:
                            res_S(sp);
                            CONTINUE
                    }
                    CONTINUE
                case 003: /* JAL */
                    sp->SR[31] = (PC + LINK_OFF) & 0x00000FFC;
                case 002: /* J */
                    set_PC(sp, 4*inst);
                    JUMP
                case 004: /* BEQ */
                    if (!(sp->SR[base] == sp->SR[rt]))
                        CONTINUE
                    set_PC(sp, PC + 4*inst + SLOT_OFF);
                    JUMP
                case 005: /* BNE */
                    if (!(sp->SR[base] != sp->SR[rt]))
                        CONTINUE
                    set_PC(sp, PC + 4*inst + SLOT_OFF);
                    JUMP
                case 006: /* BLEZ */
                    if (!((signed)sp->SR[base] <= 0x00000000))
                        CONTINUE
                    set_PC(sp, PC + 4*inst + SLOT_OFF);
                    JUMP
                case 007: /* BGTZ */
                    if (!((signed)sp->SR[base] >  0x00000000))
                        CONTINUE
                    set_PC(sp, PC + 4*inst + SLOT_OFF);
                    JUMP
                case 010: /* ADDI */
                case 011: /* ADDIU */
                    sp->SR[rt] = sp->SR[base] + (signed short)(inst);
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 012: /* SLTI */
                    sp->SR[rt] = ((signed)(sp->SR[base]) < (signed short)(inst));
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 013: /* SLTIU */
                    sp->SR[rt] = ((unsigned)(sp->SR[base]) < (unsigned short)(inst));
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 014: /* ANDI */
                    sp->SR[rt] = sp->SR[base] & (unsigned short)(inst);
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 015: /* ORI */
                    sp->SR[rt] = sp->SR[base] | (unsigned short)(inst);
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 016: /* XORI */
                    sp->SR[rt] = sp->SR[base] ^ (unsigned short)(inst);
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 017: /* LUI */
                    sp->SR[rt] = inst << 16;
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 020: /* COP0 */
                    switch (base)
                    {
                        case 000: /* MFC0 */
                            MFC0(sp, rt, rd & 0xF);
                            CONTINUE
                        case 004: /* MTC0 */
                            MTC0[rd & 0xF](sp, rt);
                            CONTINUE
                        default:
                            res_S(sp);
                            CONTINUE
                    }
                    CONTINUE
                case 022: /* COP2 */
                    switch (base)
                    {
                        case 000: /* MFC2 */
                            MFC2(sp, rt, rd, element);
                            CONTINUE
                        case 002: /* CFC2 */
                            CFC2(sp, rt, rd);
                            CONTINUE
                        case 004: /* MTC2 */
                            MTC2(sp, rt, rd, element);
                            CONTINUE
                        case 006: /* CTC2 */
                            CTC2(sp, rt, rd);
                            CONTINUE
                        default:
                            res_S(sp);
                            CONTINUE
                    }
                    CONTINUE
                case 040: /* LB */
                    offset = (signed short)(inst);
                    addr = (sp->SR[base] + offset) & 0x00000FFF;
                    sp->SR[rt] = sp->DMEM[BES(addr)];
                    sp->SR[rt] = (signed char)(sp->SR[rt]);
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 041: /* LH */
                    offset = (signed short)(inst);
                    addr = (sp->SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 == 0x003)
                    {
                        SR_B(rt, 2) = sp->DMEM[addr - BES(0x000)];
                        addr = (addr + 0x00000001) & 0x00000FFF;
                        SR_B(rt, 3) = sp->DMEM[addr + BES(0x000)];
                        sp->SR[rt] = (signed short)(sp->SR[rt]);
                    }
                    else
                    {
                        addr -= HES(0x000)*(addr%0x004 - 1);
                        sp->SR[rt] = *(signed short *)(sp->DMEM + addr);
                    }
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 043: /* LW */
                    offset = (signed short)(inst);
                    addr = (sp->SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 != 0x000)
                        ULW(sp, rt, addr);
                    else
                        sp->SR[rt] = *(int32_t *)(sp->DMEM + addr);
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 044: /* LBU */
                    offset = (signed short)(inst);
                    addr = (sp->SR[base] + offset) & 0x00000FFF;
                    sp->SR[rt] = sp->DMEM[BES(addr)];
                    sp->SR[rt] = (unsigned char)(sp->SR[rt]);
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 045: /* LHU */
                    offset = (signed short)(inst);
                    addr = (sp->SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 == 0x003)
                    {
                        SR_B(rt, 2) = sp->DMEM[addr - BES(0x000)];
                        addr = (addr + 0x00000001) & 0x00000FFF;
                        SR_B(rt, 3) = sp->DMEM[addr + BES(0x000)];
                        sp->SR[rt] = (unsigned short)(sp->SR[rt]);
                    }
                    else
                    {
                        addr -= HES(0x000)*(addr%0x004 - 1);
                        sp->SR[rt] = *(unsigned short *)(sp->DMEM + addr);
                    }
                    sp->SR[0] = 0x00000000;
                    CONTINUE
                case 050: /* SB */
                    offset = (signed short)(inst);
                    addr = (sp->SR[base] + offset) & 0x00000FFF;
                    sp->DMEM[BES(addr)] = (unsigned char)(sp->SR[rt]);
                    CONTINUE
                case 051: /* SH */
                    offset = (signed short)(inst);
                    addr = (sp->SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 == 0x003)
                    {
                        sp->DMEM[addr - BES(0x000)] = SR_B(rt, 2);
                        addr = (addr + 0x00000001) & 0x00000FFF;
                        sp->DMEM[addr + BES(0x000)] = SR_B(rt, 3);
                        CONTINUE
                    }
                    addr -= HES(0x000)*(addr%0x004 - 1);
                    *(short *)(sp->DMEM + addr) = (short)(sp->SR[rt]);
                    CONTINUE
                case 053: /* SW */
                    offset = (signed short)(inst);
                    addr = (sp->SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 != 0x000)
                        USW(sp, rt, addr);
                    else
                        *(int32_t *)(sp->DMEM + addr) = sp->SR[rt];
                    CONTINUE
                case 062: /* LWC2 */
                    offset = SE(inst, 6);
                    switch (rd)
                    {
                        case 000: /* LBV */
                            LBV(sp, rt, element, offset, base);
                            CONTINUE
                        case 001: /* LSV */
                            LSV(sp, rt, element, offset, base);
                            CONTINUE
                        case 002: /* LLV */
                            LLV(sp, rt, element, offset, base);
                            CONTINUE
                        case 003: /* LDV */
                            LDV(sp, rt, element, offset, base);
                            CONTINUE
                        case 004: /* LQV */
                            LQV(sp, rt, element, offset, base);
                            CONTINUE
                        case 005: /* LRV */
                            LRV(sp, rt, element, offset, base);
                            CONTINUE
                        case 006: /* LPV */
                            LPV(sp, rt, element, offset, base);
                            CONTINUE
                        case 007: /* LUV */
                            LUV(sp, rt, element, offset, base);
                            CONTINUE
                        case 010: /* LHV */
                            LHV(sp, rt, element, offset, base);
                            CONTINUE
                        case 011: /* LFV */
                            LFV(sp, rt, element, offset, base);
                            CONTINUE
                        case 013: /* LTV */
                            LTV(sp, rt, element, offset, base);
                            CONTINUE
                        default:
                            res_S(sp);
                            CONTINUE
                    }
                    CONTINUE
                case 072: /* SWC2 */
                    offset = SE(inst, 6);
                    switch (rd)
                    {
                        case 000: /* SBV */
                            SBV(sp, rt, element, offset, base);
                            CONTINUE
                        case 001: /* SSV */
                            SSV(sp, rt, element, offset, base);
                            CONTINUE
                        case 002: /* SLV */
                            SLV(sp, rt, element, offset, base);
                            CONTINUE
                        case 003: /* SDV */
                            SDV(sp, rt, element, offset, base);
                            CONTINUE
                        case 004: /* SQV */
                            SQV(sp, rt, element, offset, base);
                            CONTINUE
                        case 005: /* SRV */
                            SRV(sp, rt, element, offset, base);
                            CONTINUE
                        case 006: /* SPV */
                            SPV(sp, rt, element, offset, base);
                            CONTINUE
                        case 007: /* SUV */
                            SUV(sp, rt, element, offset, base);
                            CONTINUE
                        case 010: /* SHV */
                            SHV(sp, rt, element, offset, base);
                            CONTINUE
                        case 011: /* SFV */
                            SFV(sp, rt, element, offset, base);
                            CONTINUE
                        case 012: /* SWV */
                            SWV(sp, rt, element, offset, base);
                            CONTINUE
                        case 013: /* STV */
                            STV(sp, rt, element, offset, base);
                            CONTINUE
                        default:
                            res_S(sp);
                            CONTINUE
                    }
                    CONTINUE
                default:
                    res_S(sp);
                    CONTINUE
            }
        }
#ifndef EMULATE_STATIC_PC
        if (sp->stage == 2) /* branch phase of scheduler */
        {
            sp->stage = 0*stage;
            PC = sp->temp_PC & 0x00000FFC;
            sp->regs2[SP_PC_REG] = sp->temp_PC;
        }
        else
        {
            sp->stage = 2*sp->stage; /* next IW in branch delay slot? */
            PC = (PC + 0x004) & 0xFFC;
            if ( FIT_IMEM(PC) == 0 && ++wrap_count == 32 )
            {
                message( sp->r4300->state, "RSP execution presumably caught in an infinite loop", 3 );
                break;
            }
            sp->regs2[SP_PC_REG] = PC;
        }
        continue;
#else
        continue;
BRANCH:
        inst = *(uint32_t *)(sp->IMEM + FIT_IMEM(PC));
#ifdef SP_EXECUTE_LOG
        last_PC = PC;
#endif
        PC = sp->temp_PC & 0x00000FFC;
        goto EX;
#endif
    }
    sp->regs2[SP_PC_REG] = FIT_IMEM(PC);
    if (sp->regs[SP_STATUS_REG] & 0x00000002) /* normal exit, from executing BREAK */
        return;
    else if (sp->r4300->mi.regs[MI_INTR_REG] & 0x00000001) /* interrupt set by MTC0 to break */
        /*check_interupt(sp)*/;
    else if (CFG_WAIT_FOR_CPU_HOST != 0) /* plugin system hack to re-sync */
        {}
    else if (sp->regs[SP_SEMAPHORE_REG] != 0x00000000) /* semaphore lock fixes */
        {}
    else /* ??? unknown, possibly external intervention from CPU memory map */
    {
        message(sp->r4300->state, "SP_SET_HALT", 3);
        return;
    }
    sp->regs[SP_STATUS_REG] &= ~0x00000001; /* CPU restarts with the correct SIGs. */
    return;
}
