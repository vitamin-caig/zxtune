/**
 *
 * @file
 *
 * @brief  TurboFM Dump chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/tfm/tfd.h"
#include "module/players/tfm/tfm_base_stream.h"
// common includes
#include <make_ptr.h>
// library includes
#include <formats/chiptune/fm/tfd.h>
#include <module/players/platforms.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>

namespace Module::TFD
{
  class ModuleData : public TFM::StreamModel
  {
  public:
    typedef std::shared_ptr<ModuleData> RWPtr;

    ModuleData() = default;

    uint_t GetTotalFrames() const override
    {
      return static_cast<uint_t>(Offsets.size() - 1);
    }

    uint_t GetLoopFrame() const override
    {
      return LoopPos;
    }

    void Get(uint_t frameNum, Devices::TFM::Registers& res) const override
    {
      const std::size_t start = Offsets[frameNum];
      const std::size_t end = Offsets[frameNum + 1];
      res.assign(Data.begin() + start, Data.begin() + end);
    }

    void Append(std::size_t count)
    {
      Offsets.resize(Offsets.size() + count, Data.size());
    }

    void AddRegister(const Devices::TFM::Register& reg)
    {
      if (!Offsets.empty())
      {
        Data.push_back(reg);
      }
    }

    void SetLoop()
    {
      if (!Offsets.empty())
      {
        LoopPos = static_cast<uint_t>(Offsets.size() - 1);
      }
    }

  private:
    uint_t LoopPos = 0;
    Devices::TFM::Registers Data;
    std::vector<std::size_t> Offsets;
  };

  class DataBuilder : public Formats::Chiptune::TFD::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Data(MakeRWPtr<ModuleData>())
      , Chip(0)
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void BeginFrames(uint_t count) override
    {
      Chip = 0;
      Data->Append(count);
    }

    void SelectChip(uint_t idx) override
    {
      Chip = idx;
    }

    void SetLoop() override
    {
      Data->SetLoop();
    }

    void SetRegister(uint_t idx, uint_t val) override
    {
      Data->AddRegister(Devices::TFM::Register(Chip, idx, val));
    }

    TFM::StreamModel::Ptr CaptureResult() const
    {
      return std::move(Data);
    }

  private:
    PropertiesHelper& Properties;
    MetaProperties Meta;
    const ModuleData::RWPtr Data;
    uint_t Chip;
  };

  class Factory : public TFM::Factory
  {
  public:
    TFM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData,
                                      Parameters::Container::Ptr properties) const override
    {
      PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Formats::Chiptune::TFD::Parse(rawData, dataBuilder))
      {
        auto data = dataBuilder.CaptureResult();
        if (data->GetTotalFrames())
        {
          props.SetSource(*container);
          props.SetPlatform(Platforms::ZX_SPECTRUM);
          return TFM::CreateStreamedChiptune(TFM::BASE_FRAME_DURATION, std::move(data), std::move(properties));
        }
      }
      return {};
    }
  };

  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::TFD
