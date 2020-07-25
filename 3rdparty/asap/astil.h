/*
 * astil.h - another SID/SAP Tune Information List parser
 *
 * Copyright (C) 2011-2012  Piotr Fusik
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

#ifndef _ASTIL_H_
#define _ASTIL_H_
#ifndef _ASAP_H_ /* FIXME */
typedef int cibool;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ASTIL ASTIL;
typedef struct ASTILCover ASTILCover;

ASTIL *ASTIL_New(void);
void ASTIL_Delete(ASTIL *self);
cibool ASTIL_Load(ASTIL *self, const char *filename, int song);
const char *ASTIL_GetStilFilename(const ASTIL *self);
/* If TRUE, the following strings are expected to be UTF-8 encoded,
   otherwise it's an old STIL.txt, ASCII or Windows-1250 maybe? */
cibool ASTIL_IsUTF8(const ASTIL *self);
const char *ASTIL_GetTitle(const ASTIL *self);
const char *ASTIL_GetAuthor(const ASTIL *self);
const char *ASTIL_GetDirectoryComment(const ASTIL *self);
const char *ASTIL_GetFileComment(const ASTIL *self);
const char *ASTIL_GetSongComment(const ASTIL *self);
const ASTILCover *ASTIL_GetCover(const ASTIL *self, int i);

int ASTILCover_GetStartSeconds(const ASTILCover *self);
int ASTILCover_GetEndSeconds(const ASTILCover *self);
const char *ASTILCover_GetTitleAndSource(const ASTILCover *self);
const char *ASTILCover_GetArtist(const ASTILCover *self);
const char *ASTILCover_GetComment(const ASTILCover *self);

#ifdef __cplusplus
}
#endif
#endif
