/**
 *
 * @file
 *
 * @brief  GlobalTracker chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/aym/globaltracker.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_properties_helper.h"
// common includes
#include <make_ptr.h>
// library includes
#include <formats/chiptune/aym/globaltracker.h>
#include <math/numeric.h>
#include <module/players/platforms.h>
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>

namespace Module::GlobalTracker
{
  enum CmdType
  {
    EMPTY,
    ENVELOPE,    // 2p
    NOENVELOPE,  // 0p
  };

  using Formats::Chiptune::GlobalTracker::Sample;
  using Formats::Chiptune::GlobalTracker::Ornament;

  using ModuleData = AYM::ModuleData<OrderList, Sample, Ornament>;

  class DataBuilder : public Formats::Chiptune::GlobalTracker::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetFrequencyTable(TABLE_PROTRACKER3_ST);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
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

    void SetVolume(uint_t vol) override
    {
      Patterns.GetChannel().SetVolume(vol);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE, type, value);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().AddCommand(NOENVELOPE);
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
    ChannelState() {}
    bool Enabled = false;
    bool Envelope = false;
    uint_t Note = 0;
    uint_t SampleNum = 0;
    uint_t PosInSample = 0;
    uint_t OrnamentNum = 0;
    uint_t PosInOrnament = 0;
    uint_t Volume = 0;
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
      if (const auto line = state.LineObject())
      {
        for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
        {
          if (const auto src = line->GetChannel(chan))
          {
            GetNewChannelState(*src, PlayerState[chan], track);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
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
        default:
          assert(!"Invalid command");
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

      const Sample& curSample = Data->Samples.Get(dst.SampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data->Ornaments.Get(dst.OrnamentNum);

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
      if (const auto container = Formats::Chiptune::GlobalTracker::Parse(rawData, dataBuilder))
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
}  // namespace Module::GlobalTracker
