/**
* 
* @file
*
* @brief  DAC-related Module properties helper interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include <module/players/properties_helper.h>

namespace Module
{
  namespace DAC
  {
    class PropertiesHelper : public Module::PropertiesHelper
    {
    public:
      explicit PropertiesHelper(Parameters::Modifier& delegate)
        : Module::PropertiesHelper(delegate)
      {
      }
      
      void SetSamplesFrequency(uint_t freq);
    };
  }
}
