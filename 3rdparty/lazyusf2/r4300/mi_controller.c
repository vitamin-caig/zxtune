/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - mi_controller.c                                         *
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

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "mi_controller.h"
#include "r4300.h"
#include "r4300_core.h"
#include "cp0.h"
#include "interupt.h"

#include <string.h>

static int update_mi_init_mode(uint32_t* mi_init_mode, uint32_t w)
{
    int clear_dp = 0;

    /* set init_length */
    *mi_init_mode &= ~0x7f;
    *mi_init_mode |= w & 0x7f;
    /* clear / set init_mode */
    if (w & 0x80)  *mi_init_mode &= ~0x80;
    if (w & 0x100) *mi_init_mode |= 0x80;
    /* clear / set ebus test_mode */
    if (w & 0x200) *mi_init_mode &= ~0x100;
    if (w & 0x400) *mi_init_mode |= 0x100;
    /* clear DP interrupt */
    if (w & 0x800) clear_dp = 1;
    /* clear / set RDRAM reg_mode */
    if (w & 0x1000) *mi_init_mode &= ~0x200;
    if (w & 0x2000) *mi_init_mode |= 0x200;

    return clear_dp;
}

static void update_mi_intr_mask(uint32_t* mi_intr_mask, uint32_t w)
{
    if (w & 0x1)   *mi_intr_mask &= ~0x1; // clear SP mask
    if (w & 0x2)   *mi_intr_mask |= 0x1; // set SP mask
    if (w & 0x4)   *mi_intr_mask &= ~0x2; // clear SI mask
    if (w & 0x8)   *mi_intr_mask |= 0x2; // set SI mask
    if (w & 0x10)  *mi_intr_mask &= ~0x4; // clear AI mask
    if (w & 0x20)  *mi_intr_mask |= 0x4; // set AI mask
    if (w & 0x40)  *mi_intr_mask &= ~0x8; // clear VI mask
    if (w & 0x80)  *mi_intr_mask |= 0x8; // set VI mask
    if (w & 0x100) *mi_intr_mask &= ~0x10; // clear PI mask
    if (w & 0x200) *mi_intr_mask |= 0x10; // set PI mask
    if (w & 0x400) *mi_intr_mask &= ~0x20; // clear DP mask
    if (w & 0x800) *mi_intr_mask |= 0x20; // set DP mask
}

void init_mi(struct mi_controller* mi)
{
    memset(mi->regs, 0, MI_REGS_COUNT*sizeof(uint32_t));
    mi->regs[MI_VERSION_REG] = 0x02020102;
    mi->AudioIntrReg = 0;
}


uint32_t read_mi_regs(void* opaque, uint32_t address)
{
    struct r4300_core* r4300 = (struct r4300_core*)opaque;
    uint32_t reg = mi_reg(address);

    return r4300->mi.regs[reg];
}

void write_mi_regs(void* opaque, uint32_t address, uint32_t value, uint32_t mask)
{
    struct r4300_core* r4300 = (struct r4300_core*)opaque;
    uint32_t reg = mi_reg(address);

    switch(reg)
    {
    case MI_INIT_MODE_REG:
        if (update_mi_init_mode(&r4300->mi.regs[MI_INIT_MODE_REG], value & mask) != 0)
        {
            clear_rcp_interrupt(r4300, MI_INTR_DP);
        }
        break;
    case MI_INTR_MASK_REG:
        update_mi_intr_mask(&r4300->mi.regs[MI_INTR_MASK_REG], value & mask);

        check_interupt(r4300->state);
        update_count(r4300->state);
        if (r4300->state->cycle_count >= 0) gen_interupt(r4300->state);
        break;
    }
}

/* interrupt execution is immediate (if not masked) */
void raise_rcp_interrupt(struct r4300_core* r4300, uint32_t mi_intr)
{
    r4300->mi.regs[MI_INTR_REG] |= mi_intr;

    if (r4300->mi.regs[MI_INTR_REG] & r4300->mi.regs[MI_INTR_MASK_REG])
        raise_maskable_interrupt(r4300->state, 0x400);
}

/* interrupt execution is scheduled (if not masked) */
void signal_rcp_interrupt(struct r4300_core* r4300, uint32_t mi_intr)
{
    r4300->mi.regs[MI_INTR_REG] |= mi_intr;
    check_interupt(r4300->state);
}

void clear_rcp_interrupt(struct r4300_core* r4300, uint32_t mi_intr)
{
    r4300->mi.regs[MI_INTR_REG] &= ~mi_intr;
    check_interupt(r4300->state);
}

