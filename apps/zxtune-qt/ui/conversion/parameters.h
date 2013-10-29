/**
* 
* @file
*
* @brief Export parameters declaration
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

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
