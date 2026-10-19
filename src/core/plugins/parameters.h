/**
 *
 * @file
 *
 * @brief  Plugins parameters names
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "core/parameters.h"

namespace Parameters::ZXTune::Core::Plugins
{
  constexpr auto Prefix = Core::Prefix + "plugins"_id;

  //! @brief Default song duration in seconds if not specified exactly (can be overridden)
  struct DefaultDuration : Value<uint_t>
  {
    static constexpr auto Key = Prefix + "default_duration"_id;
    static constexpr Type Default = 3 * 60, Min = 30, Max = 600;
  };

  //! @brief RAW scaner parameters namespace
  namespace Raw
  {
    constexpr auto Prefix = Plugins::Prefix + "raw"_id;

    //! @brief Perform double analysis of plain data containers
    struct PlainDoubleAnalysis : Value<bool>
    {
      static constexpr auto Key = Prefix + "plain_double_analysis"_id;
      static constexpr Type Default = false;
    };

    //! @brief Minimal scan size in bytes
    struct MinSize : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "min_size"_id;
      static constexpr Type Default = 128, Min = 128, Max = 1024;
    };

    //! @brief Scan window limit in bytes; 0 means unlimited
    struct ScanLimit : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "scan_limit"_id;
      static constexpr Type Default = 5 * 1048576;
    };
  }  // namespace Raw

  //! @brief HRIP container parameters namespace
  namespace Hrip
  {
    constexpr auto Prefix = Plugins::Prefix + "hrip"_id;

    //! @brief Ignore corrupted blocks
    struct IgnoreCorrupted : Value<bool>
    {
      static constexpr auto Key = Prefix + "ignore_corrupted"_id;
      static constexpr Type Default = false;
    };
  }  // namespace Hrip

  //! @brief SID container/player parameters namespace
  namespace SID
  {
    constexpr auto Prefix = Plugins::Prefix + "sid"_id;

    //! @brief 8192-byte KERNAL ROM
    struct Kernal : Value<Binary::Data>
    {
      static constexpr auto Key = Prefix + "kernal"_id;
    };

    //! @brief 8192-byte BASIC ROM
    struct Basic : Value<Binary::Data>
    {
      static constexpr auto Key = Prefix + "basic"_id;
    };

    //! @brief 4096-byte CHARGEN ROM
    struct Chargen : Value<Binary::Data>
    {
      static constexpr auto Key = Prefix + "chargen"_id;
    };
  }  // namespace SID

  //! @brief ZIP container parameters namespace
  namespace Zip
  {
    constexpr auto Prefix = Plugins::Prefix + "zip"_id;

    //! @brief Maximal file size to be depacked in Mb
    struct MaxDepackedFileSizeMb : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "max_depacked_size_mb"_id;
      static constexpr Type Default = 32;
    };
  }  // namespace Zip
}  // namespace Parameters::ZXTune::Core::Plugins
