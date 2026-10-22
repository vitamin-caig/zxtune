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

#include "module/players/platforms.h"

#include "core/parameters.h"

#include "string_view.h"

#include <cassert>

namespace Module::AYM
{
  PropertiesHelper::PropertiesHelper(Parameters::Modifier& delegate)
    : Module::PropertiesHelper(delegate)
  {
    SetPlatform(Platforms::ZX_SPECTRUM);
    SetChipsCount(1);
  }

  void PropertiesHelper::SetChipsCount(uint_t count)
  {
    SetChannels({"A"s, "B"s, "C"s, "N"s, "E"s}, count);
  }

  void PropertiesHelper::SetFrequencyTable(StringView freqTable)
  {
    assert(!freqTable.empty());
    Parameters::Set<Parameters::ZXTune::Core::AYM::Table>(Delegate, freqTable);
  }

  void PropertiesHelper::SetChipType(uint_t type)
  {
    Parameters::Set<Parameters::ZXTune::Core::AYM::Chip>(Delegate, type);
  }

  void PropertiesHelper::SetChannelsLayout(uint_t layout)
  {
    Parameters::Set<Parameters::ZXTune::Core::AYM::Layout>(Delegate, layout);
  }

  void PropertiesHelper::SetChipFrequency(uint_t freq)
  {
    Parameters::Set<Parameters::ZXTune::Core::AYM::Clockrate>(Delegate, freq);
  }
}  // namespace Module::AYM
