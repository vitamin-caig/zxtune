/**
 *
 * @file
 *
 * @brief  ProTracker v2.x chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/aym/protracker2.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_properties_helper.h"
// common includes
#include <make_ptr.h>
// library includes
#include <formats/chiptune/aym/protracker2.h>
#include <module/players/platforms.h>
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>

namespace Module::ProTracker2
{
  // supported commands and parameters
  enum CmdType
  {
    // no parameters
    EMPTY,
    // r13,period
    ENVELOPE,
    // no parameters
    NOENVELOPE,
    // glissade
    GLISS,
    // glissade,target node
    GLISS_NOTE,
    // no parameters
    NOGLISS,
    // noise addon
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
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetFrequencyTable(TABLE_PROTRACKER2);
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
      MutableCell& channel = Patterns.GetChannel();
      channel.SetEnabled(true);
      if (Command* cmd = channel.FindCommand(GLISS_NOTE))
      {
        cmd->Param2 = int_t(note);
      }
      else
      {
        channel.SetNote(note);
      }
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
      Patterns.GetChannel().AddCommand(GLISS, val);
    }

    void SetNoteGliss(int_t val, uint_t limit) override
    {
      Patterns.GetChannel().AddCommand(GLISS_NOTE, val, limit);
    }

    void SetNoGliss() override
    {
      Patterns.GetChannel().AddCommand(NOGLISS);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE, type, value);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().AddCommand(NOENVELOPE);
    }

    void SetNoiseAddon(int_t val) override
    {
      Patterns.GetChannel().AddCommand(NOISE_ADD, val);
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

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(std::move(data))
    {}

    void Reset() override
    {
      std::fill(PlayerState.begin(), PlayerState.end(), ChannelState());
    }

    void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track) override
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(track);
    }

  private:
    void GetNewLineState(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (const auto* const line = state.LineObject())
      {
        for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
        {
          if (const auto* const src = line->GetChannel(chan))
          {
            GetNewChannelState(*src, PlayerState[chan], track);
          }
        }
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
      if (const uint_t* note = src.GetNote())
      {
        assert(src.GetEnabled());
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
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          track.SetEnvelopeType(it->Param1);
          track.SetEnvelopeTone(it->Param2);
          dst.Envelope = true;
          break;
        case NOENVELOPE:
          dst.Envelope = false;
          break;
        case NOISE_ADD:
          dst.NoiseAdd = it->Param1;
          break;
        case GLISS_NOTE:
          dst.Sliding = 0;
          dst.Glissade = it->Param1;
          dst.SlidingTargetNote = it->Param2;
          break;
        case GLISS:
          dst.Glissade = it->Param1;
          dst.SlidingTargetNote = LIMITER;
          break;
        case NOGLISS:
          dst.Glissade = 0;
          break;
        default:
          assert(!"Invalid command");
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

      const Sample& curSample = Data->Samples.Get(dst.SampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data->Ornaments.Get(dst.OrnamentNum);

      // apply tone
      const int_t halftones = int_t(dst.Note) + curOrnament.GetLine(dst.PosInOrnament);
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
        const int_t absoluteSlidingRange = track.GetSlidingDifference(dst.Note, dst.SlidingTargetNote);
        const int_t realSlidingRange = absoluteSlidingRange - (dst.Sliding + dst.Glissade);

        if ((dst.Glissade > 0 && realSlidingRange <= 0) || (dst.Glissade < 0 && realSlidingRange >= 0))
        {
          dst.Note = dst.SlidingTargetNote;
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
    const ModuleData::Ptr Data;
    std::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
  };

  class Factory : public AYM::Factory
  {
  public:
    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData,
                                      Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Formats::Chiptune::ProTracker2::Parse(rawData, dataBuilder))
      {
        props.SetSource(*container);
        props.SetPlatform(Platforms::ZX_SPECTRUM);
        return MakePtr<AYM::TrackingChiptune<ModuleData, DataRenderer>>(dataBuilder.CaptureResult(),
                                                                        std::move(properties));
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
}  // namespace Module::ProTracker2
