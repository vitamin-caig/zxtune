/**
 *
 * @file
 *
 * @brief  AYM parameters helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/aym_parameters.h"

#include "core/plugins/players/ay/freq_tables_internal.h"
#include <devices/aym/chip.h>

#include <core/core_parameters.h>
#include <l10n/api.h>
#include <math/numeric.h>

#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
#include <string_view.h>

#include <cstring>
#include <numeric>
#include <utility>

namespace Module::AYM
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("module_players");

  // duty-cycle related parameter: accumulate letters to bitmask functor
  inline uint_t LetterToMask(uint_t val, const char letter)
  {
    constexpr auto LETTERS = "ABC"sv;
    constexpr uint_t MASKS[] = {
        Devices::AYM::CHANNEL_MASK_A,
        Devices::AYM::CHANNEL_MASK_B,
        Devices::AYM::CHANNEL_MASK_C,
    };
    static_assert(LETTERS.size() == std::size(MASKS), "Invalid layout");
    const auto pos = LETTERS.find(letter);
    if (pos == LETTERS.npos)
    {
      throw MakeFormattedError(THIS_LINE, translate("Invalid duty cycle mask item: '{}'."), String(1, letter));
    }
    return val | MASKS[pos];
  }

  uint_t String2Mask(StringView str)
  {
    return std::accumulate(str.begin(), str.end(), uint_t(0), LetterToMask);
  }

  Devices::AYM::LayoutType String2Layout(StringView str)
  {
    if (str == "ABC")
    {
      return Devices::AYM::LAYOUT_ABC;
    }
    else if (str == "ACB")
    {
      return Devices::AYM::LAYOUT_ACB;
    }
    else if (str == "BAC")
    {
      return Devices::AYM::LAYOUT_BAC;
    }
    else if (str == "BCA")
    {
      return Devices::AYM::LAYOUT_BCA;
    }
    else if (str == "CBA")
    {
      return Devices::AYM::LAYOUT_CBA;
    }
    else if (str == "CAB")
    {
      return Devices::AYM::LAYOUT_CAB;
    }
    else if (str == "MONO")
    {
      return Devices::AYM::LAYOUT_MONO;
    }
    else
    {
      throw MakeFormattedError(THIS_LINE, translate("Invalid layout value ({})."), str);
    }
  }

  class ChipParametersImpl : public Devices::AYM::ChipParameters
  {
  public:
    ChipParametersImpl(uint_t samplerate, Parameters::Accessor::Ptr params)
      : Samplerate(samplerate)
      , Params(std::move(params))
    {}

    uint_t Version() const override
    {
      return Params->Version();
    }

    uint64_t ClockFreq() const override
    {
      using namespace Parameters::ZXTune::Core::AYM;
      const auto val = Parameters::GetInteger(*Params, CLOCKRATE, CLOCKRATE_DEFAULT);
      if (!Math::InRange(val, CLOCKRATE_MIN, CLOCKRATE_MAX))
      {
        throw MakeFormattedError(THIS_LINE, translate("Invalid clock frequency ({})."), val);
      }
      return static_cast<uint64_t>(val);
    }

    uint_t SoundFreq() const override
    {
      return Samplerate;
    }

    Devices::AYM::ChipType Type() const override
    {
      using namespace Parameters::ZXTune::Core::AYM;
      return Parameters::GetInteger<Devices::AYM::ChipType>(*Params, TYPE, TYPE_DEFAULT);
    }

    Devices::AYM::InterpolationType Interpolation() const override
    {
      using namespace Parameters::ZXTune::Core::AYM;
      return Parameters::GetInteger<Devices::AYM::InterpolationType>(*Params, INTERPOLATION, INTERPOLATION_DEFAULT);
    }

    uint_t DutyCycleValue() const override
    {
      using namespace Parameters::ZXTune::Core::AYM;
      const auto val = Parameters::GetInteger(*Params, DUTY_CYCLE, DUTY_CYCLE_DEFAULT);
      // duty cycle in percents should be in range 1..99 inc
      if (!Math::InRange(val, DUTY_CYCLE_MIN, DUTY_CYCLE_MAX))
      {
        throw MakeFormattedError(THIS_LINE, translate("Invalid duty cycle value ({})."), val);
      }
      return static_cast<uint_t>(val);
    }

    uint_t DutyCycleMask() const override
    {
      using namespace Parameters::ZXTune::Core::AYM;
      Parameters::IntType intVal = DUTY_CYCLE_MASK_DEFAULT;
      if (Parameters::FindValue(*Params, DUTY_CYCLE_MASK, intVal))
      {
        return static_cast<uint_t>(intVal);
      }
      if (const auto strVal = Params->FindString(DUTY_CYCLE_MASK))
      {
        return String2Mask(*strVal);
      }
      return 0;
    }

    Devices::AYM::LayoutType Layout() const override
    {
      using namespace Parameters::ZXTune::Core::AYM;
      if (const auto val = Params->FindInteger(LAYOUT))
      {
        if (!Math::InRange<Parameters::IntType>(*val, Devices::AYM::LAYOUT_ABC, Devices::AYM::LAYOUT_LAST))
        {
          throw MakeFormattedError(THIS_LINE, translate("Invalid layout value ({})."), *val);
        }
        return static_cast<Devices::AYM::LayoutType>(*val);
      }
      if (const auto strVal = Params->FindString(LAYOUT))
      {
        return String2Layout(*strVal);
      }
      return Devices::AYM::LAYOUT_ABC;
    }

  private:
    const uint_t Samplerate;
    const Parameters::Accessor::Ptr Params;
  };

  class AYTrackParameters : public TrackParameters
  {
  public:
    explicit AYTrackParameters(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {}

    uint_t Version() const override
    {
      return Params->Version();
    }

    void FreqTable(FrequencyTable& table) const override
    {
      using namespace Parameters::ZXTune::Core::AYM;
      if (const auto newName = Params->FindString(TABLE))
      {
        GetFreqTable(*newName, table);
      }
      else
      {
        const auto newData = Params->FindData(TABLE);
        // frequency table is mandatory!!!
        Require(!!newData);
        // as dump
        if (newData->Size() != table.size() * sizeof(table.front()))
        {
          throw MakeFormattedError(THIS_LINE, translate("Invalid frequency table size ({})."), newData->Size());
        }
        std::memcpy(table.data(), newData->Start(), newData->Size());
      }
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };

  class TSTrackParameters : public TrackParameters
  {
  public:
    TSTrackParameters(Parameters::Accessor::Ptr params, uint_t idx)
      : Params(std::move(params))
      , Index(idx)
    {
      Require(Index <= 1);
    }

    uint_t Version() const override
    {
      return Params->Version();
    }

    void FreqTable(FrequencyTable& table) const override
    {
      if (const auto newName = Params->FindString(Parameters::ZXTune::Core::AYM::TABLE))
      {
        const auto& subName = ExtractMergedValue(*newName);
        GetFreqTable(subName, table);
      }
    }

  private:
    /*
      ('a', 0) => 'a'
      ('a', 1) => 'a'
      ('a/b', 0) => 'a'
      ('a/b', 1) => 'b'
    */
    StringView ExtractMergedValue(StringView val) const
    {
      const auto pos = val.find_first_of('/');
      if (pos != val.npos)
      {
        Require(val.npos == val.find_first_of('/', pos + 1));
        return Index == 0 ? val.substr(0, pos) : val.substr(pos + 1);
      }
      else
      {
        return val;
      }
    }

  private:
    const Parameters::Accessor::Ptr Params;
    const uint_t Index;
  };

  Devices::AYM::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    return MakePtr<ChipParametersImpl>(samplerate, std::move(params));
  }

  TrackParameters::Ptr TrackParameters::Create(Parameters::Accessor::Ptr params)
  {
    return MakePtr<AYTrackParameters>(std::move(params));
  }

  TrackParameters::Ptr TrackParameters::Create(Parameters::Accessor::Ptr params, uint_t idx)
  {
    return MakePtr<TSTrackParameters>(std::move(params), idx);
  }
}  // namespace Module::AYM
