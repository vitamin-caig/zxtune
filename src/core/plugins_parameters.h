/**
*
* @file     core/plugins_parameters.h
* @brief    Plugins parameters names
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_PLUGINS_PARAMETERS_H_DEFINED__
#define __CORE_PLUGINS_PARAMETERS_H_DEFINED__

//common includes
#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace Core
    {
      //! @brief Plugins-related parameters namespace
      namespace Plugins
      {
        //! @brief RAW scaner parameters namespace
        namespace Raw
        {
          //@{
          //! @name Scanning step parameter

          //! Default value
          const IntType SCAN_STEP_DEFAULT = 1;
          //! Parameter name
          const Char SCAN_STEP[] =
          {
            'z','x','t','u','n','e','.','c','o','r','e','.','p','l','u','g','i','n','s','.','r','a','w','.','s','c','a','n','_','s','t','e','p','\0'
          };
          //@}

          //@{
          //! @name Minimal scan size parameter

          //! Default value
          const IntType MIN_SIZE_DEFAULT = 128;
          //! Parameter name
          const Char MIN_SIZE[] =
          {
            'z','x','t','u','n','e','.','c','o','r','e','.','p','l','u','g','i','n','s','.','r','a','w','.','m','i','n','_','s','i','z','e','\0'
          };
          //@}
        }

        //! @brief HRIP container parameters namespace
        namespace Hrip
        {
          //! @brief Ignore corrupted blocks
          //! @details 1 if do so
          const Char IGNORE_CORRUPTED[] =
          {
            'z','x','t','u','n','e','.','c','o','r','e','.','p','l','u','g','i','n','s','.','h','r','i','p','.','i','g','n','o','r','e','_','c','o','r','r','u','p','t','e','d','\0'
          };
        }
      }
    }
  }
}
#endif //__CORE_PLUGINS_PARAMETERS_H_DEFINED__
