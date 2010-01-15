/*
Abstract:
  Frequency tables for AY-trackers

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __CORE_FREQ_TABLES_H_DEFINED__
#define __CORE_FREQ_TABLES_H_DEFINED__

#include <string_type.h>
#include <types.h>

#include <boost/array.hpp>

namespace ZXTune
{
  namespace Module
  {
    typedef boost::array<uint16_t, 96> FrequencyTable;
    
    //table names
    const Char TABLE_SOUNDTRACKER[] = {'S', 'o', 'u', 'n', 'd', 'T', 'r', 'a', 'c', 'k', 'e', 'r', '\0'};
    const Char TABLE_ASM[] = {'A', 'S', 'M', '\0'};
  }
}

#endif //__CORE_FREQ_TABLES_H_DEFINED__
