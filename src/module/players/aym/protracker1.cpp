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
#include "module/players/generic_presentation.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"

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

  class DataBuilder : public Formats::Chiptune::ProTracker1::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetFrequencyTable(AYM::TABLE_PROTRACKER3_ST);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
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
      auto& chan = Patterns.GetChannel();
      chan.SetEnabled(true);
      chan.SetNote(note);
    }

    void SetSample(uint_t sample) override
    {
      Patterns.GetChannel().SetSample(sample);
    }

    void SetOrnament(uint_t ornament) override
    {
      Patterns.GetChannel().SetOrnament(ornament);
    }

    void SetVolume(uint_t vol) override
    {
      Patterns.GetChannel().SetVolume(vol);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(ENVELOPE, type, value);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(ENVELOPE_OFF);
    }

    ModuleData::Ptr CaptureResult()
    {
      Data->Patterns = Patterns.CaptureResult();
      return std::move(Data);
    }

  private:
    AYM::PropertiesHelper& Properties;
    MetaProperties Meta;
    PatternsBuilder Patterns;
    ModuleData::RWPtr Data;
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
  namespace Presentation
  {
    using Delimiter = Module::Presentation::Delimiter;
    using Layout = Module::Presentation::Layout<'0'>;

    struct RowPrefix
    {
      Layout::Hex<4> EnvelopeTone;
    };

    struct Channel
    {
      Delimiter Separator;
      Module::Presentation::NoteAndOctave Note;
      char Space = ' ';
      Layout::Base36 Sample;
      Layout::Hex<1> Envelope;
      Layout::Hex<1> Ornament;
      Layout::Hex<1> Volume;
    };

    struct Row
    {
      Module::Presentation::Dec<2> Index;
      Delimiter Separator;
      RowPrefix Prefix;
      Channel Channels[AYM::TRACK_CHANNELS];
    };

    void FillCell(const Cell& cell, Channel& txt, RowPrefix& prefix)
    {
      txt.Note.Set(&cell);
      for (const auto& cmd : cell.GetCommands())
      {
        switch (cmd.Type)
        {
        case ENVELOPE:
          txt.Envelope = cmd.Param1;
          prefix.EnvelopeTone = cmd.Param2;
          break;
        case ENVELOPE_OFF:
          txt.Envelope = 0xf;
          break;
        default:
          break;
        }
      }
      if (const auto* sample = cell.GetSample())
      {
        txt.Sample = *sample;
      }
      if (const auto* ornament = cell.GetOrnament())
      {
        txt.Ornament = *ornament;
      }
      if (const auto* volume = cell.GetVolume())
      {
        txt.Volume = *volume;
      }
    }

    void FillLine(const Line& line, Row& row)
    {
      for (uint_t channel = 0; channel != AYM::TRACK_CHANNELS; ++channel)
      {
        if (const auto* cell = line.GetChannel(channel))
        {
          FillCell(*cell, row.Channels[channel], row.Prefix);
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
}  // namespace Module::ProTracker1

namespace Module::AYM
{
  Chiptune::Ptr CreateProTracker1Chiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    ProTracker1::DataBuilder dataBuilder(props);
    if (const auto container = Formats::Chiptune::ProTracker1::Parse(rawData, dataBuilder))
    {
      props.SetSource(*container);
      return CreateTrackingChiptune<ProTracker1::DataRenderer>(dataBuilder.CaptureResult(),
                                                               &ProTracker1::Presentation::ForPattern);
    }
    return {};
  }
}  // namespace Module::AYM
