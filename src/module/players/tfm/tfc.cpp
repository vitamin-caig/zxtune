/**
 *
 * @file
 *
 * @brief  TurboFM Compiled chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/tfm/tfc.h"
#include "module/players/tfm/tfm_base_stream.h"
// common includes
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <formats/chiptune/fm/tfc.h>
#include <module/players/platforms.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <tools/iterators.h>
// std includes
#include <algorithm>
#include <array>

namespace Module::TFC
{
  const auto PROGRAM_PREFIX = "TurboFM Compiler v"sv;

  class ChannelData
  {
  public:
    ChannelData() = default;

    void AddFrame()
    {
      Offsets.push_back(Data.size());
    }

    void AddFrames(uint_t count)
    {
      Offsets.resize(Offsets.size() + count - 1, Data.size());
    }

    void AddRegister(Devices::FM::Register reg)
    {
      Data.push_back(reg);
    }

    void SetLoop()
    {
      Loop = Offsets.size();
    }

    RangeIterator<Devices::FM::Registers::const_iterator> Get(std::size_t row) const
    {
      if (row >= Offsets.size())
      {
        const std::size_t size = Offsets.size();
        row = Loop + (row - size) % (size - Loop);
      }
      const std::size_t start = Offsets[row];
      const std::size_t end = row != Offsets.size() - 1 ? Offsets[row + 1] : Data.size();
      return {Data.begin() + start, Data.begin() + end};
    }

    std::size_t GetSize() const
    {
      return Offsets.size();
    }

  private:
    std::vector<std::size_t> Offsets;
    Devices::FM::Registers Data;
    std::size_t Loop = 0;
  };

  class ModuleData : public TFM::StreamModel
  {
  public:
    using RWPtr = std::shared_ptr<ModuleData>;

    uint_t GetTotalFrames() const override
    {
      return static_cast<uint_t>(std::max({Data[0].GetSize(), Data[1].GetSize(), Data[2].GetSize(), Data[3].GetSize(),
                                           Data[4].GetSize(), Data[5].GetSize()}));
    }

    uint_t GetLoopFrame() const override
    {
      return 0;
    }

    void Get(uint_t frameNum, Devices::TFM::Registers& res) const override
    {
      Devices::TFM::Registers result;
      for (uint_t idx = 0; idx != 6; ++idx)
      {
        const uint_t chip = idx < 3 ? 0 : 1;
        for (RangeIterator<Devices::FM::Registers::const_iterator> regs = Data[idx].Get(frameNum); regs; ++regs)
        {
          result.emplace_back(chip, *regs);
        }
      }
      res.swap(result);
    }

    ChannelData& GetChannel(uint_t channel)
    {
      return Data[channel];
    }

  private:
    std::array<ChannelData, 6> Data;
  };

  class DataBuilder : public Formats::Chiptune::TFC::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Data(MakeRWPtr<ModuleData>())
      , Frequency()
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetVersion(StringView version) override
    {
      Properties.SetProgram(String(PROGRAM_PREFIX).append(version));
    }

    void SetIntFreq(uint_t freq) override
    {
      if (freq)
      {
        FrameDuration = Time::Microseconds::FromFrequency(freq);
      }
    }

    void StartChannel(uint_t idx) override
    {
      Channel = idx;
    }

    void StartFrame() override
    {
      GetChannel().AddFrame();
    }

    void SetSkip(uint_t count) override
    {
      GetChannel().AddFrames(count);
    }

    void SetLoop() override
    {
      GetChannel().SetLoop();
    }

    void SetSlide(uint_t slide) override
    {
      const uint_t oldFreq = Frequency[Channel];
      const uint_t newFreq = (oldFreq & 0xff00) | ((oldFreq + slide) & 0xff);
      SetFreq(newFreq);
    }

    void SetKeyOff() override
    {
      const uint_t key = Channel < 3 ? Channel : Channel + 1;
      SetRegister(0x28, key);
    }

    void SetFreq(uint_t freq) override
    {
      Frequency[Channel] = freq;
      const uint_t chan = Channel % 3;
      SetRegister(0xa4 + chan, freq >> 8);
      SetRegister(0xa0 + chan, freq & 0xff);
    }

    void SetRegister(uint_t idx, uint_t val) override
    {
      GetChannel().AddRegister(Devices::FM::Register(idx, val));
    }

    void SetKeyOn() override
    {
      const uint_t key = Channel < 3 ? Channel : Channel + 1;
      SetRegister(0x28, 0xf0 | key);
    }

    TFM::StreamModel::Ptr CaptureResult() const
    {
      return Data;
    }

    Time::Microseconds GetFrameDuration() const
    {
      return FrameDuration;
    }

  private:
    ChannelData& GetChannel()
    {
      return Data->GetChannel(Channel);
    }

  private:
    PropertiesHelper& Properties;
    MetaProperties Meta;
    const ModuleData::RWPtr Data;
    uint_t Channel = 0;
    std::array<uint_t, 6> Frequency;
    Time::Microseconds FrameDuration = TFM::BASE_FRAME_DURATION;
  };

  class Factory : public TFM::Factory
  {
  public:
    TFM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData,
                                      Parameters::Container::Ptr properties) const override
    {
      PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Formats::Chiptune::TFC::Parse(rawData, dataBuilder))
      {
        auto data = dataBuilder.CaptureResult();
        if (data->GetTotalFrames())
        {
          props.SetSource(*container);
          props.SetPlatform(Platforms::ZX_SPECTRUM);
          return TFM::CreateStreamedChiptune(dataBuilder.GetFrameDuration(), std::move(data), std::move(properties));
        }
      }
      return {};
    }
  };

  TFM::Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::TFC
