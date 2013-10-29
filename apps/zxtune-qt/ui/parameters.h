/**
* 
* @file
*
* @brief UI parameters definition
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

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
