/* Extended Module Player
 * Copyright (C) 1996-2013 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU Lesser General Public License. See COPYING.LIB
 * for more information.
 */

#include <stdlib.h>
#include <string.h>
#include "loaders/prowizard/prowiz.h"
#include "format.h"
#include "xmp_private.h"

extern const struct pw_format *const pw_format[];

const struct format_loader *const format_loader[NUM_FORMATS + 2] = {
	&xm_loader,
	&mod_loader,
	&flt_loader,
	&st_loader,
	&it_loader,
	&s3m_loader,
	&stm_loader,
	&stx_loader,
	&mtm_loader,
	&ice_loader,
	&imf_loader,
	&ptm_loader,
	&mdl_loader,
	&ult_loader,
	&liq_loader,
	&no_loader,
	&masi_loader,
	&gal5_loader,
	&gal4_loader,
	&psm_loader,
	&amf_loader,
	&asylum_loader,
	&gdm_loader,
	&mmd1_loader,
	&mmd3_loader,
	&med2_loader,
	&med3_loader,
	&med4_loader,
	&dmf_loader,
	&rtm_loader,
	&pt3_loader,
	&tcb_loader,
	&dt_loader,
	&gtk_loader,
	&dtt_loader,
	&mgt_loader,
	&arch_loader,
	&sym_loader,
	&digi_loader,
	&dbm_loader,
	&emod_loader,
	&okt_loader,
	&sfx_loader,
	&far_loader,
	&umx_loader,
	&hmn_loader,
	&stim_loader,
	&coco_loader,
	&mtp_loader,
	&ims_loader,
	&ssn_loader,
	&fnk_loader,
	&mfp_loader,
	/* &alm_loader, */
	&polly_loader,
	&pw_loader,
	NULL
};

static const char *_farray[NUM_FORMATS + NUM_PW_FORMATS + 1] = { NULL };

char **format_list()
{
	int count, i, j;

	if (_farray[0] == NULL) {
		for (count = i = 0; format_loader[i] != NULL; i++) {
			if (strcmp(format_loader[i]->name, "prowizard") == 0) {
				for (j = 0; pw_format[j] != NULL; j++) {
					_farray[count++] = pw_format[j]->name;
				}
			} else {
				_farray[count++] = format_loader[i]->name;
			}
		}

		_farray[count] = NULL;
	}

	return (char **)_farray;
}

const char* xmp_get_loader_name(const struct format_loader* loader)
{
  return loader->name;
}
