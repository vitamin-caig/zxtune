/**
 *
 * @file
 *
 * @brief  Frequency tables for AY-trackers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// std includes
#include <array>

namespace Module
{
  //! @brief Frequency table type- 96 words
  using FrequencyTable = std::array<uint16_t, 96>;

  // clang-format off
  //! %Sound Tracker frequency table
  const Char TABLE_SOUNDTRACKER[] = {'S','o','u','n','d','T','r','a','c','k','e','r','\0'};
  //! Pro Tracker v2.x frequency table
  const Char TABLE_PROTRACKER2[] = {'P','r','o','T','r','a','c','k','e','r','2','\0'};
  //! Pro Tracker v3.3 native frequency table
  const Char TABLE_PROTRACKER3_3[] = {'P','r','o','T','r','a','c','k','e','r','3','.','3','\0'};
  //! Pro Tracker v3.4 native frequency table
  const Char TABLE_PROTRACKER3_4[] = {'P','r','o','T','r','a','c','k','e','r','3','.','4','\0'};
  //! Pro Tracker v3.x ST frequency table
  const Char TABLE_PROTRACKER3_ST[] = {'P','r','o','T','r','a','c','k','e','r','3','_','S','T','\0'};
  //! Pro Tracker v3.3 ASM frequency table
  const Char TABLE_PROTRACKER3_3_ASM[] = {'P','r','o','T','r','a','c','k','e','r','3','.','3','_','A','S','M','\0'};
  //! Pro Tracker v3.4 ASM frequency table
  const Char TABLE_PROTRACKER3_4_ASM[] = {'P','r','o','T','r','a','c','k','e','r','3','.','4','_','A','S','M','\0'};
  //! Pro Tracker v3.3 real frequency table
  const Char TABLE_PROTRACKER3_3_REAL[] = {'P','r','o','T','r','a','c','k','e','r','3','.','3','_','R','e','a','l','\0'};
  //! Pro Tracker v3.4 real frequency table
  const Char TABLE_PROTRACKER3_4_REAL[] = {'P','r','o','T','r','a','c','k','e','r','3','.','4','_','R','e','a','l','\0'};
  //! ASC %Sound Master frequency table
  const Char TABLE_ASM[] = {'A', 'S', 'M', '\0'};
  //! %Sound Tracker Pro frequency table
  const Char TABLE_SOUNDTRACKER_PRO[] = {'S','o','u','n','d','T','r','a','c','k','e','r','P','r','o','\0'};
  //! Natural scaled frequency table (best for 1520640Hz)
  const Char TABLE_NATURAL_SCALED[] = {'N','a','t','u','r','a','l','S','c','a','l','e','d','\0'};
  //! Pro Sound Maker frequency table
  const Char TABLE_PROSOUNDMAKER[] = {'P','r','o','S','o','u','n','d','M','a','k','e','r','\0'};
  //! SQ-Tracker frequency table
  const Char TABLE_SQTRACKER[] = {'S','Q','T','r','a','c','k','e','r','\0'};
  //! Fuxoft frequency table
  const Char TABLE_FUXOFT[] = {'F','u','x','o','f','t','\0'};
  //! FastTracker frequency table
  const Char TABLE_FASTTRACKER[] = {'F','a','s','t','T','r','a','c','k','e','r','\0'};
  // clang-format on
}  // namespace Module
