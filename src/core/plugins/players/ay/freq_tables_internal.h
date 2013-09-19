/*
Abstract:
  Frequency tables internal interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_FREQ_TABLES_INTERNAL_DEFINED__
#define __CORE_PLUGINS_PLAYERS_FREQ_TABLES_INTERNAL_DEFINED__

//library includes
#include <core/freq_tables.h>

namespace Module
{
  // getting frequency table data by name
  // throw Error in case of problem
  void GetFreqTable(const String& id, FrequencyTable& result);
}

#endif //__CORE_PLUGINS_PLAYERS_FREQ_TABLES_INTERNAL_DEFINED__
