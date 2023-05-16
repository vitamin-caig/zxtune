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

// library includes
#include <parameters/accessor.h>

namespace Playlist::Item::Conversion
{
  struct Options
  {
    typedef std::shared_ptr<const Options> Ptr;

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
