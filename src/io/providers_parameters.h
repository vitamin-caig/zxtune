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

// library includes
#include <io/io_parameters.h>
#include <parameters/types.h>

namespace Parameters::ZXTune::IO::Providers
{
  //! @brief Parameters#ZXTune#IO#Providers namespace prefix
  const auto PREFIX = IO::PREFIX + "providers"_id;

  //! @brief %File provider parameters namespace
  namespace File
  {
    //! @brief Parameters#ZXTune#IO#Providers#File namespace prefix
    const auto PREFIX = Providers::PREFIX + "file"_id;
    //@{
    //! @name Memory-mapping usage data size threshold parameter.

    //! Default value
    const IntType MMAP_THRESHOLD_DEFAULT = 16384;
    //! Parameter full path
    const auto MMAP_THRESHOLD = PREFIX + "mmap_threshold"_id;
    //@}

    //@{
    //! @name Create intermediate directories

    //! Default value
    const IntType CREATE_DIRECTORIES_DEFAULT = 1;
    //! @Parameter full path
    const auto CREATE_DIRECTORIES = PREFIX + "create_directories"_id;
    //@}

    //@{
    //! @name Overwrite files

    //! Default value
    const IntType OVERWRITE_EXISTING_DEFAULT = 0;
    //! @Parameter full path
    const auto OVERWRITE_EXISTING = PREFIX + "overwrite"_id;
    //@}

    //@{
    //! @name Sanitize name's components

    //! Default value
    const IntType SANITIZE_NAMES_DEFAULT = 1;
    //! @Parameter full path
    const auto SANITIZE_NAMES = PREFIX + "sanitize"_id;
  }  // namespace File

  //! @brief %Network provider parameters namespace
  namespace Network
  {
    //! @brief Parameters#ZXTune#IO#Providers#Network namespace prefix
    const auto PREFIX = Providers::PREFIX + "network"_id;

    //! @brief Parameters for HTTP protocol
    namespace Http
    {
      //! @brief Parameters#ZXTune#IO#Providers#Network#Http namespace prefix
      const auto PREFIX = Network::PREFIX + "http"_id;

      //@{
      //! @name Useragent parameter

      //! Parameter full path
      const auto USERAGENT = PREFIX + "useragent"_id;
    }  // namespace Http
  }    // namespace Network
}  // namespace Parameters::ZXTune::IO::Providers
