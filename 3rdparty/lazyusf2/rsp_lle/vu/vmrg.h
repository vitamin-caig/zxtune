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

INLINE static void do_mrg(struct rsp_core* sp, short* VD, short* VS, short* VT)
{
    merge(VACC_L, sp->comp, VS, VT);
    vector_copy(VD, VACC_L);
    return;
}

static void VMRG(struct rsp_core* sp, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, sp->VR[vt], e);
    do_mrg(sp, sp->VR[vd], sp->VR[vs], ST);
    return;
}
