/**
*
* @file     core/freq_tables.h
* @brief    Frequency tables for AY-trackers
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_FREQ_TABLES_H_DEFINED__
#define __CORE_FREQ_TABLES_H_DEFINED__

#include <string_type.h>
#include <types.h>

#include <boost/array.hpp>

namespace ZXTune
{
  namespace Module
  {
    //! @brief Frequency table type- 96 words
    typedef boost::array<uint16_t, 96> FrequencyTable;
    
    //! Sound Tracker frequency table
    const Char TABLE_SOUNDTRACKER[] = {'S', 'o', 'u', 'n', 'd', 'T', 'r', 'a', 'c', 'k', 'e', 'r', '\0'};
    //! Pro Tracker v2.x frequency table
    const Char TABLE_PROTRACKER2[] = {'P', 'r', 'o', 'T', 'r', 'a', 'c', 'k', 'e', 'r', '2', '\0'};
    //! ASC %Sound Master frequency table
    const Char TABLE_ASM[] = {'A', 'S', 'M', '\0'};
  }
}

#endif //__CORE_FREQ_TABLES_H_DEFINED__
