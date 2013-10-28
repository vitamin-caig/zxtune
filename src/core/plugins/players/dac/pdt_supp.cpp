/*
Abstract:
  PDT modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dac_plugin.h"
#include "digital.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/simple_orderlist.h"
#include "core/plugins/players/simple_ornament.h"
#include "core/plugins/players/tracking.h"
//library includes
#include <core/plugin_attrs.h>
#include <devices/dac/sample_factories.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/digital/prodigitracker.h>

namespace Module
{
namespace ProDigiTracker
{
  const uint_t CHANNELS_COUNT = 4;

  const uint64_t Z80_FREQ = 3500000;
  const uint_t TICKS_PER_CYCLE = 374;
  const uint_t C_1_STEP = 46;
  const uint_t SAMPLES_FREQ = Z80_FREQ * C_1_STEP / TICKS_PER_CYCLE / 256;
  
  typedef SimpleOrnament Ornament;

  class ModuleData : public DAC::ModuleData
  {
  public:
    typedef boost::shared_ptr<const ModuleData> Ptr;

    SparsedObjectsStorage<Ornament> Ornaments;
  };

  class DataBuilder : public Formats::Chiptune::ProDigiTracker::Builder
  {
  public:
    explicit DataBuilder(PropertiesBuilder& props)
      : Data(boost::make_shared<ModuleData>())
      , Properties(props)
      , Patterns(PatternsBuilder::Create<ProDigiTracker::CHANNELS_COUNT>())
    {
      Data->Patterns = Patterns.GetResult();
      Properties.SetSamplesFreq(SAMPLES_FREQ);
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return Properties;
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr sample)
    {
      Data->Samples.Add(index, Devices::DAC::CreateU8Sample(sample, loop));
    }

    virtual void SetOrnament(uint_t index, std::size_t loop, const std::vector<int_t>& ornament)
    {
      Data->Ornaments.Add(index, Ornament(loop, ornament.begin(), ornament.end()));
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      Data->Order = boost::make_shared<SimpleOrderList>(loop, positions.begin(), positions.end());
    }

    virtual Formats::Chiptune::PatternBuilder& StartPattern(uint_t index)
    {
      Patterns.SetPattern(index);
      return Patterns;
    }

    virtual void StartChannel(uint_t index)
    {
      Patterns.SetChannel(index);
    }

    virtual void SetRest()
    {
      Patterns.GetChannel().SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      Patterns.GetChannel().SetEnabled(true);
      Patterns.GetChannel().SetNote(note);
    }

    virtual void SetSample(uint_t sample)
    {
      Patterns.GetChannel().SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Patterns.GetChannel().SetOrnament(ornament);
    }

    ModuleData::Ptr GetResult() const
    {
      return Data;
    }
  private:
    const boost::shared_ptr<ModuleData> Data;
    PropertiesBuilder& Properties;
    PatternsBuilder Patterns;
  };

  struct OrnamentState
  {
    OrnamentState() : Object(), Position()
    {
    }
    const Ornament* Object;
    std::size_t Position;

    int_t GetOffset() const
    {
      return Object ? Object->GetLine(Position) : 0;
    }

    void Update()
    {
      if (Object && Position++ >= Object->GetSize())
      {
        Position = Object->GetLoop();
      }
    }
  };

  class DataRenderer : public DAC::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(data)
    {
      Reset();
    }

    virtual void Reset()
    {
      std::fill(Ornaments.begin(), Ornaments.end(), OrnamentState());
    }

    virtual void SynthesizeData(const TrackModelState& state, DAC::TrackBuilder& track)
    {
      SynthesizeChannelsData(track);
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
    }  
  private:
    void SynthesizeChannelsData(DAC::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        OrnamentState& ornament = Ornaments[chan];
        ornament.Update();
        DAC::ChannelDataBuilder builder = track.GetChannel(chan);
        builder.SetNoteSlide(ornament.GetOffset());
      }
    }

    void GetNewLineState(const TrackModelState& state, DAC::TrackBuilder& track)
    {
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            DAC::ChannelDataBuilder builder = track.GetChannel(chan);
            GetNewChannelState(*src, Ornaments[chan], builder);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, OrnamentState& ornamentState, DAC::ChannelDataBuilder& builder)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        builder.SetEnabled(*enabled);
        if (!*enabled)
        {
          builder.SetPosInSample(0);
        }
      }

      if (const uint_t* note = src.GetNote())
      {
        if (const uint_t* ornament = src.GetOrnament())
        {
          ornamentState.Object = Data->Ornaments.Find(*ornament);
          ornamentState.Position = 0;
          builder.SetNoteSlide(ornamentState.GetOffset());
        }
        if (const uint_t* sample = src.GetSample())
        {
          builder.SetSampleNum(*sample);
        }
        builder.SetNote(*note);
        builder.SetPosInSample(0);
      }
    }
  private:
    const ModuleData::Ptr Data;
    boost::array<OrnamentState, CHANNELS_COUNT> Ornaments;
  };

  class Chiptune : public DAC::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, CHANNELS_COUNT))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual DAC::DataIterator::Ptr CreateDataIterator() const
    {
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const DAC::DataRenderer::Ptr renderer = boost::make_shared<DataRenderer>(Data);
      return DAC::CreateDataIterator(iterator, renderer);
    }

    virtual void GetSamples(Devices::DAC::Chip::Ptr chip) const
    {
      for (uint_t idx = 0, lim = Data->Samples.Size(); idx != lim; ++idx)
      {
        chip->SetSample(idx, Data->Samples.Get(idx));
      }
    }
  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
    const Information::Ptr Info;
  };

  class Factory : public DAC::Factory
  {
  public:
    virtual DAC::Chiptune::Ptr CreateChiptune(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      DataBuilder dataBuilder(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ProDigiTracker::Parse(rawData, dataBuilder))
      {
        propBuilder.SetSource(*container);
        return boost::make_shared<Chiptune>(dataBuilder.GetResult(), propBuilder.GetResult());
      }
      else
      {
        return DAC::Chiptune::Ptr();
      }
    }
  };
}
}

namespace ZXTune
{
  void RegisterPDTSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'P', 'D', 'T', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProDigiTrackerDecoder();
    const Module::DAC::Factory::Ptr factory = boost::make_shared<Module::ProDigiTracker::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
