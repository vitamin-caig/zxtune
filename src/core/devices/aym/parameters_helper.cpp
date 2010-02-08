/*
Abstract:
  AYM parameters helper implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "freq_tables_internal.h"

#include <error_tools.h>
#include <tools.h>
#include <core/error_codes.h>
#include <core/core_parameters.h>
#include <core/devices/aym.h>
#include <core/devices/aym_parameters_helper.h>

#include <numeric>

#include <text/core.h>

#define FILE_TAG 6972CAAF

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::AYM;

  inline uint8_t LetterToMask(uint8_t val, const Char letter)
  {
    static const Char LETTERS[] = {'A', 'B', 'C', 'N', 'E'};
    static const uint8_t MASKS[] = {
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
        TEXT_MODULE_ERROR_INVALID_DUTY_CYCLE_MASK_ITEM, String(1, letter));
    }
    return val | MASKS[pos];
  }

  class ParametersHelperImpl : public ParametersHelper
  {
  public:
    explicit ParametersHelperImpl(const String& freqTableName)
      : Chunk()
    {
      ThrowIfError(Module::GetFreqTable(freqTableName, FreqTable));
    }

    virtual void SetParameters(const Parameters::Map& params)
    {
      Parameters::IntType intParam = 0;
      Parameters::StringType strParam;
      if (Parameters::FindByName(params, Parameters::ZXTune::Core::AYM::TYPE, intParam))
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
      if (Parameters::FindByName(params, Parameters::ZXTune::Core::AYM::TABLE, strParam))
      {
        ThrowIfError(Module::GetFreqTable(strParam, FreqTable));
      }
      else if (const Parameters::DataType* const table = Parameters::FindByName<Parameters::DataType>(params,
        Parameters::ZXTune::Core::AYM::TABLE))
      {
        if (table->size() != FreqTable.size() * sizeof(FreqTable.front()))
        {
          throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
            TEXT_MODULE_ERROR_INVALID_FREQ_TABLE_SIZE, table->size());
        }
        std::memcpy(&FreqTable.front(), &table->front(), table->size());
      }
      if (Parameters::FindByName(params, Parameters::ZXTune::Core::AYM::DUTY_CYCLE, intParam))
      {
        if (intParam < 1 || intParam > 99)
        {
          throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
            TEXT_MODULE_ERROR_INVALID_DUTY_CYCLE, intParam);
        }
        Chunk.Data[DataChunk::PARAM_DUTY_CYCLE] = static_cast<uint8_t>(intParam);
      }
      if (Parameters::FindByName(params, Parameters::ZXTune::Core::AYM::DUTY_CYCLE_MASK, strParam))
      {
        Chunk.Data[DataChunk::PARAM_DUTY_CYCLE_MASK] = std::accumulate(strParam.begin(), strParam.end(), 0, LetterToMask);
      }
      else if (Parameters::FindByName(params, Parameters::ZXTune::Core::AYM::DUTY_CYCLE_MASK, intParam))
      {
        Chunk.Data[DataChunk::PARAM_DUTY_CYCLE_MASK] = static_cast<uint8_t>(intParam);
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
  }
}
