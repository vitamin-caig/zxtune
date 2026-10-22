/**
 *
 * @file
 *
 * @brief Update parameters definition
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/parameters.h"

namespace Parameters::ZXTuneQT::Update
{
  constexpr auto Prefix = ZXTuneQT::Prefix + "Update"_id;

  //! @brief Feed URL (code default is Urls::DownloadsFeed())
  struct Feed : Value<StringType>
  {
    static constexpr auto Key = Prefix + "Feed"_id;
  };

  //! @brief Check period in seconds; zero disables the checks
  struct CheckPeriod : Value<uint_t>
  {
    static constexpr auto Key = Prefix + "CheckPeriod"_id;
    static constexpr Type Default = 86400;
  };

  //! @brief Last check timestamp
  struct LastCheck : Value<uint_t>
  {
    static constexpr auto Key = Prefix + "LastCheck"_id;
    static constexpr Type Default = 0;
  };
}  // namespace Parameters::ZXTuneQT::Update
