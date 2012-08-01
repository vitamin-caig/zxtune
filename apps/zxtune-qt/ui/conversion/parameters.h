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
        const std::string NAMESPACE_NAME("Export");

        const NameType PREFIX = UI::PREFIX + NAMESPACE_NAME;

        const NameType TYPE = PREFIX + "Type";
      }
    }
  }
}
#endif //ZXTUNE_QT_EXPORT_PARAMETERS_H_DEFINED
