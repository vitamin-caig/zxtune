/**
 *
 * @file
 *
 * @brief  IO parameters names
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <zxtune.h>

namespace Parameters
{
  namespace ZXTune
  {
    //! @brief IO-parameters namespace
    namespace IO
    {
      //! @brief Parameters#ZXTune#IO namespace prefix
      const auto PREFIX = ZXTune::PREFIX + "io"_id;
      // IO-related parameters
    }  // namespace IO
  }    // namespace ZXTune
}  // namespace Parameters
