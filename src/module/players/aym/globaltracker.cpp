/**
 *
 * @file
 *
 * @brief  GlobalTracker chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/globaltracker.h"

#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_chiptune.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"
#include "module/players/track_builder.h"

#include "binary/container.h"
#include "math/numeric.h"
#include "parameters/container.h"

#include "make_ptr.h"

#include <array>

namespace Module::GlobalTracker
{
  // Supported commands and their parameters
  enum CmdType
  {
    // type, tone
    ENVELOPE,
    // no parameters
    ENVELOPE_OFF
  };

  using Formats::Chiptune::GlobalTracker::Sample;
  using Formats::Chiptune::GlobalTracker::Ornament;

  using ModuleData = AYM::ModuleData<OrderList, Sample, Ornament>;

  class DataChannelBuilder : public TrackChannelBuilder<Formats::Chiptune::GlobalTracker::ChannelBuilder>
  {
  public:
    void SetEnvelope(uint_t type, uint_t value) override
    {
      GetCommands().AddOrReplace(ENVELOPE, type, value);
    }
    void SetNoEnvelope() override
    {
      GetCommands().AddOrReplace(ENVELOPE_OFF);
    }
  };

  using LineBuilder = TrackLineBuilder<Formats::Chiptune::GlobalTracker::ChannelBuilder, DataChannelBuilder>;
  using PatternsBuilder = TrackPatternsBuilder<Formats::Chiptune::GlobalTracker::ChannelBuilder, DataChannelBuilder>;

  class DataBuilder : public TrackDataBuilder<Formats::Chiptune::GlobalTracker::Builder, ModuleData, PatternsBuilder>
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : TrackDataBuilder(props)
      , Properties(props)
    {
      Properties.SetFrequencyTable(AYM::TABLE_PROTRACKER3_ST);
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, Formats::Chiptune::GlobalTracker::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::GlobalTracker::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::GlobalTracker::Positions positions) override
    {
      Data->Order = MakePtr<SimpleOrderList>(positions.Loop, std::move(positions.Lines));
    }

  private:
    AYM::PropertiesHelper& Properties;
  };

  struct ChannelState
  {
    ChannelState() = default;
    bool Enabled = false;
    bool Envelope = false;
    uint_t Note = 0;
    uint_t SampleNum = 0;
    uint_t PosInSample = 0;
    uint_t OrnamentNum = 0;
    uint_t PosInOrnament = 0;
    uint_t Volume = 0;
  };

  class DataRenderer
  {
  public:
    explicit DataRenderer(const ModuleData& data)
      : Data(data)
    {}

    void Reset()
    {
      std::fill(PlayerState.begin(), PlayerState.end(), ChannelState());
    }

    void SynthesizeData(const TrackState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Quirk)
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(track);
    }

  private:
    void GetNewLineState(const TrackState& state, AYM::TrackBuilder& track)
    {
      if (const auto* const line = Data.GetLine(state))
      {
        line->ForEachChannel([&](auto chan, const auto& src) { GetNewChannelState(src, PlayerState[chan], track); });
      }
    }

    static void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        dst.Enabled = *enabled;
      }
      if (const uint_t* note = src.GetNote())
      {
        assert(src.GetEnabled());
        dst.Note = *note;
        dst.PosInSample = dst.PosInOrnament = 0;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleNum = *sample;
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentNum = *ornament;
        dst.PosInOrnament = 0;
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Volume = *volume;
      }
      for (const auto& cmd : src.GetCommands())
      {
        switch (cmd.Type)
        {
        case ENVELOPE:
          track.SetEnvelopeType(cmd.Param1);
          track.SetEnvelopeTone(cmd.Param2);
          dst.Envelope = true;
          break;
        case ENVELOPE_OFF:
          dst.Envelope = false;
          break;
        default:
          break;
        }
      }
    }

    void SynthesizeChannelsData(AYM::TrackBuilder& track)
    {
      uint_t noise = 0;
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(PlayerState[chan], channel, track, noise);
      }
      track.SetNoise(noise);
    }

    void SynthesizeChannel(ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track, uint_t& noise)
    {
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        channel.DisableTone();
        channel.DisableNoise();
        return;
      }

      const Sample& curSample = Data.Samples.Get(dst.SampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data.Ornaments.Get(dst.OrnamentNum);

      // apply tone
      const auto halftones = Math::Clamp<int_t>(int_t(dst.Note) + curOrnament.GetLine(dst.PosInOrnament), 0, 95);
      const uint_t freq = (track.GetFrequency(halftones) + curSampleLine.Vibrato) & 0xfff;
      channel.SetTone(freq);

      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      const auto level = Math::Clamp<int_t>(int_t(curSampleLine.Level) - dst.Volume, 0, 255);
      // apply level
      channel.SetLevel(level & 0xf);
      // apply envelope
      if (dst.Envelope && curSampleLine.EnvelopeMask)
      {
        channel.EnableEnvelope();
      }
      // apply noise
      noise |= curSampleLine.Noise;
      if (curSampleLine.NoiseMask)
      {
        channel.DisableNoise();
      }

      if (++dst.PosInSample >= curSample.GetSize())
      {
        dst.PosInSample = curSample.GetLoop();
      }
      if (++dst.PosInOrnament >= curOrnament.GetSize())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }
    }

  private:
    const ModuleData& Data;
    std::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
  };
}  // namespace Module::GlobalTracker

namespace Module::AYM
{
  Chiptune::Ptr CreateGlobalTrackerChiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    GlobalTracker::DataBuilder dataBuilder(props);
    if (const auto container = Formats::Chiptune::GlobalTracker::Parse(rawData, dataBuilder))
    {
      props.SetSource(*container);
      return CreateTrackingChiptune<GlobalTracker::DataRenderer>(dataBuilder.CaptureResult());
    }
    return {};
  }
}  // namespace Module::AYM
