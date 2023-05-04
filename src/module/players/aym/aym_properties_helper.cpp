/**
 *
 * @file
 *
 * @brief  AYM-related Module properties helper implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/aym/aym_properties_helper.h"
// core includes
#include <core/core_parameters.h>
// std includes
#include <cassert>

namespace Module
{
  namespace AYM
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
  }  // namespace AYM
}  // namespace Module
