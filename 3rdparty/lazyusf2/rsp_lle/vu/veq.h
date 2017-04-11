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

INLINE static void do_eq(struct rsp_core* sp, short* VD, short* VS, short* VT)
{
    register int i;

#ifdef ARCH_MIN_ARM_NEON

	int16x8_t one = vdupq_n_s16(1);
	int16x8_t zero = vdupq_n_s16(0);
	int16x8_t vs = vld1q_s16((const int16_t *)VS);
    int16x8_t vt = vld1q_s16((const int16_t *)VT);
	int16x8_t v_ne = vld1q_s16((const int16_t *)sp->ne);

	uint16x8_t v_comp = vceqq_s16(vs, vt);
	int16x8_t v_comp_ = vnegq_s16((int16x8_t)v_comp);
	v_ne = veorq_s16(v_ne, one);
	v_comp_ = vandq_s16(v_comp_, v_ne);

    vector_copy(VACC_L, VT);
    vector_copy(VD, VACC_L);
	
	vst1q_s16(sp->comp,v_comp_);
	vst1q_s16(sp->ne,zero);
	vst1q_s16(sp->co,zero);

	return;

#else
	
    for (i = 0; i < N; i++)
        sp->clip[i] = 0;
    for (i = 0; i < N; i++)
        sp->comp[i] = (VS[i] == VT[i]);
    for (i = 0; i < N; i++)
        sp->comp[i] = sp->comp[i] & (sp->ne[i] ^ 1);
#if (0)
    merge(VACC_L, sp->comp, VS, VT); /* correct but redundant */
#else
    vector_copy(VACC_L, VT);
#endif
    vector_copy(VD, VACC_L);

    for (i = 0; i < N; i++)
        sp->ne[i] = 0;
    for (i = 0; i < N; i++)
        sp->co[i] = 0;
    return;
#endif
}

static void VEQ(struct rsp_core* sp, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, sp->VR[vt], e);
    do_eq(sp, sp->VR[vd], sp->VR[vs], ST);
    return;
}
