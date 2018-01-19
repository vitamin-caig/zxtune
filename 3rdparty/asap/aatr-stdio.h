/*
 * aatr-stdio.h - another ATR file extractor
 *
 * Copyright (C) 2012-2013  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _AATR_STDIO_H_
#define _AATR_STDIO_H_

#include <stdio.h>
#include "aatr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AATR AATR;

AATR *AATRStdio_New(FILE *fp);
int AATRStdio_ReadFile(const char *atr_filename, const char *inside_filename, unsigned char *buffer, int length);


#ifdef __cplusplus
}
#endif
#endif
