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

//library includes
#include <parameters/types.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace Core
    {
      //! @brief Plugins-related parameters namespace
      namespace Plugins
      {
        //! @brief Parameters#ZXTune#Core#Plugins namespace prefix
        extern const NameType PREFIX;
        
        //@{
        //! @name Default song duration in seconds (if not specified exactly) for all types (can be overriden)

        //! Default value (3min)
        const IntType DEFAULT_DURATION_DEFAULT = 3 * 60;
        //! Parameter name for type
        extern const NameType DEFAULT_DURATION;
        //@}

        //! @brief RAW scaner parameters namespace
        namespace Raw
        {
          //! @brief Parameters#ZXTune#Core#Plugins#Raw namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name Perform double analysis of plain data containers

          //! Parameter name
          extern const NameType PLAIN_DOUBLE_ANALYSIS;
          //@}

          //@{
          //! @name Minimal scan size parameter

          //! Default value
          const IntType MIN_SIZE_DEFAULT = 128;
          //! Parameter name
          extern const NameType MIN_SIZE;
          //@}
        }

        //! @brief HRIP container parameters namespace
        namespace Hrip
        {
          //! @brief Parameters#ZXTune#Core#Plugins#Hrip namespace prefix
          extern const NameType PREFIX;

          //! @brief Ignore corrupted blocks
          //! @details 1 if do so
          extern const NameType IGNORE_CORRUPTED;
        }

        //! @brief SID container/player parameters namespace
        namespace SID
        {
          //! @brief Parameters#ZXTune#Core#Plugins#SID namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name ROMs content
          
          //! 8192 bytes
          extern const NameType KERNAL;
          //! 8192 bytes
          extern const NameType BASIC;
          //! 4096 bytes
          extern const NameType CHARGEN;
          //@}
        }
        
        //! @brief ZIP container parameters namespace
        namespace Zip
        {
          //! @brief Parameters#ZXTune#Core#Plugins#Zip namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name Maximal file size to be depacked in Mb

          //! Default value
          const IntType MAX_DEPACKED_FILE_SIZE_MB_DEFAULT = 32;
          //! Parameter name
          extern const NameType MAX_DEPACKED_FILE_SIZE_MB;
          //@}
        }
      }
    }
  }
}
