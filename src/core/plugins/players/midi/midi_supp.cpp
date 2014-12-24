/**
* 
* @file
*
* @brief  MIDI support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "midi_base_stream.h"
#include "midi_plugin.h"
#include "core/plugins/registrator.h"
#include "core/plugins/utils.h"
#include "core/plugins/players/streaming.h"
//common includes
#include <contract.h>
#include <iterator.h>
//library includes
#include <debug/log.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/midi/midi.h>
#include <sound/sound_parameters.h>
#include <strings/array.h>
//boost includes
#include <boost/algorithm/string/join.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  const Debug::Stream Dbg("Core::MIDI::MIDISupp");
}

namespace Module
{
namespace MIDI
{
  class ChiptuneData
  {
  public:
    ChiptuneData()
    {
    }
    
    void SetMessage(uint_t frame, uint8_t status, uint8_t data1, uint8_t data2 = 0)
    {
      const Chunk chunk = AllocateChunk(3);
      Data.push_back(status);
      Data.push_back(data1);
      Data.push_back(data2);
      AllocateFrame(frame).Add(chunk);
    }
    
    void SetMessage(uint_t frame, const Dump& sysex)
    {
      const Chunk chunk = AllocateChunk(1 + sysex.size());
      Data.push_back(0xf0);
      std::copy(sysex.begin(), sysex.end(), std::back_inserter(Data));
      AllocateFrame(frame).Add(chunk);
    }
    
    void SetMessage(uint_t frame)
    {
      AllocateFrame(frame);
    }
    
    std::size_t GetSize() const
    {
      return Frames.size();
    }
    
    void GetData(uint_t frame, Devices::MIDI::DataChunk& result) const
    {
      Require(frame < Frames.size());
      Frames[frame].GetData(Data, result);
    }
  private:
    struct Chunk
    {
      std::size_t DataOffset;
      std::size_t DataSize;
      
      Chunk()
        : DataOffset()
        , DataSize()
      {
      }
      
      Chunk(std::size_t offset, std::size_t size)
        : DataOffset(offset)
        , DataSize(size)
      {
      }
    };
    
    struct Frame
    {
      std::vector<Chunk> Chunks;
      
      void Add(const Chunk& chunk)
      {
        Chunks.push_back(chunk);
      }
      
      void GetData(const Dump& data, Devices::MIDI::DataChunk& result) const
      {
        Devices::MIDI::DataChunk tmp;
        for (RangeIterator<std::vector<Chunk>::const_iterator> it(Chunks.begin(), Chunks.end()); it; ++it)
        {
          const std::size_t offset = it->DataOffset;
          const std::size_t size = it->DataSize;
          tmp.Data.reserve(tmp.Data.size() + size);
          std::copy(data.begin() + offset, data.begin() + offset + size, std::back_inserter(tmp.Data));
          tmp.Sizes.push_back(size);
        }
        result.Data.swap(tmp.Data);
        result.Sizes.swap(tmp.Sizes);
      }
    };

    Frame& AllocateFrame(uint_t idx)
    {
      if (idx >= Frames.size())
      {
        Frames.resize(idx + 1);
      }
      return Frames[idx];
    }
    
    Chunk AllocateChunk(std::size_t size)
    {
      const std::size_t offset = Data.size();
      Data.reserve(offset + size);
      return Chunk(offset, size);
    }
  private:
    std::vector<Frame> Frames;
    Dump Data;
  };
  
  typedef boost::shared_ptr<const ChiptuneData> ChiptuneDataPtr;

  class ModuleData : public MIDI::StreamModel
  {
  public:
    ModuleData(ChiptuneDataPtr data)
      : Data(data)
    {
    }

    virtual uint_t Size() const
    {
      return Data->GetSize();
    }

    virtual uint_t Loop() const
    {
      return 0;
    }

    virtual void Get(uint_t frameNum, Devices::MIDI::DataChunk& res) const
    {
      Data->GetData(frameNum, res);
    }
  private:
    const ChiptuneDataPtr Data;  
  };

  class DataBuilder : public Formats::Chiptune::MIDI::Builder
  {
  public:
    explicit DataBuilder(PropertiesBuilder& props)
     : Properties(props)
     , Data(new ChiptuneData())
     , Track(0)
     , Frame(0)
     , TicksPerQN(0)
    {
    }

    virtual void SetTimeBase(Formats::Chiptune::MIDI::TimeBase tickPeriod)
    {
      SMPTEPeriod = tickPeriod;
    }

    virtual void SetTicksPerQN(uint_t ticks)
    {
      TicksPerQN = ticks;
    }
    
    virtual void SetTempo(Formats::Chiptune::MIDI::TimeBase qnPeriod)
    {
      if (SMPTEPeriod.Get() == 0)
      {
        if (Frame == 0)
        {
          PPQNPeriod = qnPeriod;
        }
      }
    }
    
    virtual void StartTrack(uint_t idx)
    {
      Track = idx;
      Frame = 0;
      Dbg("Starting track %1%", idx);
    }
    
    virtual void StartEvent(uint32_t ticksDelta)
    {
      Frame += ticksDelta;
    }
    
    virtual void SetMessage(uint8_t status, uint8_t data)
    {
      Data->SetMessage(Frame, status, data);
    }
    
    virtual void SetMessage(uint8_t status, uint8_t data1, uint8_t data2)
    {
      Data->SetMessage(Frame, status, data1, data2);
    }
    
    virtual void SetMessage(const Dump& sysex)
    {
      Data->SetMessage(Frame, sysex);
    }
    
    virtual void SetTitle(const String& title)
    {
      AddString(title, Titles);
    }
    
    virtual void FinishTrack()
    {
      Data->SetMessage(Frame);
      Dbg("Finished track %1% with %2% frames (%3% total)", Track, Frame, Data->GetSize());
    }
    
    Formats::Chiptune::MIDI::TimeBase GetFrameDuration() const
    {
      return SMPTEPeriod.Get() != 0
        ? SMPTEPeriod
        : Formats::Chiptune::MIDI::TimeBase(PPQNPeriod.Get() / TicksPerQN);
    }
    
    MIDI::StreamModel::Ptr GetResult() const
    {
      switch (Titles.size())
      {
      case 1:
        Properties.SetTitle(Titles.front());
        break;
      default:
        Properties.SetComment(boost::algorithm::join(Titles, String(1, '\n')));
      case 0:
        break;
      }
      return MIDI::StreamModel::Ptr(new ModuleData(Data));
    }
  private:
    static void AddString(const String& in, Strings::Array& out)
    {
      const String optimized = OptimizeString(in);
      if (!optimized.empty())
      {
        out.push_back(optimized);
      }
    }
  private:
    PropertiesBuilder& Properties;
    boost::shared_ptr<ChiptuneData> Data;
    uint_t Track;
    uint_t Frame;
    Strings::Array Titles;
    Formats::Chiptune::MIDI::TimeBase SMPTEPeriod;
    Formats::Chiptune::MIDI::TimeBase PPQNPeriod;
    uint_t TicksPerQN;
  };

  class MIDIFactory : public Factory
  {
  public:
    virtual MIDI::Chiptune::Ptr CreateChiptune(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      DataBuilder dataBuilder(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::MIDI::Parse(rawData, dataBuilder))
      {
        const MIDI::StreamModel::Ptr data = dataBuilder.GetResult();
        if (data->Size())
        {
          propBuilder.SetSource(*container);
          propBuilder.SetValue(Parameters::ZXTune::Sound::FRAMEDURATION, Time::Microseconds(dataBuilder.GetFrameDuration()).Get());
          return MIDI::CreateStreamedChiptune(data, propBuilder.GetResult());
        }
      }
      return MIDI::Chiptune::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterMIDISupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'M', 'I', 'D', 'I', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateMIDIDecoder();
    const Module::MIDI::Factory::Ptr factory = boost::make_shared<Module::MIDI::MIDIFactory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
