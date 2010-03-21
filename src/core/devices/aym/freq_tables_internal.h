/*
Abstract:
  Frequency tables internal interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __CORE_PLUGINS_PLAYERS_FREQ_TABLES_INTERNAL_DEFINED__
#define __CORE_PLUGINS_PLAYERS_FREQ_TABLES_INTERNAL_DEFINED__

#include <core/freq_tables.h>

class Error;

namespace ZXTune
{
  namespace Module
  {
    // getting frequency table data by name
    Error GetFreqTable(const String& id, FrequencyTable& result);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_FREQ_TABLES_INTERNAL_DEFINED__
