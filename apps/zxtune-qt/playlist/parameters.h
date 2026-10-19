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

#include "apps/zxtune-qt/parameters.h"

namespace Parameters::ZXTuneQT::Playlist
{
  constexpr auto Prefix = ZXTuneQT::Prefix + "Playlist"_id;
  const auto NAMESPACE_NAME = static_cast<Identifier>(Prefix).Name();

  //! @brief Loop playlist playback
  struct Looped : Value<bool>
  {
    static constexpr auto Key = Prefix + "Loop"_id;
    static constexpr Type Default = false;
  };

  //! @brief Randomize playlist playback
  struct Randomized : Value<bool>
  {
    static constexpr auto Key = Prefix + "Random"_id;
    static constexpr Type Default = false;
  };

  //! @brief Index of last played tab
  struct Index : Value<uint_t>
  {
    static constexpr auto Key = Prefix + "Index"_id;
    static constexpr Type Default = 0;
  };

  //! @brief Index of last played track
  struct Track : Value<uint_t>
  {
    static constexpr auto Key = Prefix + "Track"_id;
    static constexpr Type Default = 0;
  };

  //! @brief Command line processing target
  enum class CmdlineTargetMode : uint_t
  {
    New = 0,
    Active = 1,
    Visible = 2,
  };

  struct CmdlineTarget : Value<CmdlineTargetMode>
  {
    static constexpr auto Key = Prefix + "CmdlineTarget"_id;
    static constexpr Type Default = CmdlineTargetMode::New;
  };

  namespace Cache
  {
    constexpr auto Prefix = Playlist::Prefix + "Cache"_id;

    //! @brief Cache size limit in MB
    struct MemoryLimitMb : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "Memory"_id;
      static constexpr Type Default = 10, Min = 0, Max = 100;
    };

    //! @brief Cache size limit in files
    struct FilesLimit : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "Files"_id;
      static constexpr Type Default = 1000, Min = 0, Max = 10'000;
    };
  }  // namespace Cache

  namespace Store
  {
    constexpr auto Prefix = Playlist::Prefix + "Store"_id;

    //! @brief Store full properties set instead of only changed
    struct Properties : Value<bool>
    {
      static constexpr auto Key = Prefix + "Properties"_id;
      static constexpr Type Default = false;
    };
  }  // namespace Store
}  // namespace Parameters::ZXTuneQT::Playlist
