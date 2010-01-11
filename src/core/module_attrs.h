/*
Abstract:
  Module attributes names

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __CORE_MODULE_ATTRS_H_DEFINED__
#define __CORE_MODULE_ATTRS_H_DEFINED__

#include <char_type.h>

namespace ZXTune
{
  namespace Module
  {
    const Char ATTR_TYPE[] = {'T', 'y', 'p', 'e', '\0'};
    const Char ATTR_CONTAINER[] = {'C', 'o', 'n', 't', 'a', 'i', 'n', 'e', 'r', '\0'};
    const Char ATTR_SUBPATH[] = {'S', 'u', 'b', 'p', 'a', 't', 'h', '\0'};
    const Char ATTR_AUTHOR[] = {'A', 'u', 't', 'h', 'o', 'r', '\0'};
    const Char ATTR_TITLE[] = {'T', 'i', 't', 'l', 'e', '\0'};
    const Char ATTR_PROGRAM[] = {'P', 'r', 'o', 'g', 'r', 'a', 'm', '\0'};
    const Char ATTR_COMPUTER[] = {'C', 'o', 'm', 'p', 'u', 't', 'e', 'r', '\0'};
    const Char ATTR_DATE[] = {'D', 'a', 't', 'e', '\0'};
    const Char ATTR_COMMENT[] = {'C', 'o', 'm', 'm', 'e', 'n', 't', '\0'};
    const Char ATTR_WARNINGS[] = {'W', 'a', 'r', 'n', 'i', 'n', 'g', 's', '\0'};
    const Char ATTR_WARNINGS_COUNT[] = {'W', 'a', 'r', 'n', 'i', 'n', 'g', 's', 'C', 'o', 'u', 'n', 't', '\0'};
    const Char ATTR_CRC[] = {'C', 'R', 'C', '\0'};
    const Char ATTR_SIZE[] = {'S', 'i', 'z', 'e', '\0'};
  }
}

#endif //__CORE_MODULE_ATTRS_H_DEFINED__
