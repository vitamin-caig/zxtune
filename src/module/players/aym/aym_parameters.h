/**
 *
 * @file
 *
 * @brief  AYM parameters helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "devices/aym/chip.h"
#include "module/players/aym/freq_tables.h"

#include "parameters/accessor.h"
#include "parameters/identifiers_set.h"

namespace Module::AYM
{
  struct ChipParameters
  {
    static const Parameters::IdentifiersSet ALL;

    static Devices::AYM::ChipParameters::Ptr Create(uint_t samplerate, Parameters::Accessor::Ptr params);
  };

  class TrackParameters
  {
  public:
    using Ptr = std::shared_ptr<const TrackParameters>;

    virtual ~TrackParameters() = default;

    virtual uint_t Version() const = 0;
    virtual void FreqTable(FrequencyTable& table) const = 0;

    static const Parameters::IdentifiersSet ALL;

    static Ptr Create(Parameters::Accessor::Ptr params);
    static Ptr Create(Parameters::Accessor::Ptr params, uint_t idx);
  };
}  // namespace Module::AYM
