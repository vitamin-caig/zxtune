/*
Abstract:
  Chars type definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CHAR_TYPE_H_DEFINED__
#define __CHAR_TYPE_H_DEFINED__

#ifdef UNICODE
typedef wchar_t Char;
#else
typedef char Char;
#endif

#endif //__CHAR_TYPE_H_DEFINED__
