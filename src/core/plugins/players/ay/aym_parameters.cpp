/*
Abstract:
  AYM parameters helper implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "aym_parameters.h"
#include "freq_tables_internal.h"
//common includes
#include <error_tools.h>
#include <tools.h>
//library includes
#include <core/error_codes.h>
#include <core/core_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
//std includes
#include <numeric>
//text includes
#include <core/text/core.h>

#define FILE_TAG 6972CAAF

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::AYM;

  //duty-cycle related parameter: accumulate letters to bitmask functor
  inline uint_t LetterToMask(uint_t val, const Char letter)
  {
    static const Char LETTERS[] = {'A', 'B', 'C', 'N', 'E'};
    static const uint_t MASKS[] =
    {
      Devices::AYM::DataChunk::DUTY_CYCLE_MASK_A,
      Devices::AYM::DataChunk::DUTY_CYCLE_MASK_B,
      Devices::AYM::DataChunk::DUTY_CYCLE_MASK_C,
      Devices::AYM::DataChunk::DUTY_CYCLE_MASK_N,
      Devices::AYM::DataChunk::DUTY_CYCLE_MASK_E
    };
    BOOST_STATIC_ASSERT(sizeof(LETTERS) / sizeof(*LETTERS) == sizeof(MASKS) / sizeof(*MASKS));
    const std::size_t pos = std::find(LETTERS, ArrayEnd(LETTERS), letter) - LETTERS;
    if (pos == ArraySize(LETTERS))
    {
      throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
        Text::MODULE_ERROR_INVALID_DUTY_CYCLE_MASK_ITEM, String(1, letter));
    }
    return val | MASKS[pos];
  }

  uint_t String2Mask(const String& str)
  {
    return std::accumulate(str.begin(), str.end(), uint_t(0), LetterToMask);
  }

  LayoutType String2Layout(const String& str)
  {
    if (str == Text::MODULE_LAYOUT_ABC)
    {
      return LAYOUT_ABC;
    }
    else if (str == Text::MODULE_LAYOUT_ACB)
    {
      return LAYOUT_ACB;
    }
    else if (str == Text::MODULE_LAYOUT_BAC)
    {
      return LAYOUT_BAC;
    }
    else if (str == Text::MODULE_LAYOUT_BCA)
    {
      return LAYOUT_BCA;
    }
    else if (str == Text::MODULE_LAYOUT_CBA)
    {
      return LAYOUT_CBA;
    }
    else if (str == Text::MODULE_LAYOUT_CAB)
    {
      return LAYOUT_CAB;
    }
    else
    {
      throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
        Text::MODULE_ERROR_INVALID_LAYOUT, str);
    }
  }

  class ChipParametersImpl : public Devices::AYM::ChipParameters
  {
  public:
    explicit ChipParametersImpl(Parameters::Accessor::Ptr params)
      : Params(params)
      , SoundParams(Sound::RenderParameters::Create(params))
    {
    }

    virtual uint_t ClockFreq() const
    {
      return SoundParams->ClockFreq();
    }

    virtual uint_t SoundFreq() const
    {
      return SoundParams->SoundFreq();
    }

    virtual bool IsYM() const
    {
      Parameters::IntType intVal = 0;
      Params->FindIntValue(Parameters::ZXTune::Core::AYM::TYPE, intVal);
      return intVal != 0;
    }

    virtual bool Interpolate() const
    {
      Parameters::IntType intVal = 0;
      Params->FindIntValue(Parameters::ZXTune::Core::AYM::INTERPOLATION, intVal);
      return intVal != 0;
    }

    virtual uint_t DutyCycleValue() const
    {
      Parameters::IntType intVal = 50;
      const bool found = Params->FindIntValue(Parameters::ZXTune::Core::AYM::DUTY_CYCLE, intVal);
      //duty cycle in percents should be in range 1..99 inc
      if (found && (intVal < 1 || intVal > 99))
      {
        throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
          Text::MODULE_ERROR_INVALID_DUTY_CYCLE, intVal);
      }
      return static_cast<uint_t>(intVal);
    }

    virtual uint_t DutyCycleMask() const
    {
      Parameters::StringType strVal;
      if (Params->FindStringValue(Parameters::ZXTune::Core::AYM::DUTY_CYCLE_MASK, strVal))
      {
        return String2Mask(strVal);
      }
      Parameters::IntType intVal = 0;
      Params->FindIntValue(Parameters::ZXTune::Core::AYM::DUTY_CYCLE_MASK, intVal);
      return static_cast<uint_t>(intVal);
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const Sound::RenderParameters::Ptr SoundParams;
  };

  class TrackParametersImpl : public TrackParameters
  {
  public:
    explicit TrackParametersImpl(Parameters::Accessor::Ptr params)
      : Params(params)
    {
    }

    virtual const Module::FrequencyTable& FreqTable() const
    {
      UpdateTable();
      return Table;
    }

    virtual LayoutType Layout() const
    {
      Parameters::StringType strVal;
      if (Params->FindStringValue(Parameters::ZXTune::Core::AYM::LAYOUT, strVal))
      {
        return String2Layout(strVal);
      }
      Parameters::IntType intVal = LAYOUT_ABC;
      const bool found = Params->FindIntValue(Parameters::ZXTune::Core::AYM::LAYOUT, intVal);
      if (found && (intVal < static_cast<int_t>(LAYOUT_ABC) ||
            intVal >= static_cast<int_t>(LAYOUT_LAST)))
      {
        throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
          Text::MODULE_ERROR_INVALID_LAYOUT, intVal);
      }
      return static_cast<LayoutType>(intVal);
    }
  private:
    void UpdateTable() const
    {
      Parameters::StringType newName;
      if (Params->FindStringValue(Parameters::ZXTune::Core::AYM::TABLE, newName))
      {
        if (newName != TableName)
        {
          ThrowIfError(Module::GetFreqTable(newName, Table));
        }
        return;
      }
      Parameters::DataType newData;
      if (Params->FindDataValue(Parameters::ZXTune::Core::AYM::TABLE, newData))
      {
        // as dump
        if (newData.size() != Table.size() * sizeof(Table.front()))
        {
          throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
            Text::MODULE_ERROR_INVALID_FREQ_TABLE_SIZE, newData.size());
        }
        std::memcpy(&Table.front(), &newData.front(), newData.size());
      }
      else
      {
        assert(!"Empty frequency table specified");
      }
    }
  private:
    const Parameters::Accessor::Ptr Params;
    mutable String TableName;
    mutable Module::FrequencyTable Table;
  };
}

namespace ZXTune
{
  namespace AYM
  {
    Devices::AYM::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params)
    {
      return boost::make_shared<ChipParametersImpl>(params);
    }

    TrackParameters::Ptr TrackParameters::Create(Parameters::Accessor::Ptr params)
    {
      return boost::make_shared<TrackParametersImpl>(params);
    }
  }
}
