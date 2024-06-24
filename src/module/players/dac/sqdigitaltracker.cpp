/**
 *
 * @file
 *
 * @brief  SQDigitalTracker chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/dac/sqdigitaltracker.h"
#include "module/players/dac/dac_properties_helper.h"
#include "module/players/dac/dac_simple.h"
// library includes
#include <devices/dac/sample_factories.h>
#include <formats/chiptune/digital/sqdigitaltracker.h>
#include <module/players/platforms.h>
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>
#include <module/players/tracking.h>
// std includes
#include <array>

namespace Module::SQDigitalTracker
{
  const std::size_t CHANNELS_COUNT = 4;

  const uint64_t Z80_FREQ = 3500000;
  const uint_t TICKS_PER_CYCLE = 346;
  const uint_t C_1_STEP = 44;
  const uint_t SAMPLES_FREQ = Z80_FREQ * C_1_STEP / TICKS_PER_CYCLE / 256;

  // supported tracking commands
  enum CmdType
  {
    // no parameters
    EMPTY,
    // 1 param
    VOLUME_SLIDE_PERIOD,
    // 1 param
    VOLUME_SLIDE,
  };

  using ModuleData = DAC::SimpleModuleData;

  class DataBuilder : public Formats::Chiptune::SQDigitalTracker::Builder
  {
  public:
    explicit DataBuilder(DAC::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Patterns(PatternsBuilder::Create<CHANNELS_COUNT>())
      , Data(MakeRWPtr<ModuleData>(CHANNELS_COUNT))
    {
      Properties.SetSamplesFrequency(SAMPLES_FREQ);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, std::size_t loop, Binary::View sample) override
    {
      Data->Samples.Add(index, Devices::DAC::CreateU8Sample(sample, loop));
    }

    void SetPositions(Formats::Chiptune::SQDigitalTracker::Positions positions) override
    {
      Data->Order = MakePtr<SimpleOrderList>(positions.Loop, std::move(positions.Lines));
    }

    Formats::Chiptune::PatternBuilder& StartPattern(uint_t index) override
    {
      Patterns.SetPattern(index);
      return Patterns;
    }

    void StartChannel(uint_t index) override
    {
      Patterns.SetChannel(index);
    }

    void SetRest() override
    {
      Patterns.GetChannel().SetEnabled(false);
    }

    void SetNote(uint_t note) override
    {
      Patterns.GetChannel().SetEnabled(true);
      Patterns.GetChannel().SetNote(note);
    }

    void SetSample(uint_t sample) override
    {
      Patterns.GetChannel().SetSample(sample);
    }

    void SetVolume(uint_t volume) override
    {
      Patterns.GetChannel().SetVolume(volume);
    }

    void SetVolumeSlidePeriod(uint_t period) override
    {
      Patterns.GetChannel().AddCommand(VOLUME_SLIDE_PERIOD, period);
    }

    void SetVolumeSlideDirection(int_t direction) override
    {
      Patterns.GetChannel().AddCommand(VOLUME_SLIDE, direction);
    }

    ModuleData::Ptr CaptureResult()
    {
      Data->Patterns = Patterns.CaptureResult();
      return std::move(Data);
    }

  private:
    DAC::PropertiesHelper& Properties;
    MetaProperties Meta;
    PatternsBuilder Patterns;
    ModuleData::RWPtr Data;
  };

  struct VolumeState
  {
    VolumeState() = default;

    int_t Value = 16;
    int_t SlideDirection = 0;
    uint_t SlideCounter = 0;
    uint_t SlidePeriod = 0;

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
  };

  class DataRenderer : public DAC::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(std::move(data))
    {
      Reset();
    }

    void Reset() override
    {
      std::fill(Volumes.begin(), Volumes.end(), VolumeState());
    }

    void SynthesizeData(const TrackModelState& state, DAC::TrackBuilder& track) override
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
      if (const auto* const line = state.LineObject())
      {
        for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
        {
          if (const auto* const src = line->GetChannel(chan))
          {
            DAC::ChannelDataBuilder builder = track.GetChannel(chan);
            GetNewChannelState(*src, Volumes[chan], builder);
          }
        }
      }
    }

    static void GetNewChannelState(const Cell& src, VolumeState& vol, DAC::ChannelDataBuilder& builder)
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
    std::array<VolumeState, CHANNELS_COUNT> Volumes;
  };

  class Chiptune : public DAC::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(std::move(data))
      , Properties(std::move(properties))
    {}

    TrackModel::Ptr GetTrackModel() const override
    {
      return Data;
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    DAC::DataIterator::Ptr CreateDataIterator() const override
    {
      auto iterator = CreateTrackStateIterator(GetFrameDuration(), Data);
      auto renderer = MakePtr<DataRenderer>(Data);
      return DAC::CreateDataIterator(std::move(iterator), std::move(renderer));
    }

    void GetSamples(Devices::DAC::Chip& chip) const override
    {
      Data->SetupSamples(chip);
    }

  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
  };

  class Factory : public DAC::Factory
  {
  public:
    DAC::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData,
                                      Parameters::Container::Ptr properties) const override
    {
      DAC::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Formats::Chiptune::SQDigitalTracker::Parse(rawData, dataBuilder))
      {
        props.SetSource(*container);
        props.SetPlatform(Platforms::ZX_SPECTRUM);
        return MakePtr<Chiptune>(dataBuilder.CaptureResult(), std::move(properties));
      }
      else
      {
        return {};
      }
    }
  };

  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::SQDigitalTracker
