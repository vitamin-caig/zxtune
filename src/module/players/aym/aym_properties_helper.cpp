/**
 *
 * @file
 *
 * @brief  AYM-related Module properties helper implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/aym_properties_helper.h"

#include <core/core_parameters.h>

#include <string_view.h>

#include <cassert>

namespace Module::AYM
{
  void PropertiesHelper::SetFrequencyTable(StringView freqTable)
  {
    assert(!freqTable.empty());
    Delegate.SetValue(Parameters::ZXTune::Core::AYM::TABLE, freqTable);
  }

  void PropertiesHelper::SetChipType(uint_t type)
  {
    Delegate.SetValue(Parameters::ZXTune::Core::AYM::TYPE, type);
  }

  void PropertiesHelper::SetChannelsLayout(uint_t layout)
  {
    Delegate.SetValue(Parameters::ZXTune::Core::AYM::LAYOUT, layout);
  }

  void PropertiesHelper::SetChipFrequency(uint64_t freq)
  {
    Delegate.SetValue(Parameters::ZXTune::Core::AYM::CLOCKRATE, freq);
  }
}  // namespace Module::AYM
