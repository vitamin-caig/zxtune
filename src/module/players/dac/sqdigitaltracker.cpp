/**
 *
 * @file
 *
 * @brief  SQDigitalTracker chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/sqdigitaltracker.h"

#include "devices/dac/sample_factories.h"
#include "module/players/dac/dac_properties_helper.h"
#include "module/players/dac/dac_simple.h"
#include "module/players/generic_presentation.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"
#include "module/players/track_builder.h"
#include "module/players/tracking.h"

#include "binary/container.h"
#include "parameters/container.h"

#include "make_ptr.h"

#include <array>

namespace Module::SQDigitalTracker
{
  const std::size_t CHANNELS_COUNT = 4;

  const uint_t Z80_FREQ = 3500000;
  const uint_t TICKS_PER_CYCLE = 346;
  const uint_t C_1_STEP = 44;
  const uint_t SAMPLES_FREQ = Z80_FREQ * C_1_STEP / TICKS_PER_CYCLE / 256;

  // per-channel commands and their parameters
  // Presentation: distinct notation commands get distinct values; don't pack variants.
  enum CmdType
  {
    // period
    VOLUME_SLIDE_PERIOD,
    // direction
    VOLUME_SLIDE
  };

  using ModuleData = DAC::SimpleModuleData;

  class DataChannelBuilder : public TrackChannelBuilder<Formats::Chiptune::SQDigitalTracker::ChannelBuilder>
  {
  public:
    void SetVolume(uint_t volume, int_t slideDirection) override
    {
      auto& chan = GetCell();
      chan.SetVolume(volume);
      chan.GetCommands().AddOrUpdate(VOLUME_SLIDE, 0, slideDirection);
    }
    void SetVolumeSlidePeriod(uint_t period) override
    {
      GetCommands().AddOrUpdate(VOLUME_SLIDE_PERIOD, period);
    }
  };

  using LineBuilder = TrackLineBuilder<Formats::Chiptune::SQDigitalTracker::ChannelBuilder, DataChannelBuilder>;
  using PatternsBuilder = TrackPatternsBuilder<Formats::Chiptune::SQDigitalTracker::ChannelBuilder, DataChannelBuilder>;

  class DataBuilder : public TrackDataBuilder<Formats::Chiptune::SQDigitalTracker::Builder, ModuleData, PatternsBuilder>
  {
  public:
    explicit DataBuilder(DAC::PropertiesHelper& props)
      : TrackDataBuilder(props, MakeRWPtr<ModuleData>(CHANNELS_COUNT))
      , Properties(props)
    {
      Properties.SetSamplesFrequency(SAMPLES_FREQ);
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

  private:
    DAC::PropertiesHelper& Properties;
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

  class DataRenderer
  {
  public:
    explicit DataRenderer(const ModuleData& data)
      : Data(data)
    {
      Reset();
    }

    void Reset()
    {
      std::fill(Volumes.begin(), Volumes.end(), VolumeState());
    }

    void SynthesizeData(const TrackState& state, DAC::TrackBuilder& track)
    {
      SynthesizeChannelsData(track);
      if (0 == state.Quirk)
      {
        GetNewLineState(state, track);
      }
    }

  private:
    void SynthesizeChannelsData(DAC::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        auto& vol = Volumes[chan];
        if (vol.Update())
        {
          auto builder = track.GetChannel(chan);
          builder.SetLevelInPercents(100 * vol.Value / 16);
        }
      }
    }

    void GetNewLineState(const TrackState& state, DAC::TrackBuilder& track)
    {
      if (const auto* const line = Data.GetLine(state))
      {
        line->ForEachChannel(
            [&](auto chan, const auto& src) { GetNewChannelState(src, Volumes[chan], track.GetChannel(chan)); });
      }
    }

    static void GetNewChannelState(const Cell& src, VolumeState& vol, DAC::ChannelDataBuilder builder)
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
      for (const auto& cmd : src.GetCommands())
      {
        switch (cmd.Type)
        {
        case VOLUME_SLIDE_PERIOD:
          vol.SlideCounter = vol.SlidePeriod = cmd.Param1;
          break;
        case VOLUME_SLIDE:
          vol.SlideDirection = cmd.Param2;
          break;
        case TEMPO_COMMAND:
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }

  private:
    const ModuleData& Data;
    std::array<VolumeState, CHANNELS_COUNT> Volumes;
  };

  namespace Presentation
  {
    using Delimiter = Module::Presentation::Delimiter;
    using Layout = Module::Presentation::Layout<'0'>;
    using LineNumber = Module::Presentation::Dec<2>;
    // The note/command key defaults to the editor's empty token '---'
    using KeyText = Module::Presentation::Layout<'-'>::Text<3>;

    // The editor shows the accidental as '_' for a natural note (G_3) and '#' for a sharp (G#3)
    constexpr auto NOTES = std::array<std::array<char, 2>, 12>{{
        {'C', '_'},
        {'C', '#'},
        {'D', '_'},
        {'D', '#'},
        {'E', '_'},
        {'F', '_'},
        {'F', '#'},
        {'G', '_'},
        {'G', '#'},
        {'A', '_'},
        {'A', '#'},
        {'B', '_'},
    }};

    // key/note(3) + sample(1) + volume(1) + effect(1)
    struct Field
    {
      KeyText Key;
      Layout::Hex<1> Sample;
      Layout::Hex<1> Volume;
      Layout::Hex<1> Effect;
    };

    struct Channel
    {
      Delimiter Separator;
      Field Cell;
    };

    struct Row
    {
      LineNumber Index;
      Channel Channels[CHANNELS_COUNT];
    };

    void FillCell(const Cell& cell, Field& txt)
    {
      if (const auto* enabled = cell.GetEnabled(); enabled && !*enabled)
      {
        txt.Key = "R__"sv;
        return;
      }
      if (const auto* note = cell.GetNote())
      {
        const auto octave = 1 + *note / NOTES.size();
        if (octave > 9)
        {
          txt.Key = "???"sv;
        }
        else
        {
          const auto& name = NOTES[*note % NOTES.size()];
          txt.Key[0] = name[0];
          txt.Key[1] = name[1];
          txt.Key[2] = static_cast<char>('0' + octave);
        }
      }
      if (const auto* sample = cell.GetSample())
      {
        txt.Sample = *sample;
      }
      if (const auto* volume = cell.GetVolume())
      {
        txt.Volume = *volume;
      }
      for (const auto& cmd : cell.GetCommands())
      {
        switch (cmd.Type)
        {
        case VOLUME_SLIDE_PERIOD:
          // A command parameter is a full byte shown as the low/high nibble pair like a note's sample/volume
          txt.Key = "VOL"sv;
          txt.Sample = cmd.Param1 & 0x0f;
          txt.Volume = cmd.Param1 >> 4;
          break;
        case VOLUME_SLIDE:
          // 1 - fade to silence, 2 - fade to maximum
          if (cmd.Param2)
          {
            txt.Effect = cmd.Param2 < 0 ? 1 : 2;
          }
          break;
        case TEMPO_COMMAND:
          txt.Key = "TMP"sv;
          txt.Sample = cmd.Param1 & 0x0f;
          txt.Volume = cmd.Param1 >> 4;
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }

    void FillLine(const Line& line, Row& row)
    {
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        if (const auto* cell = line.GetChannel(chan))
        {
          FillCell(*cell, row.Channels[chan].Cell);
        }
      }
    }

    String ForPattern(const Pattern& pattern, uint_t /*channels*/)
    {
      String result;
      const auto lines = pattern.GetSize();
      result.reserve(lines * (sizeof(Row) + 1));
      for (uint_t idx = 0; idx != lines; ++idx)
      {
        auto& dst = Module::Presentation::AppendLine<Row>(result);
        dst.Index = idx;
        if (const auto* line = pattern.GetLine(idx))
        {
          FillLine(*line, dst);
        }
      }
      return result;
    }
  }  // namespace Presentation

}  // namespace Module::SQDigitalTracker

namespace Module::DAC
{
  Chiptune::Ptr CreateSQDigitalTrackerChiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    props.SetChannels(SQDigitalTracker::CHANNELS_COUNT);
    SQDigitalTracker::DataBuilder dataBuilder(props);
    if (const auto container = Formats::Chiptune::SQDigitalTracker::Parse(rawData, dataBuilder))
    {
      props.SetSource(*container);
      return CreateTrackingChiptune<SQDigitalTracker::DataRenderer>(dataBuilder.CaptureResult(),
                                                                    &SQDigitalTracker::Presentation::ForPattern);
    }
    return {};
  }
}  // namespace Module::DAC
