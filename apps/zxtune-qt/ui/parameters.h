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

//local includes
#include "app_parameters.h"

namespace Parameters
{
  namespace ZXTuneQT
  {
    namespace UI
    {
      const Char NAMESPACE_NAME[] = {'U','I','\0'};
      const NameType PREFIX = NameType(Parameters::ZXTuneQT::PREFIX) + NAMESPACE_NAME + NAMESPACE_DELIMITER;

      const Char PARAM_GEOMETRY[] =
      {
        'G','e','o','m','e','t','r','y','\0'
      };

      const Char PARAM_LAYOUT[] =
      {
        'L','a','y','o','u','t','\0'
      };

      const Char PARAM_INDEX[] =
      {
        'I','n','d','e','x','\0'
      };

      const Char PARAM_SIZE[] =
      {
        'S','i','z','e','\0'
      };
    }
  }
}
#endif //ZXTUNE_QT_UI_PARAMETERS_H_DEFINED
