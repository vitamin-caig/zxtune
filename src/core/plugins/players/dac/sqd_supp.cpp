/*
Abstract:
  SQD modules playback support

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
#include "core/plugins/players/tracking.h"
//common includes
#include <tools.h>
//library includes
#include <devices/dac/sample_factories.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/digital/sqdigitaltracker.h>

namespace Module
{
namespace SQDigitalTracker
{
  const std::size_t CHANNELS_COUNT = 4;

  const uint64_t Z80_FREQ = 3500000;
  const uint_t TICKS_PER_CYCLE = 346;
  const uint_t C_1_STEP = 44;
  const uint_t SAMPLES_FREQ = Z80_FREQ * C_1_STEP / TICKS_PER_CYCLE / 256;
  
  //supported tracking commands
  enum CmdType
  {
    //no parameters
    EMPTY,
    //1 param
    VOLUME_SLIDE_PERIOD,
    //1 param
    VOLUME_SLIDE,
  };

  typedef DAC::ModuleData ModuleData;

  class DataBuilder : public Formats::Chiptune::SQDigitalTracker::Builder
  {
  public:
    explicit DataBuilder(PropertiesBuilder& props)
      : Data(boost::make_shared<ModuleData>())
      , Properties(props)
      , Patterns(PatternsBuilder::Create<CHANNELS_COUNT>())
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

    virtual void SetVolume(uint_t volume)
    {
      Patterns.GetChannel().SetVolume(volume);
    }

    virtual void SetVolumeSlidePeriod(uint_t period)
    {
      Patterns.GetChannel().AddCommand(VOLUME_SLIDE_PERIOD, period);
    }

    virtual void SetVolumeSlideDirection(int_t direction)
    {
      Patterns.GetChannel().AddCommand(VOLUME_SLIDE, direction);
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

  struct VolumeState
  {
    VolumeState()
      : Value(16)
      , SlideDirection(0)
      , SlideCounter(0)
      , SlidePeriod(0)
    {
    }

    int_t Value;
    int_t SlideDirection;
    uint_t SlideCounter;
    uint_t SlidePeriod;

    bool Update()
    {
      if (SlideDirection && !--SlideCounter)
      {
        Value += SlideDirection;
        SlideCounter = SlidePeriod;
        if (-1 == SlideDirection && -1 == Value)
        {
          Value = 0;
        }
        else if (Value == 17)
        {
          Value = 16;
        }
        return true;
      }
      return false;
    }

    void Reset()
    {
      SlideDirection = 0;
      SlideCounter = 0;
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
      std::fill(Volumes.begin(), Volumes.end(), VolumeState());
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
        VolumeState& vol = Volumes[chan];
        if (vol.Update())
        {
          DAC::ChannelDataBuilder builder = track.GetChannel(chan);
          builder.SetLevelInPercents(100 * vol.Value / 16);
        }
      }
    }

    void GetNewLineState(const TrackModelState& state, DAC::TrackBuilder& track)
    {
      std::for_each(Volumes.begin(), Volumes.end(), std::mem_fun_ref(&VolumeState::Reset));
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            DAC::ChannelDataBuilder builder = track.GetChannel(chan);
            GetNewChannelState(*src, Volumes[chan], builder);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, VolumeState& vol, DAC::ChannelDataBuilder& builder)
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
        builder.SetNote(*note);
        builder.SetPosInSample(0);
      }
      if (const uint_t* sample = src.GetSample())
      {
        builder.SetSampleNum(*sample);
        builder.SetPosInSample(0);
      }
      if (const uint_t* volume = src.GetVolume())
      {
        vol.Value = *volume;
        builder.SetLevelInPercents(100 * vol.Value / 16);
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case VOLUME_SLIDE_PERIOD:
          vol.SlideCounter = vol.SlidePeriod = it->Param1;
          break;
        case VOLUME_SLIDE:
          vol.SlideDirection = it->Param1;
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }
  private:
    const ModuleData::Ptr Data;
    boost::array<VolumeState, CHANNELS_COUNT> Volumes;
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
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::SQDigitalTracker::Parse(rawData, dataBuilder))
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
  void RegisterSQDSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'S', 'Q', 'D', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateSQDigitalTrackerDecoder();
    const Module::DAC::Factory::Ptr factory = boost::make_shared<Module::SQDigitalTracker::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
