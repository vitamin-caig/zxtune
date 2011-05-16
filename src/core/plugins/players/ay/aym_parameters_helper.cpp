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
      DataChunk::DUTY_CYCLE_MASK_A,
      DataChunk::DUTY_CYCLE_MASK_B,
      DataChunk::DUTY_CYCLE_MASK_C,
      DataChunk::DUTY_CYCLE_MASK_N,
      DataChunk::DUTY_CYCLE_MASK_E
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

  uint_t String2Layout(const String& str)
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

  class ParametersHelperImpl : public ParametersHelper
  {
  public:
    explicit ParametersHelperImpl(const String& freqTableName)
      : Chunk()
    {
      ThrowIfError(Module::GetFreqTable(freqTableName, FreqTable));
    }

    virtual void SetParameters(const Parameters::Accessor& params)
    {
      Parameters::IntType intParam = 0;
      Parameters::StringType strParam;
      Parameters::DataType dataParam;
      // chip type parameter
      if (params.FindIntValue(Parameters::ZXTune::Core::AYM::TYPE, intParam))
      {
        if (intParam)
        {
          Chunk.Mask |= AYM::DataChunk::YM_CHIP;
        }
        else
        {
          Chunk.Mask &= ~AYM::DataChunk::YM_CHIP;
        }
      }
      // interpolation parameter
      if (params.FindIntValue(Parameters::ZXTune::Core::AYM::INTERPOLATION, intParam))
      {
        if (intParam)
        {
          Chunk.Mask |= AYM::DataChunk::INTERPOLATE;
        }
        else
        {
          Chunk.Mask &= ~AYM::DataChunk::INTERPOLATE;
        }
      }
      // freqtable parameter
      if (params.FindStringValue(Parameters::ZXTune::Core::AYM::TABLE, strParam))
      {
        // as name
        ThrowIfError(Module::GetFreqTable(strParam, FreqTable));
      }
      else if (params.FindDataValue(Parameters::ZXTune::Core::AYM::TABLE, dataParam))
      {
        // as dump
        if (dataParam.size() != FreqTable.size() * sizeof(FreqTable.front()))
        {
          throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
            Text::MODULE_ERROR_INVALID_FREQ_TABLE_SIZE, dataParam.size());
        }
        std::memcpy(&FreqTable.front(), &dataParam.front(), dataParam.size());
      }
      // duty cycle value parameter
      if (params.FindIntValue(Parameters::ZXTune::Core::AYM::DUTY_CYCLE, intParam))
      {
        //duty cycle in percents should be in range 1..99 inc
        if (intParam < 1 || intParam > 99)
        {
          throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
            Text::MODULE_ERROR_INVALID_DUTY_CYCLE, intParam);
        }
        Chunk.Data[DataChunk::PARAM_DUTY_CYCLE] = static_cast<uint8_t>(intParam);
      }
      // duty cycle mask parameter
      if (params.FindStringValue(Parameters::ZXTune::Core::AYM::DUTY_CYCLE_MASK, strParam))
      {
        // as string mask
        Chunk.Data[DataChunk::PARAM_DUTY_CYCLE_MASK] = std::accumulate(strParam.begin(), strParam.end(), 0, LetterToMask);
      }
      else if (params.FindIntValue(Parameters::ZXTune::Core::AYM::DUTY_CYCLE_MASK, intParam))
      {
        // as integer mask
        Chunk.Data[DataChunk::PARAM_DUTY_CYCLE_MASK] = static_cast<uint8_t>(intParam);
      }
      // layout parameter
      if (params.FindStringValue(Parameters::ZXTune::Core::AYM::LAYOUT, strParam))
      {
        // as string mask
        Chunk.Data[DataChunk::PARAM_LAYOUT] = String2Layout(strParam);
      }
      else if (params.FindIntValue(Parameters::ZXTune::Core::AYM::LAYOUT, intParam))
      {
        if (intParam < static_cast<int_t>(LAYOUT_ABC) ||
            intParam >= static_cast<int_t>(LAYOUT_LAST))
        {
          throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
            Text::MODULE_ERROR_INVALID_LAYOUT, intParam);
        }
        Chunk.Data[DataChunk::PARAM_LAYOUT] = static_cast<uint8_t>(intParam);
      }
    }

    virtual const Module::FrequencyTable& GetFreqTable() const
    {
      return FreqTable;
    }

    virtual void GetDataChunk(DataChunk& dst) const
    {
      dst = Chunk;
    }

  private:
    Module::FrequencyTable FreqTable;
    DataChunk Chunk;
  };
}

namespace ZXTune
{
  namespace AYM
  {
    ParametersHelper::Ptr ParametersHelper::Create(const String& defaultFreqTable)
    {
      return ParametersHelper::Ptr(new ParametersHelperImpl(defaultFreqTable));
    }

    void ChannelBuilder::SetTone(int_t halfTones, int_t offset) const
    {
      const int_t halftone = clamp<int_t>(halfTones, 0, static_cast<int_t>(Table.size()) - 1);
      const uint_t tone = (Table[halftone] + offset) & 0xfff;

      const uint_t reg = AYM::DataChunk::REG_TONEA_L + 2 * Channel;
      Chunk.Data[reg] = static_cast<uint8_t>(tone & 0xff);
      Chunk.Data[reg + 1] = static_cast<uint8_t>(tone >> 8);
      Chunk.Mask |= (1 << reg) | (1 << (reg + 1));
    }

    void ChannelBuilder::SetLevel(int_t level) const
    {
      const uint_t reg = AYM::DataChunk::REG_VOLA + Channel;
      Chunk.Data[reg] = static_cast<uint8_t>(clamp<int_t>(level, 0, 15));
      Chunk.Mask |= 1 << reg;
    }

    void ChannelBuilder::DisableTone() const
    {
      Chunk.Data[AYM::DataChunk::REG_MIXER] |= (AYM::DataChunk::REG_MASK_TONEA << Channel);
      Chunk.Mask |= 1 << AYM::DataChunk::REG_MIXER;
    }

    void ChannelBuilder::EnableEnvelope() const
    {
      const uint_t reg = AYM::DataChunk::REG_VOLA + Channel;
      Chunk.Data[reg] |= AYM::DataChunk::REG_MASK_ENV;
      Chunk.Mask |= 1 << reg;
    }

    void ChannelBuilder::DisableNoise() const
    {
      Chunk.Data[AYM::DataChunk::REG_MIXER] |= (AYM::DataChunk::REG_MASK_NOISEA << Channel);
      Chunk.Mask |= 1 << AYM::DataChunk::REG_MIXER;
    }

    void TrackBuilder::SetNoise(uint_t level) const
    {
      Chunk.Data[AYM::DataChunk::REG_TONEN] = level & 31;
      Chunk.Mask |= 1 << AYM::DataChunk::REG_TONEN;
    }

    void TrackBuilder::SetEnvelopeType(uint_t type) const
    {
      Chunk.Data[AYM::DataChunk::REG_ENV] = static_cast<uint8_t>(type);
      Chunk.Mask |= 1 << AYM::DataChunk::REG_ENV;
    }

    void TrackBuilder::SetEnvelopeTone(uint_t tone) const
    {
      Chunk.Data[AYM::DataChunk::REG_TONEE_L] = static_cast<uint8_t>(tone & 0xff);
      Chunk.Data[AYM::DataChunk::REG_TONEE_H] = static_cast<uint8_t>(tone >> 8);
      Chunk.Mask |= (1 << AYM::DataChunk::REG_TONEE_L) | (1 << AYM::DataChunk::REG_TONEE_H);
    }

    int_t TrackBuilder::GetSlidingDifference(int_t halfToneFrom, int_t halfToneTo) const
    {
      const int_t halfFrom = clamp<int_t>(halfToneFrom, 0, static_cast<int_t>(Table.size()) - 1);
      const int_t halfTo = clamp<int_t>(halfToneTo, 0, static_cast<int_t>(Table.size()) - 1);
      const int_t toneFrom = Table[halfFrom];
      const int_t toneTo = Table[halfTo];
      return toneTo - toneFrom;
    }

    void TrackBuilder::SetRawChunk(const AYM::DataChunk& chunk) const
    {
      std::copy(chunk.Data.begin(), chunk.Data.end(), Chunk.Data.begin());
      Chunk.Mask &= ~AYM::DataChunk::MASK_ALL_REGISTERS;
      Chunk.Mask |= chunk.Mask & AYM::DataChunk::MASK_ALL_REGISTERS;
    }
  }
}
