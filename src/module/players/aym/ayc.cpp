/**
 *
 * @file
 *
 * @brief  AYC chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/ayc.h"

#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_stream.h"
#include "module/players/aym/aym_chiptune.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/platforms.h"
#include "module/players/properties_helper.h"

#include "binary/container.h"
#include "parameters/container.h"

#include "contract.h"
#include "make_ptr.h"

namespace Module::AYC
{
  class DataBuilder : public Formats::Chiptune::AYC::Builder
  {
  public:
    DataBuilder()
      : Data(MakePtr<AYM::MutableStreamModel>())
    {}

    void SetFrames(std::size_t count) override
    {
      Data->Resize(count);
    }

    void StartChannel(uint_t idx) override
    {
      Require(idx < Devices::AYM::Registers::TOTAL);
      Register = static_cast<Devices::AYM::Registers::Index>(idx);
      Frame = 0;
    }

    void AddValues(Binary::View values) override
    {
      Require(Register < Devices::AYM::Registers::TOTAL);
      const auto* reg = values.As<uint8_t>();
      for (uint_t idx = 0, lim = values.Size(); idx != lim; ++idx)
      {
        const auto val = reg[idx];
        if (Register != Devices::AYM::Registers::ENV || val != 0xff)
        {
          Data->Frame(Frame++)[Register] = val;
        }
        else
        {
          ++Frame;
        }
      }
    }

    AYM::StreamModel::Ptr CaptureResult() const
    {
      return Data->IsEmpty() ? AYM::StreamModel::Ptr() : AYM::StreamModel::Ptr(Data);
    }

  private:
    Devices::AYM::Registers::Index Register = Devices::AYM::Registers::TOTAL;
    uint_t Frame = 0;
    AYM::MutableStreamModel::Ptr Data;
  };
}  // namespace Module::AYC

namespace Module::AYM
{
  Chiptune::Ptr CreateAYCChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties)
  {
    AYC::DataBuilder dataBuilder;
    if (const auto container = Formats::Chiptune::AYC::Parse(rawData, dataBuilder))
    {
      if (auto data = dataBuilder.CaptureResult())
      {
        PropertiesHelper props(*properties);
        props.SetSource(*container);
        props.SetPlatform(Platforms::AMSTRAD_CPC);
        props.SetChipFrequency(1000000);
        return CreateStreamedChiptune(BASE_FRAME_DURATION, std::move(data), std::move(properties));
      }
    }
    return {};
  }
}  // namespace Module::AYM
