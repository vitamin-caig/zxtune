/**
 *
 * @file
 *
 * @brief  SoundTrackerPro-based chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/soundtrackerpro.h"

#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"

#include "make_ptr.h"

#include <array>

namespace Module::SoundTrackerPro
{
  enum CmdType
  {
    EMPTY,
    ENVELOPE,    // 2p
    NOENVELOPE,  // 0p
    GLISS,       // 1p
  };

  using Formats::Chiptune::SoundTrackerPro::Sample;
  using Formats::Chiptune::SoundTrackerPro::Ornament;

  using OrderListWithTransposition =
      SimpleOrderListWithTransposition<Formats::Chiptune::SoundTrackerPro::PositionEntry>;

  using ModuleData = AYM::ModuleData<OrderListWithTransposition, Sample, Ornament>;

  class DataBuilder : public Formats::Chiptune::SoundTrackerPro::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetFrequencyTable(TABLE_SOUNDTRACKER_PRO);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, Formats::Chiptune::SoundTrackerPro::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::SoundTrackerPro::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::SoundTrackerPro::Positions positions) override
    {
      Data->Order = MakePtr<OrderListWithTransposition>(positions.Loop, std::move(positions.Lines));
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

    void SetOrnament(uint_t ornament) override
    {
      Patterns.GetChannel().SetOrnament(ornament);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE, type, value);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().AddCommand(NOENVELOPE);
    }

    void SetGliss(uint_t target) override
    {
      Patterns.GetChannel().AddCommand(GLISS, target);
    }

    void SetVolume(uint_t vol) override
    {
      Patterns.GetChannel().SetVolume(vol);
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

  struct ChannelState
  {
    ChannelState()
      : SampleNum(Formats::Chiptune::SoundTrackerPro::DEFAULT_SAMPLE)
      , OrnamentNum(Formats::Chiptune::SoundTrackerPro::DEFAULT_ORNAMENT)
    {}
    bool Enabled = false;
    bool Envelope = false;
    uint_t Volume = 0;
    uint_t Note = 0;
    uint_t SampleNum;
    uint_t PosInSample = 0;
    uint_t OrnamentNum;
    uint_t PosInOrnament = 0;
    int_t TonSlide = 0;
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
      SynthesizeChannelsData(state, track);
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
        dst.PosInSample = 0;
        dst.PosInOrnament = 0;
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
        dst.PosInSample = 0;
        dst.PosInOrnament = 0;
        dst.TonSlide = 0;
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
          if (it->Param1)
          {
            track.SetEnvelopeType(it->Param1);
            track.SetEnvelopeTone(it->Param2);
          }
          dst.Envelope = true;
          break;
        case NOENVELOPE:
          dst.Envelope = false;
          break;
        case GLISS:
          dst.Glissade = it->Param1;
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }

    void SynthesizeChannelsData(const TrackState& state, AYM::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(state, PlayerState[chan], channel, track);
      }
    }

    void SynthesizeChannel(const TrackState& state, ChannelState& dst, AYM::ChannelBuilder& channel,
                           AYM::TrackBuilder& track)
    {
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        channel.DisableTone();
        channel.DisableNoise();
        return;
      }

      const Sample& curSample = Data->Samples.Get(dst.SampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data->Ornaments.Get(dst.OrnamentNum);

      // calculate tone
      dst.TonSlide += dst.Glissade;

      // apply level
      channel.SetLevel(int_t(curSampleLine.Level) - dst.Volume);
      // apply envelope
      if (curSampleLine.EnvelopeMask && dst.Envelope)
      {
        channel.EnableEnvelope();
      }
      // apply tone
      const int_t halftones = int_t(dst.Note) + Data->Order->GetTransposition(state.Position())
                              + (dst.Envelope ? 0 : curOrnament.GetLine(dst.PosInOrnament));
      channel.SetTone(halftones, dst.TonSlide + curSampleLine.Vibrato);
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
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

      if (++dst.PosInOrnament >= curOrnament.GetSize())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }

      if (++dst.PosInSample >= curSample.GetSize())
      {
        const auto loop = curSample.GetLoop();
        if (loop < dst.PosInSample)
        {
          dst.PosInSample = loop;
        }
        else
        {
          dst.Enabled = false;
        }
      }
    }

  private:
    const ModuleData::Ptr Data;
    std::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
  };

  class Factory : public AYM::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::SoundTrackerPro::Decoder::Ptr decoder)
      : Decoder(std::move(decoder))
    {}

    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData,
                                      Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Decoder->Parse(rawData, dataBuilder))
      {
        props.SetSource(*container);
        return MakePtr<AYM::TrackingChiptune<ModuleData, DataRenderer>>(dataBuilder.CaptureResult(),
                                                                        std::move(properties));
      }
      else
      {
        return {};
      }
    }

  private:
    const Formats::Chiptune::SoundTrackerPro::Decoder::Ptr Decoder;
  };

  Factory::Ptr CreateFactory(Formats::Chiptune::SoundTrackerPro::Decoder::Ptr decoder)
  {
    return MakePtr<Factory>(std::move(decoder));
  }
}  // namespace Module::SoundTrackerPro
