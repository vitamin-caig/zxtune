/*
Abstract:
  COP modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "saa_base.h"
#include "core/plugins/registrator.h"
#include "core/plugins/archives/archive_supp_common.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/simple_orderlist.h"
//common includes
#include <error_tools.h>
#include <range_checker.h>
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/saa/etracker.h>
#include <math/numeric.h>

namespace ETracker
{
  //supported commands and parameters
  enum CmdType
  {
    //no parameters
    EMPTY,
    //yes/no
    SWAPCHANNELS,
    //tone
    ENVELOPE,
    //type
    NOISE
  };

  struct Sample : public Formats::Chiptune::ETracker::Sample
  {
    Sample()
      : Formats::Chiptune::ETracker::Sample()
    {
    }

    Sample(const Formats::Chiptune::ETracker::Sample& rh)
      : Formats::Chiptune::ETracker::Sample(rh)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    const Line& GetLine(uint_t idx) const
    {
      return Lines[idx];
    }
  };

  struct Ornament : public Formats::Chiptune::ETracker::Ornament
  {
    typedef uint_t Line;

    Ornament()
      : Formats::Chiptune::ETracker::Ornament()
    {
    }

    Ornament(const Formats::Chiptune::ETracker::Ornament& rh)
      : Formats::Chiptune::ETracker::Ornament(rh)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    const Line& GetLine(uint_t idx) const
    {
      return Lines[idx];
    }
  };

  using namespace ZXTune;
  using namespace ZXTune::Module;

  typedef SimpleOrderListWithTransposition<Formats::Chiptune::ETracker::PositionEntry> OrderListWithTransposition;

  class ModuleData : public TrackModel
  {
  public:
    typedef boost::shared_ptr<ModuleData> RWPtr;
    typedef boost::shared_ptr<const ModuleData> Ptr;

    ModuleData()
      : InitialTempo()
    {
    }

    virtual uint_t GetInitialTempo() const
    {
      return InitialTempo;
    }

    virtual const OrderList& GetOrder() const
    {
      return *Order;
    }

    virtual const PatternsSet& GetPatterns() const
    {
      return *Patterns;
    }

    uint_t InitialTempo;
    OrderListWithTransposition::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<Sample> Samples;
    SparsedObjectsStorage<Ornament> Ornaments;
  };

  std::auto_ptr<Formats::Chiptune::ETracker::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props);
  SAA::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties);
}

namespace ETracker
{
  class DataBuilder : public Formats::Chiptune::ETracker::Builder
  {
  public:
    DataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
      : Data(data)
      , Properties(props)
      , Builder(PatternsBuilder::Create<SAA::TRACK_CHANNELS>())
    {
      Data->Patterns = Builder.GetPatterns();
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return *Properties;
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::ETracker::Sample& sample)
    {
      Data->Samples.Add(index, Sample(sample));
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::ETracker::Ornament& ornament)
    {
      Data->Ornaments.Add(index, Ornament(ornament));
    }

    virtual void SetPositions(const std::vector<Formats::Chiptune::ETracker::PositionEntry>& positions, uint_t loop)
    {
      Data->Order = boost::make_shared<OrderListWithTransposition>(loop, positions.begin(), positions.end());
    }

    virtual Formats::Chiptune::PatternBuilder& StartPattern(uint_t index)
    {
      Builder.SetPattern(index);
      return Builder;
    }

    virtual void StartChannel(uint_t index)
    {
      Builder.SetChannel(index);
    }

    virtual void SetRest()
    {
      Builder.GetChannel().SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      Builder.GetChannel().SetNote(note);
    }

    virtual void SetSample(uint_t sample)
    {
      Builder.GetChannel().SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Builder.GetChannel().SetOrnament(ornament);
    }

    virtual void SetAttenuation(uint_t vol)
    {
      Builder.GetChannel().SetVolume(15 - vol);
    }

    virtual void SetSwapSampleChannels(bool swapChannels)
    {
      Builder.GetChannel().AddCommand(SWAPCHANNELS, swapChannels);
    }

    virtual void SetEnvelope(uint_t value)
    {
      Builder.GetChannel().AddCommand(ENVELOPE, value);
    }

    virtual void SetNoise(uint_t type)
    {
      Builder.GetChannel().AddCommand(NOISE, type);
    }
  private:
    const ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    PatternsBuilder Builder;
  };

  template<class Object>
  class ObjectLinesIterator
  {
  public:
    ObjectLinesIterator()
      : Obj()
      , Position()
    {
    }

    void Set(const Object* obj)
    {
      Obj = obj;
      Reset();
    }

    void Reset()
    {
      Position = 0;
    }

    void Disable()
    {
      Position = Obj->GetSize();
    }

    const typename Object::Line* GetLine() const
    {
      return Position < Obj->GetSize()
        ? &Obj->GetLine(Position)
        : 0;
    }

    void Next()
    {
      if (++Position == Obj->GetSize())
      {
        Position = Obj->GetLoop();
      }
    }
  private:
    const Object* Obj;
    uint_t Position;
  };

  //enabled state via sample
  struct ChannelState
  {
    ChannelState()
      : Note()
      , Attenuation()
      , SwapSampleChannels()
    {
    }

    uint_t Note;
    ObjectLinesIterator<Sample> SampleIterator;
    ObjectLinesIterator<Ornament> OrnamentIterator;
    uint_t Attenuation;
    bool SwapSampleChannels;
  };

  class DataRenderer : public SAA::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
       : Data(data)
       , Noise()
       , Transposition()
    {
      Reset();
    }

    virtual void Reset()
    {
      static const Sample STUB_SAMPLE;
      static const Ornament STUB_ORNAMENT;
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        ChannelState& dst = PlayerState[chan];
        dst = ChannelState();
        dst.SampleIterator.Set(&STUB_SAMPLE);
        dst.SampleIterator.Disable();
        dst.OrnamentIterator.Set(&STUB_ORNAMENT);
        dst.OrnamentIterator.Reset();
      }
      std::fill(Noise.begin(), Noise.end(), 0);
      Transposition = 0;
    }

    virtual void SynthesizeData(const TrackModelState& state, SAA::TrackBuilder& track)
    {
      if (0 == state.Quirk())
      {
        if (0 == state.Line())
        {
          Transposition = Data->Order->GetTransposition(state.Position());
        }
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(track);
    }
  private:
    void GetNewLineState(const TrackModelState& state, SAA::TrackBuilder& track)
    {
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            SAA::ChannelBuilder channel = track.GetChannel(chan);
            GetNewChannelState(chan, *src, PlayerState[chan], channel);
          }
        }
      }
    }

    void GetNewChannelState(uint_t idx, const Cell& src, ChannelState& dst, SAA::ChannelBuilder& channel)
    {
      static const uint_t ENVELOPE_TABLE[] = 
      {
        0x00, 0x96, 0x9e, 0x9a, 0x086, 0x8e, 0x8a, 0x97, 0x9f, 0x9b, 0x87, 0x8f, 0x8b
      };
      if (const bool* enabled = src.GetEnabled())
      {
        if (!*enabled)
        {
          dst.SampleIterator.Disable();
          dst.OrnamentIterator.Reset();
        }
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
        dst.SampleIterator.Reset();
        dst.OrnamentIterator.Reset();
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleIterator.Set(Data->Samples.Find(*sample));
        dst.OrnamentIterator.Reset();
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentIterator.Set(Data->Ornaments.Find(*ornament));
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Attenuation = 15 - *volume;
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          channel.SetEnvelope(ENVELOPE_TABLE[it->Param1]);
          break;
        case SWAPCHANNELS:
          dst.SwapSampleChannels = it->Param1 != 0;
          break;
        case NOISE:
          Noise[idx >= 3] = it->Param1;
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }

    void SynthesizeChannelsData(SAA::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        SAA::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(chan, channel);
        if (const uint_t noise = Noise[chan >= 3])
        {
          channel.AddNoise(noise);
        }
      }
    }

    class Note
    {
    public:
      Note()
        : Value(0x7ff)
      {
      }

      void Set(uint_t halfTones)
      {
        const uint_t TONES_PER_OCTAVE = 12;
        static const uint_t TONE_TABLE[TONES_PER_OCTAVE] = 
        {
          0x5, 0x21, 0x3c, 0x55, 0x6d, 0x84, 0x99, 0xad, 0xc0, 0xd2, 0xe3, 0xf3
        };
        Value = ((halfTones / TONES_PER_OCTAVE) << 8) + TONE_TABLE[halfTones % TONES_PER_OCTAVE];
      }

      void Add(uint_t delta)
      {
        Value += delta;
      }

      uint_t Octave() const
      {
        return (Value >> 8) & 7;
      }

      uint_t Number() const
      {
        return Value & 0xff;
      }
    private:
      uint_t Value;
    };

    void SynthesizeChannel(uint_t idx, SAA::ChannelBuilder& channel)
    {
      ChannelState& dst = PlayerState[idx];

      Sample::Line curLine;
      if (const Sample::Line* curSampleLine = dst.SampleIterator.GetLine())
      {
        curLine = *curSampleLine;
        dst.SampleIterator.Next();
      }
      uint_t halfTones = dst.Note;
      if (const uint_t* curOrnamentLine = dst.OrnamentIterator.GetLine())
      {
        halfTones += *curOrnamentLine;
        dst.OrnamentIterator.Next();
      }
      //set tone
      Note note;
      if (halfTones != 0x5f)
      {
        halfTones += Transposition;
        if (halfTones >= 0x100)
        {
          halfTones -= 0x60;
        }
        note.Set(halfTones);
      }
      note.Add(curLine.ToneDeviation);
      channel.SetTone(note.Octave(), note.Number());
      if (curLine.ToneEnabled)
      {
        channel.EnableTone();
      }

      //set levels
      if (dst.SwapSampleChannels)
      {
        std::swap(curLine.LeftLevel, curLine.RightLevel);
      }
      channel.SetVolume(int_t(curLine.LeftLevel) - dst.Attenuation, int_t(curLine.RightLevel) - dst.Attenuation);

      //set noise
      if (curLine.NoiseEnabled)
      {
        channel.EnableNoise();
      }
      if (curLine.NoiseEnabled || 0 == idx)
      {
        channel.SetNoise(curLine.NoiseFreq);
      }
    }
  private:
    const ModuleData::Ptr Data;
    boost::array<ChannelState, SAA::TRACK_CHANNELS> PlayerState;
    boost::array<uint_t, 2> Noise;
    uint_t Transposition;
  };

  class Chiptune : public SAA::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, SAA::TRACK_CHANNELS))
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

    virtual SAA::DataIterator::Ptr CreateDataIterator(SAA::TrackParameters::Ptr trackParams) const
    {
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const SAA::DataRenderer::Ptr renderer = boost::make_shared<DataRenderer>(Data);
      return SAA::CreateDataIterator(trackParams, iterator, renderer);
    }
  private:
    const ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };
}

namespace ETracker
{
  std::auto_ptr<Formats::Chiptune::ETracker::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
  {
    return std::auto_ptr<Formats::Chiptune::ETracker::Builder>(new DataBuilder(data, props));
  }

  SAA::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
  {
    return boost::make_shared<Chiptune>(data, properties);
  }
}

namespace COP
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'C', 'O', 'P', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_SAA | CAP_CONV_RAW;

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

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr rawData, std::size_t& usedSize) const
    {
      const ::ETracker::ModuleData::RWPtr modData = boost::make_shared< ::ETracker::ModuleData>();
      const std::auto_ptr<Formats::Chiptune::ETracker::Builder> dataBuilder = ::ETracker::CreateDataBuilder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ETracker::Parse(*rawData, *dataBuilder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const SAA::Chiptune::Ptr chiptune = ::ETracker::CreateChiptune(modData, properties);
        return SAA::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterCOPSupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateETrackerDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<COP::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(COP::ID, decoder->GetDescription(), COP::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
