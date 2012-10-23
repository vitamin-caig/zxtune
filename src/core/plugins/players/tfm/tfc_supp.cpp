/*
Abstract:
  TFC modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "tfm_base.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/streaming.h"
//common includes
#include <contract.h>
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/tfc.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  struct FrameData
  {
    uint_t Number;
    Devices::FM::DataChunk::Registers Data;

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

    const Devices::FM::DataChunk::Registers* Get(uint_t row) const
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

  class ModuleData
  {
  public:
    typedef boost::shared_ptr<const ModuleData> Ptr;

    ModuleData(ChiptuneDataPtr data)
      : Data(data)
    {
    }

    uint_t Count() const
    {
      const ChiptuneData& data = *Data;
      const uint_t sizes[6] = {data[0].GetSize(), data[1].GetSize(), data[2].GetSize(),
        data[3].GetSize(), data[4].GetSize(), data[5].GetSize()};
      return *std::max_element(sizes, ArrayEnd(sizes));
    }

    Devices::TFM::DataChunk Get(std::size_t frameNum) const
    {
      const ChiptuneData& data = *Data;
      Devices::TFM::DataChunk result;
      for (uint_t idx = 0; idx != 6; ++idx)
      {
        const uint_t chip = idx < 3 ? 0 : 1;
        if (const Devices::FM::DataChunk::Registers* regs = data[idx].Get(frameNum))
        {
          std::copy(regs->begin(), regs->end(), std::back_inserter(result.Data[chip]));
        }
      }
      return result;
    }
  private:
    const ChiptuneDataPtr Data;  
  };

  class Builder : public Formats::Chiptune::TFC::Builder
  {
  public:
    explicit Builder(ModuleProperties& props)
     : Props(props)
     , Data(new ChiptuneData())
     , Channel(0)
    {
    }

    virtual void SetVersion(const String& version)
    {
      Props.SetProgram(Text::TFC_COMPILER_VERSION + version);
    }

    virtual void SetIntFreq(uint_t freq)
    {
      Props.GetInternalContainer()->SetValue(Parameters::ZXTune::Sound::FRAMEDURATION, Time::GetPeriodForFrequency<Time::Microseconds>(freq).Get());
    }

    virtual void SetTitle(const String& title)
    {
      Props.SetTitle(title);
    }

    virtual void SetAuthor(const String& author)
    {
      Props.SetAuthor(author);
    }

    virtual void SetComment(const String& comment)
    {
      Props.SetComment(comment);
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
      frame.Data.push_back(Devices::FM::DataChunk::Register(idx, val));
    }

    virtual void SetKeyOn()
    {
      const uint_t key = Channel < 3 ? Channel : Channel + 1;
      SetRegister(0x28, 0xf0 | key);
    }

    ModuleData::Ptr GetResult() const
    {
      return ModuleData::Ptr(new ModuleData(Data));
    }
  private:
    ChannelData& GetChannel()
    {
      return (*Data)[Channel];
    }
  private:
    ModuleProperties& Props;
    mutable ChiptuneDataPtr Data;
    uint_t Channel;
    uint_t Frequency[6];
  };
}

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class DataIterator : public TFM::DataIterator
  {
  public:
    DataIterator(StateIterator::Ptr delegate, ModuleData::Ptr data)
      : Delegate(delegate)
      , State(Delegate->GetStateObserver())
      , Data(data)
    {
      UpdateCurrentState();
    }

    virtual void Reset()
    {
      CurrentChunk = Devices::TFM::DataChunk();
      Delegate->Reset();
      UpdateCurrentState();
    }

    virtual bool IsValid() const
    {
      return Delegate->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      Delegate->NextFrame(looped);
      UpdateCurrentState();
    }

    virtual TrackState::Ptr GetStateObserver() const
    {
      return State;
    }

    virtual Devices::TFM::DataChunk GetData() const
    {
      Devices::TFM::DataChunk res;
      std::swap(res, CurrentChunk);
      return res;
    }
  private:
    void UpdateCurrentState()
    {
      if (Delegate->IsValid())
      {
        const uint_t frameNum = State->Frame();
        const Devices::TFM::DataChunk& inChunk = Data->Get(frameNum);
        for (uint_t idx = 0; idx != Devices::TFM::CHIPS; ++idx)
        {
          UpdateChunk(CurrentChunk.Data[idx], inChunk.Data[idx]);
        }
      }
    }

    static void UpdateChunk(Devices::FM::DataChunk::Registers& dst, const Devices::FM::DataChunk::Registers& src)
    {
      std::copy(src.begin(), src.end(), std::back_inserter(dst));
    }
  private:
    const StateIterator::Ptr Delegate;
    const TrackState::Ptr State;
    const ModuleData::Ptr Data;
    mutable Devices::TFM::DataChunk CurrentChunk;
  };

  class Chiptune : public TFM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateStreamInfo(Data->Count(), Devices::TFM::CHANNELS, 0))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual ModuleProperties::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual TFM::DataIterator::Ptr CreateDataIterator(TFM::TrackParameters::Ptr /*trackParams*/) const
    {
      const StateIterator::Ptr iter = CreateStreamStateIterator(Info);
      return boost::make_shared<DataIterator>(iter, Data);
    }
  private:
    const ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };
}

namespace TFC
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'T', 'F', 'C', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_FM | CAP_CONV_RAW;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::Decoder::Ptr decoder)
      : Decoder(decoder)
    {
    }

    virtual bool Check(const Binary::Container& data) const
    {
      return Decoder->Check(data);
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr data, std::size_t& usedSize) const
    {
      Builder builder(*properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::TFC::Parse(*data, builder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const ModuleData::Ptr data = builder.GetResult();
        if (data->Count())
        {
          const TFM::Chiptune::Ptr chiptune = boost::make_shared<Chiptune>(data, properties);
          return TFM::CreateHolder(chiptune);
        }
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterTFCSupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateTFCDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<TFC::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(TFC::ID, decoder->GetDescription(), TFC::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
