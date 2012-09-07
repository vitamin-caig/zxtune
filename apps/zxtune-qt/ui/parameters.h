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
      const NameType PREFIX = ZXTuneQT::PREFIX + "UI";

      const NameType LANGUAGE = PREFIX + "Language";

      const std::string PARAM_GEOMETRY("Geometry");
      const std::string PARAM_LAYOUT("Layout");
      const std::string PARAM_VISIBLE("Visible");
      const std::string PARAM_INDEX("Index");
      const std::string PARAM_SIZE("Size");
    }
  }
}
#endif //ZXTUNE_QT_UI_PARAMETERS_H_DEFINED
