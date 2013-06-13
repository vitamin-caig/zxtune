/*
Abstract:
  SQD modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "digital.h"
#include "core/plugins/registrator.h"
#include "core/plugins/utils.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/simple_orderlist.h"
#include "core/plugins/players/tracking.h"
//common includes
#include <error_tools.h>
#include <tools.h>
//library includes
#include <binary/typed_container.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <devices/dac/sample_factories.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/digital/sqdigitaltracker.h>
#include <sound/mixer_factory.h>

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

  using namespace ZXTune;
  using namespace ZXTune::Module;

  typedef DAC::ModuleData ModuleData;

  std::auto_ptr<Formats::Chiptune::SQDigitalTracker::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props);
}

namespace SQDigitalTracker
{
  class DataBuilder : public Formats::Chiptune::SQDigitalTracker::Builder
  {
  public:
    DataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
      : Data(data)
      , Properties(props)
      , Builder(PatternsBuilder::Create<CHANNELS_COUNT>())
    {
      Data->Patterns = Builder.GetPatterns();
      Properties->SetSamplesFreq(SAMPLES_FREQ);
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return *Properties;
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
      Builder.GetChannel().SetEnabled(true);
      Builder.GetChannel().SetNote(note);
    }

    virtual void SetSample(uint_t sample)
    {
      Builder.GetChannel().SetSample(sample);
    }

    virtual void SetVolume(uint_t volume)
    {
      Builder.GetChannel().SetVolume(volume);
    }

    virtual void SetVolumeSlidePeriod(uint_t period)
    {
      Builder.GetChannel().AddCommand(VOLUME_SLIDE_PERIOD, period);
    }

    virtual void SetVolumeSlideDirection(int_t direction)
    {
      Builder.GetChannel().AddCommand(VOLUME_SLIDE, direction);
    }
  private:
    const ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    PatternsBuilder Builder;
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
}

namespace SQD
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'S', 'Q', 'D', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_4DAC | CAP_CONV_RAW;

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
      const ::SQDigitalTracker::ModuleData::RWPtr modData = boost::make_shared< ::SQDigitalTracker::ModuleData>();
      ::SQDigitalTracker::DataBuilder builder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::SQDigitalTracker::Parse(*data, builder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const DAC::Chiptune::Ptr chiptune = boost::make_shared< ::SQDigitalTracker::Chiptune>(modData, properties);
        return DAC::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterSQDSupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateSQDigitalTrackerDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<SQD::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(SQD::ID, decoder->GetDescription(), SQD::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
