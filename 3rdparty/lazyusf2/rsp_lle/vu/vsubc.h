/******************************************************************************\
* Authors:  Iconoclast                                                         *
* Release:  2013.11.26                                                         *
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
#include "vu.h"

INLINE static void set_bo(struct rsp_core* sp, short* VD, short* VS, short* VT)
{ /* set CARRY and borrow out from difference */

#ifdef ARCH_MIN_ARM_NEON

	int16x8_t vs, vt,vaccl,ne, co2;
	uint16x8_t cond;
	
	int16x8_t zero = vdupq_n_s16(0);
	int16x8_t one = vdupq_n_s16(1);
	vs = vld1q_s16((const int16_t*)VS);
	vt = vld1q_s16((const int16_t*)VT);
	
	vaccl = vsubq_s16(vs, vt);
	uint16x8_t vdif = vqsubq_u16((uint16x8_t)vs, (uint16x8_t)vt);
	
	vst1q_s16(VACC_L, vaccl);
	vector_copy(VD, VACC_L);
	
	cond = vceqq_s16(vs, vt);
	ne = vaddq_s16((int16x8_t)cond, one);

	vdif = vorrq_u16(vdif,cond);
	cond = vceqq_u16(vdif, (uint16x8_t)zero);
	co2 = vnegq_s16((int16x8_t)cond);
	
	vst1q_s16(sp->ne, ne);
	vst1q_s16(sp->co, co2);
	return;

#else

    ALIGNED int32_t dif[N];
    register int i;

    for (i = 0; i < N; i++)
        dif[i] = (unsigned short)(VS[i]) - (unsigned short)(VT[i]);
    for (i = 0; i < N; i++)
        VACC_L[i] = VS[i] - VT[i];
    vector_copy(VD, VACC_L);
    for (i = 0; i < N; i++)
        sp->ne[i] = (VS[i] != VT[i]);
    for (i = 0; i < N; i++)
        sp->co[i] = (dif[i] < 0);
    return;
#endif
}

static void VSUBC(struct rsp_core* sp, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, sp->VR[vt], e);
    set_bo(sp, sp->VR[vd], sp->VR[vs], ST);
    return;
}
