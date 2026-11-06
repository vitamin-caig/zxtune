/**
 *
 * @file
 *
 * @brief  ProTracker v2.x chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/protracker2.h"

#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_chiptune.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/generic_presentation.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"

#include "binary/container.h"
#include "parameters/container.h"

#include "make_ptr.h"

#include <array>

namespace Module::ProTracker2
{
  // Supported commands and their parameters
  enum CmdType
  {
    // type, tone
    ENVELOPE,
    // no parameters
    ENVELOPE_OFF,
    // glissade
    GLISS,
    // glissade step; target note is stored in the cell
    GLISS_NOTE,
    // no parameters
    NOGLISS,
    // addon; must stay per-channel as the last noise writer applies its own addon
    NOISE_ADD
  };

  using Formats::Chiptune::ProTracker2::Sample;
  using Formats::Chiptune::ProTracker2::Ornament;

  using ModuleData = AYM::ModuleData<OrderList, Sample, Ornament>;

  class DataBuilder : public Formats::Chiptune::ProTracker2::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetFrequencyTable(AYM::TABLE_PROTRACKER2);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, Formats::Chiptune::ProTracker2::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::ProTracker2::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::ProTracker2::Positions positions) override
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
      auto& channel = Patterns.GetChannel();
      channel.SetEnabled(true);
      channel.SetNote(note);
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

    void SetGlissade(int_t val) override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(GLISS, val);
    }

    void SetNoteGliss(int_t val, uint_t /*limit*/) override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(GLISS_NOTE, 0, val);
    }

    void SetNoGliss() override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(NOGLISS);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(ENVELOPE, type, value);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(ENVELOPE_OFF);
    }

    void SetNoiseAddon(int_t val) override
    {
      Patterns.GetChannel().GetCommands().AddOrUpdate(NOISE_ADD, val);
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

  const uint_t LIMITER = ~uint_t(0);

  inline uint_t GetVolume(uint_t volume, uint_t level)
  {
    return (volume * 17 + (volume > 7 ? 1 : 0)) * level / 256;
  }

  struct ChannelState
  {
    ChannelState()
      : SampleNum(Formats::Chiptune::ProTracker2::DEFAULT_SAMPLE)
      , OrnamentNum(Formats::Chiptune::ProTracker2::DEFAULT_ORNAMENT)
      , SlidingTargetNote(LIMITER)
    {}
    bool Enabled = false;
    bool Envelope = false;
    uint_t Note = 0;
    // Native stores only the frequency delta and keeps the source note as the tone base.
    // Both require the source note when the target note is kept in the cell.
    uint_t PrevNote = 0;
    uint_t SampleNum;
    uint_t PosInSample = 0;
    uint_t OrnamentNum;
    uint_t PosInOrnament = 0;
    uint_t Volume = 15;
    int_t NoiseAdd = 0;
    int_t Sliding = 0;
    uint_t SlidingTargetNote;
    int_t Glissade = 0;
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
        if (!dst.Enabled)
        {
          dst.Sliding = dst.Glissade = 0;
          dst.SlidingTargetNote = LIMITER;
        }
        dst.PosInSample = dst.PosInOrnament = 0;
      }
      const uint_t* const note = src.GetNote();
      if (note)
      {
        assert(src.GetEnabled());
        // A note-gliss keeps the source note sounding until the slide completes,
        // so the cell note (the target) cannot become the tone base immediately.
        bool noteGliss = false;
        for (const auto& cmd : src.GetCommands())
        {
          noteGliss = noteGliss || cmd.Type == GLISS_NOTE;
        }
        dst.PrevNote = noteGliss ? dst.PrevNote : *note;
        dst.Note = *note;
        dst.Sliding = dst.Glissade = 0;
        dst.SlidingTargetNote = LIMITER;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleNum = *sample;
        dst.PosInSample = 0;
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
        case NOISE_ADD:
          dst.NoiseAdd = static_cast<int8_t>(cmd.Param1);
          break;
        case GLISS_NOTE:
          dst.Sliding = 0;
          dst.Glissade = cmd.Param2;
          dst.SlidingTargetNote = dst.Note;
          break;
        case GLISS:
          dst.Glissade = static_cast<int8_t>(cmd.Param1);
          dst.SlidingTargetNote = LIMITER;
          break;
        case NOGLISS:
          dst.Glissade = 0;
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
      const int_t halftones = int_t(dst.PrevNote) + curOrnament.GetLine(dst.PosInOrnament);
      channel.SetTone(halftones, dst.Sliding + curSampleLine.Vibrato);
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
        track.SetNoise(curSampleLine.Noise + dst.NoiseAdd);
      }
      else
      {
        channel.DisableNoise();
      }

      // recalculate gliss
      if (dst.SlidingTargetNote != LIMITER)
      {
        const int_t absoluteSlidingRange = track.GetSlidingDifference(dst.PrevNote, dst.SlidingTargetNote);
        const int_t realSlidingRange = absoluteSlidingRange - (dst.Sliding + dst.Glissade);

        if ((dst.Glissade > 0 && realSlidingRange <= 0) || (dst.Glissade < 0 && realSlidingRange >= 0))
        {
          dst.Note = dst.SlidingTargetNote;
          dst.PrevNote = dst.SlidingTargetNote;
          dst.SlidingTargetNote = LIMITER;
          dst.Sliding = dst.Glissade = 0;
        }
      }
      dst.Sliding += dst.Glissade;

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
      bool hasEnvelope = false;
      bool hasEnvelopeOff = false;
      uint_t envelopeType = 0;
      for (const auto& cmd : cell.GetCommands())
      {
        switch (cmd.Type)
        {
        case ENVELOPE:
          hasEnvelope = true;
          envelopeType = cmd.Param1;
          prefix.EnvelopeTone = cmd.Param2;
          break;
        case ENVELOPE_OFF:
          hasEnvelopeOff = true;
          break;
        default:
          break;
        }
      }
      if (const auto* sample = cell.GetSample())
      {
        txt.Sample = *sample;
      }
      else
      {
        txt.Sample = 0;
      }
      txt.Envelope = hasEnvelope ? envelopeType : (hasEnvelopeOff ? 0x0f : 0);
      if (const auto* ornament = cell.GetOrnament())
      {
        txt.Ornament = *ornament;
      }
      else
      {
        txt.Ornament = 0;
      }
      if (const auto* volume = cell.GetVolume())
      {
        txt.Volume = *volume;
      }
      else
      {
        txt.Volume = 0;
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
}  // namespace Module::ProTracker2

namespace Module::AYM
{
  Chiptune::Ptr CreateProTracker2Chiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    ProTracker2::DataBuilder dataBuilder(props);
    if (const auto container = Formats::Chiptune::ProTracker2::Parse(rawData, dataBuilder))
    {
      props.SetSource(*container);
      return CreateTrackingChiptune<ProTracker2::DataRenderer>(dataBuilder.CaptureResult(),
                                                               &ProTracker2::Presentation::ForPattern);
    }
    return {};
  }
}  // namespace Module::AYM
