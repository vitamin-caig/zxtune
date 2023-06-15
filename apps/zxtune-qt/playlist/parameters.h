/**
 *
 * @file
 *
 * @brief Playlist parameters definition
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "app_parameters.h"

namespace Parameters::ZXTuneQT::Playlist
{
  const auto PREFIX = ZXTuneQT::PREFIX + "Playlist"_id;
  const auto NAMESPACE_NAME = static_cast<Identifier>(PREFIX).Name();

  //@{
  //! @name Loop playlist playback

  //! Parameter name
  const auto LOOPED = PREFIX + "Loop"_id;
  //! Default value
  const IntType LOOPED_DEFAULT = 0;
  //@}

  //@{
  //! @name Randomize playlist playback

  //! Parameter name
  const auto RANDOMIZED = PREFIX + "Random"_id;
  //! Default value
  const IntType RANDOMIZED_DEFAULT = 0;
  //@}

  //@}
  //! @name Index of last played tab
  const auto INDEX = PREFIX + "Index"_id;

  //! @name Index of last played track
  const auto TRACK = PREFIX + "Track"_id;
  //@}

  const auto CMDLINE_TARGET = PREFIX + "CmdlineTarget"_id;

  const IntType CMDLINE_TARGET_NEW = 0;
  const IntType CMDLINE_TARGET_ACTIVE = 1;
  const IntType CMDLINE_TARGET_VISIBLE = 2;

  const IntType CMDLINE_TARGET_DEFAULT = CMDLINE_TARGET_NEW;

  namespace Cache
  {
    const auto PREFIX = Playlist::PREFIX + "Cache"_id;

    //@{
    //! @name Cache size limit in bytes

    //! Parameter name
    const auto MEMORY_LIMIT_MB = PREFIX + "Memory"_id;
    //! Default value
    const IntType MEMORY_LIMIT_MB_DEFAULT = 10;
    //@}

    //@{
    //! @name Cache size limit in files

    //! Parameter name
    const auto FILES_LIMIT = PREFIX + "Files"_id;
    //! Default value
    const IntType FILES_LIMIT_DEFAULT = 1000;
    //@}
  }  // namespace Cache

  namespace Store
  {
    const auto PREFIX = Playlist::PREFIX + "Store"_id;

    //@{

    //! @name Store full properties set instead of only changed

    //! Parameter name
    const auto PROPERTIES = PREFIX + "Properties"_id;
    //! Default value
    const IntType PROPERTIES_DEFAULT = 0;
    //@}
  }  // namespace Store
}  // namespace Parameters::ZXTuneQT::Playlist
