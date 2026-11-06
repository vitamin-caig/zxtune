/**
 *
 * @file
 *
 * @brief  ChipTracker chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/chiptracker.h"

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

namespace Module::ChipTracker
{
  const std::size_t CHANNELS_COUNT = 4;

  const uint_t Z80_FREQ = 3500000;
  // one cycle is 4 outputs
  const uint_t OUTS_PER_CYCLE = 4;
  const uint_t TICKS_PER_CYCLE = 890;
  const uint_t C_1_STEP = 72;
  // Z80_FREQ * (C_1_STEP / 256) / (TICKS_PER_CYCLE / OUTS_PER_CYCLE)
  const uint_t SAMPLES_FREQ = Z80_FREQ * C_1_STEP * OUTS_PER_CYCLE / TICKS_PER_CYCLE / 256;

  inline int_t StepToHz(int_t step)
  {
    // C-1 frequency is 32.7Hz
    // step * 32.7 / c-1_step
    return step * 3270 / int_t(C_1_STEP * 100);
  }

  // per-channel commands and their parameters
  // Presentation: distinct notation commands get distinct values; don't pack variants.
  enum CmdType
  {
    // offset in bytes
    SAMPLE_OFFSET,
    // step
    SLIDE,
    // pattern terminator
    STOP
  };

  using ModuleData = DAC::SimpleModuleData;

  class DataChannelBuilder : public TrackChannelBuilder<Formats::Chiptune::ChipTracker::ChannelBuilder>
  {
  public:
    void SetSlide(int_t step) override
    {
      GetCommands().AddOrUpdate(SLIDE, 0, step);
    }
    void SetSampleOffset(uint_t offset) override
    {
      GetCommands().AddOrUpdate(SAMPLE_OFFSET, 0, offset);
    }
    void SetStop() override
    {
      GetCommands().AddOrUpdate(STOP, 0, 0);
    }
  };

  using LineBuilder = TrackLineBuilder<Formats::Chiptune::ChipTracker::ChannelBuilder, DataChannelBuilder>;
  using PatternsBuilder = TrackPatternsBuilder<Formats::Chiptune::ChipTracker::ChannelBuilder, DataChannelBuilder>;

  class DataBuilder : public TrackDataBuilder<Formats::Chiptune::ChipTracker::Builder, ModuleData, PatternsBuilder>
  {
  public:
    explicit DataBuilder(DAC::PropertiesHelper& props)
      : TrackDataBuilder(props, MakeRWPtr<ModuleData>(CHANNELS_COUNT))
      , Properties(props)
    {
      Properties.SetSamplesFrequency(SAMPLES_FREQ);
    }

    virtual void SetVersion(uint_t major, uint_t minor)
    {
      Properties.SetVersion(major, minor);
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, std::size_t loop, Binary::View sample) override
    {
      Data->Samples.Add(index, Devices::DAC::CreateU8Sample(sample, loop));
    }

    void SetPositions(Formats::Chiptune::ChipTracker::Positions positions) override
    {
      Data->Order = MakePtr<SimpleOrderList>(positions.Loop, std::move(positions.Lines));
    }

  private:
    DAC::PropertiesHelper& Properties;
  };

  struct GlissData
  {
    GlissData() = default;
    int_t Sliding = 0;
    int_t Glissade = 0;

    void Reset()
    {
      Sliding = Glissade = 0;
    }

    bool Update()
    {
      Sliding += Glissade;
      return Glissade != 0;
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
      std::fill(Gliss.begin(), Gliss.end(), GlissData());
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
        auto& gliss = Gliss[chan];
        if (gliss.Update())
        {
          auto builder = track.GetChannel(chan);
          builder.SetFreqSlideHz(StepToHz(gliss.Sliding));
        }
      }
    }

    void GetNewLineState(const TrackState& state, DAC::TrackBuilder& track)
    {
      Gliss.fill(GlissData());
      if (const auto* const line = Data.GetLine(state))
      {
        line->ForEachChannel(
            [&](auto chan, const auto& src) { GetNewChannelState(src, Gliss[chan], track.GetChannel(chan)); });
      }
    };

    static void GetNewChannelState(const Cell& src, GlissData& gliss, DAC::ChannelDataBuilder builder)
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
      builder.SetFreqSlideHz(0);
      gliss.Reset();
      for (const auto& cmd : src.GetCommands())
      {
        switch (cmd.Type)
        {
        case SAMPLE_OFFSET:
          builder.SetPosInSample(cmd.Param2);
          break;
        case SLIDE:
          gliss.Glissade = cmd.Param2;
          break;
        case STOP:
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
    std::array<GlissData, CHANNELS_COUNT> Gliss;
  };

  namespace Presentation
  {
    using Delimiter = Module::Presentation::Delimiter;
    using Layout = Module::Presentation::Layout<'0'>;

    // note/rest(3) + command(1) + parameter(1) + sample(1)
    struct Field
    {
      Module::Presentation::NoteAndOctave Note;
      char Command = ' ';
      Layout::Hex<1> Param;
      Layout::Hex<1> Sample;
    };

    struct Channel
    {
      Delimiter Separator;
      Field Cell;
    };

    struct Row
    {
      char Hash = '#';
      Layout::Hex<2> Index;
      Channel Channels[CHANNELS_COUNT];
    };

    void FillCell(const Cell& cell, Field& txt)
    {
      txt.Note.Set(&cell);
      if (const auto* sample = cell.GetSample())
      {
        txt.Sample = *sample;
      }
      for (const auto& cmd : cell.GetCommands())
      {
        switch (cmd.Type)
        {
        case SAMPLE_OFFSET:
          txt.Param = cmd.Param2 / 512;
          break;
        case SLIDE:
          if (cmd.Param2)
          {
            txt.Command = cmd.Param2 < 0 ? '-' : '+';
            txt.Param = (cmd.Param2 < 0 ? -cmd.Param2 : cmd.Param2) / 2;
          }
          break;
        case STOP:
          txt.Command = 'E';
          break;
        case TEMPO_COMMAND:
          txt.Command = 'T';
          txt.Param = cmd.Param1;
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

}  // namespace Module::ChipTracker

namespace Module::DAC
{
  Chiptune::Ptr CreateChipTrackerChiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    props.SetChannels(ChipTracker::CHANNELS_COUNT);
    ChipTracker::DataBuilder dataBuilder(props);
    if (const auto container = Formats::Chiptune::ChipTracker::Parse(rawData, dataBuilder))
    {
      props.SetSource(*container);
      return CreateTrackingChiptune<ChipTracker::DataRenderer>(dataBuilder.CaptureResult(),
                                                               &ChipTracker::Presentation::ForPattern);
    }
    return {};
  }
}  // namespace Module::DAC
