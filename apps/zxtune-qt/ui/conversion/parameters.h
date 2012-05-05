/*
Abstract:
  Export parameters definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_EXPORT_PARAMETERS_H_DEFINED
#define ZXTUNE_QT_EXPORT_PARAMETERS_H_DEFINED

//local includes
#include "ui/parameters.h"

namespace Parameters
{
  namespace ZXTuneQT
  {
    namespace UI
    {
      namespace Export
      {
        const Char NAMESPACE_NAME[] = {'E','x','p','o','r','t','\0'};
        const NameType PREFIX = NameType(Parameters::ZXTuneQT::UI::PREFIX) + NAMESPACE_NAME + NAMESPACE_DELIMITER;

        const Char PARAM_TYPE[] =
        {
          'T','y','p','e','\0'
        };

        const NameType TYPE = PREFIX + PARAM_TYPE;
      }
    }
  }
}
#endif //ZXTUNE_QT_EXPORT_PARAMETERS_H_DEFINED
