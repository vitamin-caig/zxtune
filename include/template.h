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

#include <string_helpers.h>

enum InstantiateMode
{
  KEEP_NONEXISTING,
  SKIP_NONEXISTING,
  FILL_NONEXISTING
};

String InstantiateTemplate(const String& templ, const StringMap& properties, 
  InstantiateMode mode = KEEP_NONEXISTING, Char beginMark = '[', Char endMark = ']');

#endif //__TEMPLATE_H_DEFINED__
