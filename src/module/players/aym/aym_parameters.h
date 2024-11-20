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

#include "core/freq_tables.h"
#include "parameters/accessor.h"

#include "types.h"

#include <memory>

namespace Module::AYM
{
  Devices::AYM::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params);

  class TrackParameters
  {
  public:
    using Ptr = std::shared_ptr<const TrackParameters>;

    virtual ~TrackParameters() = default;

    virtual uint_t Version() const = 0;
    virtual void FreqTable(FrequencyTable& table) const = 0;

    static Ptr Create(Parameters::Accessor::Ptr params);
    static Ptr Create(Parameters::Accessor::Ptr params, uint_t idx);
  };
}  // namespace Module::AYM
