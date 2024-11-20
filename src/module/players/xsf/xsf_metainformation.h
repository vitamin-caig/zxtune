/**
 *
 * @file
 *
 * @brief  Xsf-based files structure support. Metainformation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "time/duration.h"

#include "string_type.h"
#include "types.h"

#include <memory>
#include <utility>
#include <vector>

namespace Parameters
{
  class Modifier;
}

namespace Module::XSF
{
  struct MetaInformation
  {
    using Ptr = std::shared_ptr<const MetaInformation>;
    using RWPtr = std::shared_ptr<MetaInformation>;

    String Title;
    String Artist;
    String Game;
    String Year;
    String Genre;
    String Comment;
    String Copyright;
    String Dumper;

    std::vector<std::pair<String, String>> Tags;

    uint_t RefreshRate = 0;
    Time::Milliseconds Duration;
    Time::Milliseconds Fadeout;

    float Volume = 0.0f;

    void Merge(const MetaInformation& rh);

    void Dump(Parameters::Modifier& out) const;
  };
}  // namespace Module::XSF
