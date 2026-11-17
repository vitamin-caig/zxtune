/**
 *
 * @file
 *
 * @brief  ExtremeTracker1 chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/extremetracker1.h"

#include "devices/dac/sample_factories.h"
#include "module/players/dac/dac_properties_helper.h"
#include "module/players/dac/dac_simple.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"
#include "module/players/track_builder.h"
#include "module/players/tracking.h"

#include "binary/container.h"
#include "parameters/container.h"

#include "make_ptr.h"

#include <array>

namespace Module::ExtremeTracker1
{
  const std::size_t CHANNELS_COUNT = 4;

  const uint_t C_1_STEP_GLISS = 0x50;

  // per-channel commands and their parameters
  // Presentation: distinct notation commands get distinct values; don't pack variants.
  enum CmdType
  {
    // glissade
    GLISS
  };

  inline int_t StepToHz(int_t step)
  {
    // C-1 frequency is 32.7Hz
    // step * 32.7 / c-1_step
    return step * 3270 / int_t(C_1_STEP_GLISS * 100);
  }

  using ModuleData = DAC::SimpleModuleData;

  class DataChannelBuilder : public TrackChannelBuilder<Formats::Chiptune::ExtremeTracker1::ChannelBuilder>
  {
  public:
    void SetGliss(int_t gliss) override
    {
      GetCommands().AddOrUpdate(GLISS, 0, gliss);
    }
  };

  using LineBuilder = TrackLineBuilder<Formats::Chiptune::ExtremeTracker1::ChannelBuilder, DataChannelBuilder>;
  using PatternsBuilder = TrackPatternsBuilder<Formats::Chiptune::ExtremeTracker1::ChannelBuilder, DataChannelBuilder>;

  class DataBuilder : public TrackDataBuilder<Formats::Chiptune::ExtremeTracker1::Builder, ModuleData, PatternsBuilder>
  {
  public:
    explicit DataBuilder(DAC::PropertiesHelper& props)
      : TrackDataBuilder(props, MakeRWPtr<ModuleData>(CHANNELS_COUNT))
      , Properties(props)
    {}

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSamplesFrequency(uint_t freq) override
    {
      Properties.SetSamplesFrequency(freq);
    }

    void SetSample(uint_t index, std::size_t loop, Binary::View sample) override
    {
      Data->Samples.Add(index, Devices::DAC::CreateU8Sample(sample, loop));
    }

    void SetPositions(Formats::Chiptune::ExtremeTracker1::Positions positions) override
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
      if (const auto* const line = Data.GetLine(state))
      {
        line->ForEachChannel(
            [&](auto chan, const auto& src) { GetNewChannelState(src, Gliss[chan], track.GetChannel(chan)); });
      }
    }

    static void GetNewChannelState(const Cell& src, GlissData& gliss, DAC::ChannelDataBuilder builder)
    {
      if (src.HasData())
      {
        builder.SetLevelInPercents(100);
      }
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
      }
      if (const uint_t* sample = src.GetSample())
      {
        builder.SetSampleNum(*sample);
        builder.SetPosInSample(0);
        builder.SetFreqSlideHz(0);
        gliss.Sliding = 0;
      }
      if (const uint_t* volume = src.GetVolume())
      {
        const uint_t level = *volume;
        builder.SetLevelInPercents(100 * level / 16);
      }
      for (const auto& cmd : src.GetCommands())
      {
        switch (cmd.Type)
        {
        case GLISS:
          gliss.Glissade = cmd.Param2;
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

}  // namespace Module::ExtremeTracker1

namespace Module::DAC
{
  Chiptune::Ptr CreateExtremeTracker1Chiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    props.SetChannels(ExtremeTracker1::CHANNELS_COUNT);
    ExtremeTracker1::DataBuilder dataBuilder(props);
    if (const auto container = Formats::Chiptune::ExtremeTracker1::Parse(rawData, dataBuilder))
    {
      props.SetSource(*container);
      return CreateTrackingChiptune<ExtremeTracker1::DataRenderer>(dataBuilder.CaptureResult());
    }
    return {};
  }
}  // namespace Module::DAC
