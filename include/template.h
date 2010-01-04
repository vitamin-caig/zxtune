/*
Abstract:
  String template working

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __TEMPLATE_H_DEFINED__
#define __TEMPLATE_H_DEFINED__

#include <types.h>

String InstantiateTemplate(const String& templ, const StringMap& properties, Char beginMark = '[', Char endMark = ']');

#endif //__TEMPLATE_H_DEFINED__
