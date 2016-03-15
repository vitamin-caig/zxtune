/**
* 
* @file
*
* @brief  AYM-related Module properties helper interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include <core/plugins/players/properties_helper.h>

namespace Module
{
  namespace AYM
  {
    class PropertiesHelper : public Module::PropertiesHelper
    {
    public:
      explicit PropertiesHelper(Parameters::Modifier& delegate)
        : Module::PropertiesHelper(delegate)
      {
      }
      
      void SetFrequencyTable(const String& freqTable);
      void SetChipType(uint_t type);
      void SetChannelsLayout(uint_t type);
      void SetChipFrequency(uint64_t freq);
    };
  }
}
