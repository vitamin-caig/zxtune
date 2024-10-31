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

#include "string_view.h"
#include "types.h"

#include <array>

namespace Module
{
  //! @brief Frequency table type- 96 words
  using FrequencyTable = std::array<uint16_t, 96>;

  // clang-format off
  //! %Sound Tracker frequency table
  const auto TABLE_SOUNDTRACKER = "SoundTracker"sv;
  //! Pro Tracker v2.x frequency table
  const auto TABLE_PROTRACKER2 = "ProTracker2"sv;
  //! Pro Tracker v3.3 native frequency table
  const auto TABLE_PROTRACKER3_3 = "ProTracker3.3"sv;
  //! Pro Tracker v3.4 native frequency table
  const auto TABLE_PROTRACKER3_4 = "ProTracker3.4"sv;
  //! Pro Tracker v3.x ST frequency table
  const auto TABLE_PROTRACKER3_ST = "ProTracker3_ST"sv;
  //! Pro Tracker v3.3 ASM frequency table
  const auto TABLE_PROTRACKER3_3_ASM = "ProTracker3.3_ASM"sv;
  //! Pro Tracker v3.4 ASM frequency table
  const auto TABLE_PROTRACKER3_4_ASM = "ProTracker3.4_ASM"sv;
  //! Pro Tracker v3.3 real frequency table
  const auto TABLE_PROTRACKER3_3_REAL = "ProTracker3.3_Real"sv;
  //! Pro Tracker v3.4 real frequency table
  const auto TABLE_PROTRACKER3_4_REAL = "ProTracker3.4_Real"sv;
  //! ASC %Sound Master frequency table
  const auto TABLE_ASM = "ASM"sv;
  //! %Sound Tracker Pro frequency table
  const auto TABLE_SOUNDTRACKER_PRO = "SoundTrackerPro"sv;
  //! Natural scaled frequency table (best for 1520640Hz)
  const auto TABLE_NATURAL_SCALED = "NaturalScaled"sv;
  //! Pro Sound Maker frequency table
  const auto TABLE_PROSOUNDMAKER = "ProSoundMaker"sv;
  //! SQ-Tracker frequency table
  const auto TABLE_SQTRACKER = "SQTracker"sv;
  //! Fuxoft frequency table
  const auto TABLE_FUXOFT = "Fuxoft"sv;
  //! FastTracker frequency table
  const auto TABLE_FASTTRACKER = "FastTracker"sv;
  // clang-format on
}  // namespace Module
