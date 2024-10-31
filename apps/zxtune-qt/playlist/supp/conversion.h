/**
 *
 * @file
 *
 * @brief Conversion-related BL
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <parameters/accessor.h>

namespace Playlist::Item::Conversion
{
  struct Options
  {
    using Ptr = std::shared_ptr<const Options>;

    Options(String type, String filenameTemplate, Parameters::Accessor::Ptr params)
      : Type(std::move(type))
      , FilenameTemplate(std::move(filenameTemplate))
      , Params(std::move(params))
    {}

    const String Type;
    const String FilenameTemplate;
    const Parameters::Accessor::Ptr Params;
  };
}  // namespace Playlist::Item::Conversion
