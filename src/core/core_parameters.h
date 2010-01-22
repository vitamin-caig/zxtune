/**
*
* @file     core_parameters.h
* @brief    Core parameters names
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_PARAMETERS_H_DEFINED__
#define __CORE_PARAMETERS_H_DEFINED__

#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    //! @brief Core parameters namespace
    namespace Core
    {
      //! @brief Parameters#ZXTune#Core namespace prefix
      const Char PREFIX[] =
      {
        'z','x','t','u','n','e','.','c','o','r','e','.','\0'
      };

      //! @brief AYM-chip related parameters namespace
      namespace AYM
      {
        //! @brief Chip type
        //! @details Integer, every bit is type for specific chip
        const Char TYPE[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','a','y','m','.','t','y','p','e','\0'
        };
        //! @brief Frequency table for ay-based plugins
        //! @details String- table name or dump @see freq_tables.h
        const Char TABLE[] =
        {
          'z','x','t','u','n','e','.','c','o','r','e','.','a','y','m','.','t','a','b','l','e','\0'
        };
      }
    }
  }
}

#endif //__CORE_PARAMETERS_H_DEFINED__
