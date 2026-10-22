/**
 *
 * @file
 *
 * @brief  Providers parameters names
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "io/io_parameters.h"
#include "parameters/value.h"

namespace Parameters::ZXTune::IO::Providers
{
  constexpr auto Prefix = IO::Prefix + "providers"_id;

  //! @brief %File provider parameters namespace
  namespace File
  {
    constexpr auto Prefix = Providers::Prefix + "file"_id;

    //! @brief Memory-mapping usage data size threshold in bytes
    struct MmapThreshold : Value<uint64_t>
    {
      static constexpr auto Key = Prefix + "mmap_threshold"_id;
      static constexpr Type Default = 16384;
    };

    //! @brief Create intermediate directories
    struct CreateDirectories : Value<bool>
    {
      static constexpr auto Key = Prefix + "create_directories"_id;
      static constexpr Type Default = true;
    };

    //! @brief Overwrite files
    //! @details Type is IO::OverwriteMode
    struct OverwriteExisting : Value<uint_t>
    {
      static constexpr auto Key = Prefix + "overwrite"_id;
      static constexpr Type Default = 0;
    };

    //! @brief Sanitize name's components
    struct SanitizeNames : Value<bool>
    {
      static constexpr auto Key = Prefix + "sanitize"_id;
      static constexpr Type Default = true;
    };
  }  // namespace File

  //! @brief %Network provider parameters namespace
  namespace Network
  {
    constexpr auto Prefix = Providers::Prefix + "network"_id;

    //! @brief Parameters for HTTP protocol
    namespace Http
    {
      constexpr auto Prefix = Network::Prefix + "http"_id;

      //! @brief Useragent
      struct UserAgent : Value<StringType>
      {
        static constexpr auto Key = Prefix + "useragent"_id;
      };
    }  // namespace Http
  }    // namespace Network
}  // namespace Parameters::ZXTune::IO::Providers
