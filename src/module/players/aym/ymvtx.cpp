/**
 *
 * @file
 *
 * @brief  YM/VTX chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/ym.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_stream.h"
#include "module/players/aym/aym_chiptune.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/properties_meta.h"

#include "binary/container.h"
#include "core/core_parameters.h"
#include "parameters/container.h"
#include "strings/conversion.h"

#include "make_ptr.h"
#include "string_view.h"

#include <utility>

namespace Module::YMVTX
{
  Devices::AYM::LayoutType VtxMode2AymLayout(uint_t mode)
  {
    using namespace Devices::AYM;
    switch (mode)
    {
    case 0:
      return LAYOUT_MONO;
    case 1:
      return LAYOUT_ABC;
    case 2:
      return LAYOUT_ACB;
    case 3:
      return LAYOUT_BAC;
    case 4:
      return LAYOUT_BCA;
    case 5:
      return LAYOUT_CAB;
    case 6:
      return LAYOUT_CBA;
    default:
      assert(!"Invalid VTX layout");
      return LAYOUT_ABC;
    }
  }

  class DataBuilder : public Formats::Chiptune::YM::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Data(MakePtr<AYM::MutableStreamModel>())
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetVersion(StringView version) override
    {
      Properties.SetVersion(version);
    }

    void SetChipType(bool ym) override
    {
      Properties.SetChipType(ym ? Parameters::ZXTune::Core::AYM::TYPE_YM : Parameters::ZXTune::Core::AYM::TYPE_AY);
    }

    void SetStereoMode(uint_t mode) override
    {
      Properties.SetChannelsLayout(VtxMode2AymLayout(mode));
    }

    void SetLoop(uint_t loop) override
    {
      Data->SetLoop(loop);
    }

    void SetDigitalSample(uint_t /*idx*/, Binary::View /*data*/) override
    {
      // TODO:
    }

    void SetClockrate(uint64_t freq) override
    {
      Properties.SetChipFrequency(freq);
    }

    void SetIntFreq(uint_t freq) override
    {
      if (freq)
      {
        FrameDuration = Time::Microseconds::FromFrequency(freq);
      }
    }

    void SetYear(uint_t year) override
    {
      if (year)
      {
        Properties.SetDate(Strings::ConvertFrom(year));
      }
    }

    void SetEditor(StringView editor) override
    {
      Properties.SetProgram(editor);
    }

    void AddData(Binary::View registers) override
    {
      auto& data = Data->AddFrame();
      const uint_t availRegs = std::min<uint_t>(registers.Size(), Devices::AYM::Registers::ENV + 1);
      const auto* regs = registers.As<uint8_t>();
      for (uint_t reg = 0, mask = 1; reg != availRegs; ++reg, mask <<= 1)
      {
        const uint8_t val = regs[reg];
        if (reg != Devices::AYM::Registers::ENV || val != 0xff)
        {
          data[static_cast<Devices::AYM::Registers::Index>(reg)] = val;
        }
      }
    }

    AYM::StreamModel::Ptr CaptureResult() const
    {
      return Data->IsEmpty() ? AYM::StreamModel::Ptr() : AYM::StreamModel::Ptr(Data);
    }

    Time::Microseconds GetFrameDuration() const
    {
      return FrameDuration;
    }

  private:
    AYM::PropertiesHelper& Properties;
    MetaProperties Meta;
    AYM::MutableStreamModel::Ptr Data;
    Time::Microseconds FrameDuration = AYM::BASE_FRAME_DURATION;
  };
}  // namespace Module::YMVTX

namespace Module::AYM
{
  Chiptune::Ptr CreateModule(const Binary::Container& rawData, Parameters::Container::Ptr properties,
                             Formats::Chiptune::YM::Parser parse)
  {
    PropertiesHelper props(*properties);
    YMVTX::DataBuilder dataBuilder(props);
    if (const auto container = parse(rawData, dataBuilder))
    {
      if (auto data = dataBuilder.CaptureResult())
      {
        // TODO: detect platform by intfreq and clockrate
        props.SetSource(*container);
        return CreateStreamedChiptune(dataBuilder.GetFrameDuration(), std::move(data), std::move(properties));
      }
    }
    return {};
  }

  Chiptune::Ptr CreateYMChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties)
  {
    return CreateModule(rawData, std::move(properties), Formats::Chiptune::YM::Parse);
  }

  Chiptune::Ptr CreateYMPackedChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties)
  {
    return CreateModule(rawData, std::move(properties), Formats::Chiptune::YM::ParsePacked);
  }

  Chiptune::Ptr CreateVTXChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties)
  {
    return CreateModule(rawData, std::move(properties), Formats::Chiptune::YM::ParseVTX);
  }
}  // namespace Module::AYM
