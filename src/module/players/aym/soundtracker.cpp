/**
 *
 * @file
 *
 * @brief  SoundTracker-based chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/soundtracker.h"

#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"

#include "make_ptr.h"

namespace Module::SoundTracker
{
  enum CmdType
  {
    EMPTY,
    ENVELOPE,    // 2p
    NOENVELOPE,  // 0p
  };

  using Formats::Chiptune::SoundTracker::Sample;
  using Formats::Chiptune::SoundTracker::Ornament;

  using OrderListWithTransposition = SimpleOrderListWithTransposition<Formats::Chiptune::SoundTracker::PositionEntry>;

  using ModuleData = AYM::ModuleData<OrderListWithTransposition, Sample, Ornament>;

  class DataBuilder : public Formats::Chiptune::SoundTracker::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetFrequencyTable(TABLE_SOUNDTRACKER);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, Formats::Chiptune::SoundTracker::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::SoundTracker::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::SoundTracker::Positions positions) override
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
      Patterns.GetChannel().AddCommand(SoundTracker::ENVELOPE, type, value);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().AddCommand(SoundTracker::NOENVELOPE);
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

  class ChannelBuilder
  {
  public:
    ChannelBuilder(int_t transposition, AYM::TrackBuilder& track, uint_t chanNum)
      : Transposition(transposition)
      , Track(track)
      , Channel(Track.GetChannel(chanNum))
    {}

    void SetLevel(uint_t level)
    {
      Channel.SetLevel(level);
    }

    void EnableEnvelope()
    {
      Channel.EnableEnvelope();
    }

    void SetTone(int_t halftones, int_t offset)
    {
      Channel.SetTone(halftones + Transposition, offset);
    }

    void DisableTone()
    {
      Channel.DisableTone();
    }

    void SetNoise(int_t level)
    {
      Track.SetNoise(level);
    }

    void DisableNoise()
    {
      Channel.DisableNoise();
    }

  private:
    const int_t Transposition;
    AYM::TrackBuilder& Track;
    AYM::ChannelBuilder Channel;
  };

  struct EnvelopeState
  {
    explicit EnvelopeState(uint_t& type, uint_t& tone)
      : Type(type)
      , Tone(tone)
    {}

    void Reset()
    {
      Enabled = 0;
    }

    void SetNewState(const Cell& src)
    {
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        ApplyCommand(*it);
      }
    }

    void Iterate()
    {
      if (2 == Enabled)
      {
        Type = 0;
      }
      else if (1 == Enabled)
      {
        Enabled = 2;
        Type = 0;
      }
    }

    void Synthesize(ChannelBuilder& channel) const
    {
      if (Enabled)
      {
        channel.EnableEnvelope();
      }
    }

  private:
    void ApplyCommand(const Command& command)
    {
      if (command == ENVELOPE)
      {
        Type = command.Param1;
        Tone = command.Param2;
        Enabled = 1;
      }
      else if (command == NOENVELOPE)
      {
        Enabled = 0;
      }
    }

  private:
    uint_t& Type;
    uint_t& Tone;
    uint_t Enabled = 0;
  };

  struct StateCursor
  {
    int_t CountDown = -1;
    uint_t Position = 0;

    StateCursor() = default;

    void Next(const Sample& sample)
    {
      if (!IsValid())
      {
        return;
      }
      --CountDown;
      Position = (Position + 1) & 0x1f;
      if (0 == CountDown)
      {
        if (const uint_t loop = sample.GetLoop())
        {
          Position = loop & 0x1f;
          CountDown = sample.GetLoopLimit() - loop + 1;
        }
        else
        {
          CountDown = -1;
        }
      }
    }

    bool IsValid() const
    {
      return CountDown >= 0;
    }
  };

  struct ChannelState
  {
    explicit ChannelState(ModuleData::Ptr data, uint_t& envType, uint_t& envTone)
      : Data(std::move(data))
      , CurSample(GetStubSample())
      , CurOrnament(GetStubOrnament())
      , EnvState(envType, envTone)
    {}

    void Reset()
    {
      Note = 0;
      Cursor = StateCursor();
      CurSample = GetStubSample();
      CurOrnament = GetStubOrnament();
      EnvState.Reset();
    }

    void SetNewState(const Cell& src)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        Cursor.CountDown = *enabled ? 32 : -1;
      }
      if (const uint_t* note = src.GetNote())
      {
        Note = *note;
        Cursor.Position = 0;
      }
      if (const uint_t* sample = src.GetSample())
      {
        CurSample = &Data->Samples.Get(*sample);
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        CurOrnament = &Data->Ornaments.Get(*ornament);
      }
      EnvState.SetNewState(src);
    }

    void Synthesize(ChannelBuilder& channel) const
    {
      StateCursor nextState(Cursor);
      nextState.Next(*CurSample);
      if (!nextState.IsValid())
      {
        channel.SetLevel(0);
        return;
      }

      const uint_t nextPosition = (nextState.Position - 1) & 0x1f;
      const Sample::Line& curSampleLine = CurSample->GetLine(nextPosition);
      // apply level
      channel.SetLevel(curSampleLine.Level);
      // apply tone
      const int_t halftones = int_t(Note) + CurOrnament->GetLine(nextPosition);
      channel.SetTone(halftones, curSampleLine.Effect);
      if (curSampleLine.EnvelopeMask)
      {
        channel.DisableTone();
      }
      // apply noise
      if (!curSampleLine.NoiseMask)
      {
        channel.SetNoise(curSampleLine.Noise);
      }
      else
      {
        channel.DisableNoise();
      }
      EnvState.Synthesize(channel);
    }

    void Iterate()
    {
      Cursor.Next(*CurSample);
      if (Cursor.IsValid())
      {
        EnvState.Iterate();
      }
    }

  private:
    static const Sample* GetStubSample()
    {
      static const Sample stubSample;
      return &stubSample;
    }

    static const Ornament* GetStubOrnament()
    {
      static const Ornament stubOrnament;
      return &stubOrnament;
    }

  private:
    const ModuleData::Ptr Data;
    uint_t Note = 0;
    StateCursor Cursor;
    const Sample* CurSample;
    const Ornament* CurOrnament;
    EnvelopeState EnvState;
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(std::move(data))
      , StateA(Data, EnvType, EnvTone)
      , StateB(Data, EnvType, EnvTone)
      , StateC(Data, EnvType, EnvTone)
    {}

    void Reset() override
    {
      StateA.Reset();
      StateB.Reset();
      StateC.Reset();
      EnvType = EnvTone = 0;
    }

    void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track) override
    {
      if (0 == state.Quirk())
      {
        SwitchToNewLine(state);
      }
      SynthesizeChannelsData(state, track);
      IterateState();
    }

  private:
    void SwitchToNewLine(const TrackModelState& state)
    {
      assert(0 == state.Quirk());
      if (const auto* const line = state.LineObject())
      {
        if (const auto* const chan = line->GetChannel(0))
        {
          StateA.SetNewState(*chan);
        }
        if (const auto* const chan = line->GetChannel(1))
        {
          StateB.SetNewState(*chan);
        }
        if (const auto* const chan = line->GetChannel(2))
        {
          StateC.SetNewState(*chan);
        }
      }
    }

    void SynthesizeChannelsData(const TrackState& state, AYM::TrackBuilder& track) const
    {
      const int_t transposition = Data->Order->GetTransposition(state.Position());
      {
        ChannelBuilder channel(transposition, track, 0);
        StateA.Synthesize(channel);
      }
      {
        ChannelBuilder channel(transposition, track, 1);
        StateB.Synthesize(channel);
      }
      {
        ChannelBuilder channel(transposition, track, 2);
        StateC.Synthesize(channel);
      }
      if (EnvType)
      {
        track.SetEnvelopeType(EnvType);
        track.SetEnvelopeTone(EnvTone);
      }
    }

    void IterateState()
    {
      StateA.Iterate();
      StateB.Iterate();
      StateC.Iterate();
    }

  private:
    const ModuleData::Ptr Data;
    ChannelState StateA;
    ChannelState StateB;
    ChannelState StateC;
    uint_t EnvType = 0, EnvTone = 0;
  };

  class Factory : public AYM::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::SoundTracker::Decoder::Ptr decoder)
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
    const Formats::Chiptune::SoundTracker::Decoder::Ptr Decoder;
  };

  Factory::Ptr CreateFactory(Formats::Chiptune::SoundTracker::Decoder::Ptr decoder)
  {
    return MakePtr<Factory>(std::move(decoder));
  }
}  // namespace Module::SoundTracker
