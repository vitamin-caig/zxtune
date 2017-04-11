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

INLINE static void do_ne(struct rsp_core* sp, short* VD, short* VS, short* VT)
{

#ifdef ARCH_MIN_ARM_NEON
	
	int16x8_t vs, vt,vaccl, ne;
	int16x8_t zero = vdupq_n_s16(0);
	uint16x8_t cond;
	
	vs = vld1q_s16((const int16_t*)VS);
	vt = vld1q_s16((const int16_t*)VT);
	ne = vld1q_s16((const int16_t*)sp->ne);

	cond = vceqq_s16(vs,vt);
	cond = vmvnq_u16(cond); // this is needed if you need to do "not-equal"
	cond = (uint16x8_t)vnegq_s16((int16x8_t)cond);
	uint16x8_t comp = vorrq_u16(cond, (uint16x8_t)ne);
	
	vst1q_s16(sp->clip, zero);
	vst1q_s16(sp->comp, (int16x8_t)cond);

	vector_copy(VACC_L, VS);		
	vector_copy(VD, VACC_L);
	vst1q_s16(sp->ne, zero);
	vst1q_s16(sp->co, zero);	
	
	return;
#else

    register int i;

    for (i = 0; i < N; i++)
        sp->clip[i] = 0;
    for (i = 0; i < N; i++)
        sp->comp[i] = (VS[i] != VT[i]);
    for (i = 0; i < N; i++)
        sp->comp[i] = sp->comp[i] | sp->ne[i];
#if (0)
    merge(VACC_L, sp->comp, VS, VT); /* correct but redundant */
#else
    vector_copy(VACC_L, VS);
#endif
    vector_copy(VD, VACC_L);

    for (i = 0; i < N; i++)
        sp->ne[i] = 0;
    for (i = 0; i < N; i++)
        sp->co[i] = 0;
    return;
#endif
}

static void VNE(struct rsp_core* sp, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, sp->VR[vt], e);
    do_ne(sp, sp->VR[vd], sp->VR[vs], ST);
    return;
}
