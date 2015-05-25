/**
* 
* @file
*
* @brief  TurboFM Compiled support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "tfm_base_stream.h"
#include "tfm_plugin.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/streaming.h"
//common includes
#include <contract.h>
#include <iterator.h>
//library includes
#include <formats/chiptune/fm/tfc.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/range/algorithm/max_element.hpp>
//text includes
#include <core/text/plugins.h>

namespace Module
{
namespace TFC
{
  class ChannelData
  {
  public:
    ChannelData()
      : Loop()
    {
    }

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
      return RangeIterator<Devices::FM::Registers::const_iterator>(Data.begin() + start, Data.begin() + end);
    }

    std::size_t GetSize() const
    {
      return Offsets.size();
    }
  private:
    std::vector<std::size_t> Offsets;
    Devices::FM::Registers Data;
    std::size_t Loop;
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
      const std::size_t sizes[6] = {data[0].GetSize(), data[1].GetSize(), data[2].GetSize(),
        data[3].GetSize(), data[4].GetSize(), data[5].GetSize()};
      return static_cast<uint_t>(*boost::max_element(sizes));
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
        for (RangeIterator<Devices::FM::Registers::const_iterator> regs = data[idx].Get(frameNum); regs; ++regs)
        {
          result.push_back(Devices::TFM::Register(chip, *regs));
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
      GetChannel().AddRegister(Devices::FM::Register(idx, val));
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
    const PlayerPlugin::Ptr plugin = CreateStreamPlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
