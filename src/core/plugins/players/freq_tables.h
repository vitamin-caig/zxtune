/*
Abstract:
  Frequency tables for AY-trackers

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <string_type.h>
#include <types.h>

#include <boost/array.hpp>

class Error;

namespace ZXTune
{
  namespace Module
  {
    typedef boost::array<uint16_t, 96> FrequencyTable;
    
    //table names
    const Char TABLE_SOUNDTRACKER[] = {'S', 'o', 'u', 'n', 'd', 'T', 'r', 'a', 'c', 'k', 'e', 'r', '\0'};
    
    Error GetFreqTable(const String& id, FrequencyTable& result);
  }
}
