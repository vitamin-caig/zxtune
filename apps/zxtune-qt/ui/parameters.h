/*
Abstract:
  UI parameters definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_PARAMETERS_H_DEFINED
#define ZXTUNE_QT_UI_PARAMETERS_H_DEFINED

//common includes
#include <parameters.h>

namespace Parameters
{
  namespace ZXTuneQT
  {
    namespace UI
    {
      //@{
      //! @name Geometry blob

      //! Parameter name
      const Char GEOMETRY[] =
      {
        'z','x','t','u','n','e','-','q','t','.','u','i','.','g','e','o','m','e','t','r','y','\0'
      };
      //@}

      //@{
      //! @name Layout blob

      //! Parameter name
      const Char LAYOUT[] =
      {
        'z','x','t','u','n','e','-','q','t','.','u','i','.','l','a','y','o','u','t','\0'
      };
    }
  }
}
#endif //ZXTUNE_QT_UI_PARAMETERS_H_DEFINED
