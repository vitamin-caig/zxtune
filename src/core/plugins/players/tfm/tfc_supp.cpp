/*
Abstract:
  TFC modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "tfm_base_stream.h"
#include "tfm_plugin.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/streaming.h"
//common includes
#include <contract.h>
#include <tools.h>
//library includes
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/fm/tfc.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace Module
{
namespace TFC
{
  struct FrameData
  {
    uint_t Number;
    Devices::FM::Registers Data;

    FrameData()
      : Number()
    {
    }

    explicit FrameData(uint_t num)
      : Number(num)
    {
    }

    bool operator < (const FrameData& rh) const
    {
      return Number < rh.Number;
    }
  };

  class ChannelData
  {
  public:
    ChannelData()
      : Size()
      , Loop()
    {
    }

    void AddFrame()
    {
      Size++;
    }

    void AddFrames(uint_t count)
    {
      Size += count - 1;
    }

    FrameData& Current()
    {
      Require(Size != 0);
      if (Frames.empty() || Frames.back().Number != Size - 1)
      {
        Frames.push_back(FrameData(Size - 1));
      }
      return Frames.back();
    }

    void SetLoop()
    {
      Loop = Size - 1;
    }

    const Devices::FM::Registers* Get(uint_t row) const
    {
      if (row >= Size)
      {
        row = Loop + (row - Size) % (Size - Loop);
      }
      const FramesList::const_iterator it = std::lower_bound(Frames.begin(), Frames.end(), FrameData(row));
      return it == Frames.end() || it->Number != row
        ? 0
        : &it->Data;
    }

    uint_t GetSize() const
    {
      return Size;
    }
  private:
    uint_t Size;
    typedef std::vector<FrameData> FramesList;
    FramesList Frames;
    uint_t Loop;
  };

  typedef boost::array<ChannelData, 6> ChiptuneData;
  typedef std::auto_ptr<ChiptuneData> ChiptuneDataPtr;

  class ModuleData : public TFM::StreamModel
  {
  public:
    ModuleData(ChiptuneDataPtr data)
      : Data(data)
    {
    }

    virtual uint_t Size() const
    {
      const ChiptuneData& data = *Data;
      const uint_t sizes[6] = {data[0].GetSize(), data[1].GetSize(), data[2].GetSize(),
        data[3].GetSize(), data[4].GetSize(), data[5].GetSize()};
      return *std::max_element(sizes, ArrayEnd(sizes));
    }

    virtual uint_t Loop() const
    {
      return 0;
    }

    virtual void Get(uint_t frameNum, Devices::TFM::Registers& res) const
    {
      Devices::TFM::Registers result;
      const ChiptuneData& data = *Data;
      for (uint_t idx = 0; idx != 6; ++idx)
      {
        const uint_t chip = idx < 3 ? 0 : 1;
        if (const Devices::FM::Registers* regs = data[idx].Get(frameNum))
        {
          for (Devices::FM::Registers::const_iterator it = regs->begin(), lim = regs->end(); it != lim; ++it)
          {
            result.push_back(Devices::TFM::Register(chip, *it));
          }
        }
      }
      res.swap(result);
    }
  private:
    const ChiptuneDataPtr Data;  
  };

  class DataBuilder : public Formats::Chiptune::TFC::Builder
  {
  public:
    explicit DataBuilder(PropertiesBuilder& props)
     : Properties(props)
     , Data(new ChiptuneData())
     , Channel(0)
    {
    }

    virtual void SetVersion(const String& version)
    {
      Properties.SetProgram(Text::TFC_COMPILER_VERSION + version);
    }

    virtual void SetIntFreq(uint_t freq)
    {
      Properties.SetValue(Parameters::ZXTune::Sound::FRAMEDURATION, Time::GetPeriodForFrequency<Time::Microseconds>(freq).Get());
    }

    virtual void SetTitle(const String& title)
    {
      Properties.SetTitle(title);
    }

    virtual void SetAuthor(const String& author)
    {
      Properties.SetAuthor(author);
    }

    virtual void SetComment(const String& comment)
    {
      Properties.SetComment(comment);
    }

    virtual void StartChannel(uint_t idx)
    {
      Channel = idx;
    }

    virtual void StartFrame()
    {
      GetChannel().AddFrame();
    }

    virtual void SetSkip(uint_t count)
    {
      GetChannel().AddFrames(count);
    }

    virtual void SetLoop()
    {
      GetChannel().SetLoop();
    }

    virtual void SetSlide(uint_t slide)
    {
      const uint_t oldFreq = Frequency[Channel];
      const uint_t newFreq = (oldFreq & 0xff00) | ((oldFreq + slide) & 0xff);
      SetFreq(newFreq);
    }

    virtual void SetKeyOff()
    {
      const uint_t key = Channel < 3 ? Channel : Channel + 1;
      SetRegister(0x28, key);
    }

    virtual void SetFreq(uint_t freq)
    {
      Frequency[Channel] = freq;
      const uint_t chan = Channel % 3;
      SetRegister(0xa4 + chan, freq >> 8);
      SetRegister(0xa0 + chan, freq & 0xff);
    }

    virtual void SetRegister(uint_t idx, uint_t val)
    {
      FrameData& frame = GetChannel().Current();
      frame.Data.push_back(Devices::FM::Register(idx, val));
    }

    virtual void SetKeyOn()
    {
      const uint_t key = Channel < 3 ? Channel : Channel + 1;
      SetRegister(0x28, 0xf0 | key);
    }

    TFM::StreamModel::Ptr GetResult() const
    {
      return TFM::StreamModel::Ptr(new ModuleData(Data));
    }
  private:
    ChannelData& GetChannel()
    {
      return (*Data)[Channel];
    }
  private:
    PropertiesBuilder& Properties;
    mutable ChiptuneDataPtr Data;
    uint_t Channel;
    uint_t Frequency[6];
  };

  class Factory : public TFM::Factory
  {
  public:
    virtual TFM::Chiptune::Ptr CreateChiptune(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      DataBuilder dataBuilder(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::TFC::Parse(rawData, dataBuilder))
      {
        const TFM::StreamModel::Ptr data = dataBuilder.GetResult();
        if (data->Size())
        {
          propBuilder.SetSource(*container);
          return TFM::CreateStreamedChiptune(data, propBuilder.GetResult());
        }
      }
      return TFM::Chiptune::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterTFCSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'T', 'F', 'C', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateTFCDecoder();
    const Module::TFM::Factory::Ptr factory = boost::make_shared<Module::TFC::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
