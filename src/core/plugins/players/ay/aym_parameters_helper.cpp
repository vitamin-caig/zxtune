/*
Abstract:
  AYM parameters helper implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "freq_tables_internal.h"
#include "aym_parameters_helper.h"
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
  inline uint8_t LetterToMask(uint8_t val, const Char letter)
  {
    static const Char LETTERS[] = {'A', 'B', 'C', 'N', 'E'};
    static const uint8_t MASKS[] =
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

  void ConvertChipParameters(Parameters::Container& params)
  {
    Parameters::StringType strParam;
    //convert duty cycle mask
    if (params.FindStringValue(Parameters::ZXTune::Core::AYM::DUTY_CYCLE_MASK, strParam))
    {
      const Parameters::IntType numVal = std::accumulate(strParam.begin(), strParam.end(), 0, LetterToMask);
      params.SetIntValue(Parameters::ZXTune::Core::AYM::DUTY_CYCLE_MASK, numVal);
    }
  }

  class ParametersHelperImpl : public ParametersHelper
  {
  public:
    explicit ParametersHelperImpl(TrackParameters::Ptr params)
      : TrackParams(params)
    {
    }

    virtual const Module::FrequencyTable& GetFreqTable() const
    {
      return TrackParams->FreqTable();
    }

    virtual void GetDataChunk(Devices::AYM::DataChunk& dst) const
    {
      dst = Chunk;
    }

  private:
    const TrackParameters::Ptr TrackParams;
    Devices::AYM::DataChunk Chunk;
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

    ParametersHelper::Ptr ParametersHelper::Create(TrackParameters::Ptr params)
    {
      return ParametersHelper::Ptr(new ParametersHelperImpl(params));
    }

    void ChannelBuilder::SetTone(int_t halfTones, int_t offset) const
    {
      const int_t halftone = clamp<int_t>(halfTones, 0, static_cast<int_t>(Table.size()) - 1);
      const uint_t tone = (Table[halftone] + offset) & 0xfff;

      const uint_t reg = Devices::AYM::DataChunk::REG_TONEA_L + 2 * Channel;
      Chunk.Data[reg] = static_cast<uint8_t>(tone & 0xff);
      Chunk.Data[reg + 1] = static_cast<uint8_t>(tone >> 8);
      Chunk.Mask |= (1 << reg) | (1 << (reg + 1));
    }

    void ChannelBuilder::SetLevel(int_t level) const
    {
      const uint_t reg = Devices::AYM::DataChunk::REG_VOLA + Channel;
      Chunk.Data[reg] = static_cast<uint8_t>(clamp<int_t>(level, 0, 15));
      Chunk.Mask |= 1 << reg;
    }

    void ChannelBuilder::DisableTone() const
    {
      Chunk.Data[Devices::AYM::DataChunk::REG_MIXER] |= (Devices::AYM::DataChunk::REG_MASK_TONEA << Channel);
      Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_MIXER;
    }

    void ChannelBuilder::EnableEnvelope() const
    {
      const uint_t reg = Devices::AYM::DataChunk::REG_VOLA + Channel;
      Chunk.Data[reg] |= Devices::AYM::DataChunk::REG_MASK_ENV;
      Chunk.Mask |= 1 << reg;
    }

    void ChannelBuilder::DisableNoise() const
    {
      Chunk.Data[Devices::AYM::DataChunk::REG_MIXER] |= (Devices::AYM::DataChunk::REG_MASK_NOISEA << Channel);
      Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_MIXER;
    }

    void TrackBuilder::SetNoise(uint_t level) const
    {
      Chunk.Data[Devices::AYM::DataChunk::REG_TONEN] = level & 31;
      Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_TONEN;
    }

    void TrackBuilder::SetEnvelopeType(uint_t type) const
    {
      Chunk.Data[Devices::AYM::DataChunk::REG_ENV] = static_cast<uint8_t>(type);
      Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_ENV;
    }

    void TrackBuilder::SetEnvelopeTone(uint_t tone) const
    {
      Chunk.Data[Devices::AYM::DataChunk::REG_TONEE_L] = static_cast<uint8_t>(tone & 0xff);
      Chunk.Data[Devices::AYM::DataChunk::REG_TONEE_H] = static_cast<uint8_t>(tone >> 8);
      Chunk.Mask |= (1 << Devices::AYM::DataChunk::REG_TONEE_L) | (1 << Devices::AYM::DataChunk::REG_TONEE_H);
    }

    int_t TrackBuilder::GetSlidingDifference(int_t halfToneFrom, int_t halfToneTo) const
    {
      const int_t halfFrom = clamp<int_t>(halfToneFrom, 0, static_cast<int_t>(Table.size()) - 1);
      const int_t halfTo = clamp<int_t>(halfToneTo, 0, static_cast<int_t>(Table.size()) - 1);
      const int_t toneFrom = Table[halfFrom];
      const int_t toneTo = Table[halfTo];
      return toneTo - toneFrom;
    }

    void TrackBuilder::SetRawChunk(const Devices::AYM::DataChunk& chunk) const
    {
      std::copy(chunk.Data.begin(), chunk.Data.end(), Chunk.Data.begin());
      Chunk.Mask &= ~Devices::AYM::DataChunk::MASK_ALL_REGISTERS;
      Chunk.Mask |= chunk.Mask & Devices::AYM::DataChunk::MASK_ALL_REGISTERS;
    }
  }
}
