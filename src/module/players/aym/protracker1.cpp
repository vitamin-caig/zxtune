/**
 *
 * @file
 *
 * @brief  ProTracker v1.x chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/protracker1.h"

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

namespace Module::ProTracker1
{
  // Supported commands and their parameters
  enum CmdType
  {
    // type, tone
    ENVELOPE,
    // no parameters
    ENVELOPE_OFF
  };

  using Formats::Chiptune::ProTracker1::Sample;
  using Formats::Chiptune::ProTracker1::Ornament;

  using ModuleData = AYM::ModuleData<OrderList, Sample, Ornament>;

  class DataChannelBuilder : public TrackChannelBuilder<Formats::Chiptune::ProTracker1::ChannelBuilder>
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

  using LineBuilder = TrackLineBuilder<Formats::Chiptune::ProTracker1::ChannelBuilder, DataChannelBuilder>;
  using PatternsBuilder = TrackPatternsBuilder<Formats::Chiptune::ProTracker1::ChannelBuilder, DataChannelBuilder>;

  class DataBuilder : public TrackDataBuilder<Formats::Chiptune::ProTracker1::Builder, ModuleData, PatternsBuilder>
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

    void SetSample(uint_t index, Formats::Chiptune::ProTracker1::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::ProTracker1::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::ProTracker1::Positions positions) override
    {
      Data->Order = MakePtr<SimpleOrderList>(positions.Loop, std::move(positions.Lines));
    }

  private:
    AYM::PropertiesHelper& Properties;
  };

  inline uint_t GetVolume(uint_t volume, uint_t level)
  {
    return ((volume * 17 + (volume > 7 ? 1 : 0)) * level + 128) / 256;
  }

  struct ChannelState
  {
    ChannelState() = default;
    bool Enabled = false;
    bool Envelope = false;
    uint_t Note = 0;
    uint_t SampleNum = 0;
    uint_t PosInSample = 0;
    uint_t OrnamentNum = 0;
    uint_t Volume = 15;
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
        dst.PosInSample = 0;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleNum = *sample;
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentNum = *ornament;
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
          track.SetEnvelopeTone(static_cast<uint16_t>(cmd.Param2));
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
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(PlayerState[chan], channel, track);
      }
    }

    void SynthesizeChannel(ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        return;
      }

      const Sample& curSample = Data.Samples.Get(dst.SampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data.Ornaments.Get(dst.OrnamentNum);

      // apply tone
      const auto halftones = Math::Clamp<int_t>(int_t(dst.Note) + curOrnament.GetLine(dst.PosInSample), 0, 95);
      channel.SetTone(halftones, curSampleLine.Vibrato + (halftones == 46));
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      // apply level
      channel.SetLevel(GetVolume(dst.Volume, curSampleLine.Level));
      // apply envelope
      if (dst.Envelope)
      {
        channel.EnableEnvelope();
      }
      // apply noise
      if (!curSampleLine.NoiseMask)
      {
        track.SetNoise(curSampleLine.Noise);
      }
      else
      {
        channel.DisableNoise();
      }

      if (++dst.PosInSample >= curSample.GetSize())
      {
        dst.PosInSample = curSample.GetLoop();
      }
    }

  private:
    const ModuleData& Data;
    std::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
  };
}  // namespace Module::ProTracker1

namespace Module::AYM
{
  Chiptune::Ptr CreateProTracker1Chiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    ProTracker1::DataBuilder dataBuilder(props);
    if (const auto container = Formats::Chiptune::ProTracker1::Parse(rawData, dataBuilder))
    {
      props.SetSource(*container);
      return CreateTrackingChiptune<ProTracker1::DataRenderer>(dataBuilder.CaptureResult());
    }
    return {};
  }
}  // namespace Module::AYM
