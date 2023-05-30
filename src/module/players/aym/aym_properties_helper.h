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

// library includes
#include <module/players/properties_helper.h>

namespace Module::AYM
{
  class PropertiesHelper : public Module::PropertiesHelper
  {
  public:
    explicit PropertiesHelper(Parameters::Modifier& delegate)
      : Module::PropertiesHelper(delegate)
    {}

    void SetFrequencyTable(StringView freqTable);
    void SetChipType(uint_t type);
    void SetChannelsLayout(uint_t layout);
    void SetChipFrequency(uint64_t freq);
  };
}  // namespace Module::AYM
