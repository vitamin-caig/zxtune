/**
 *
 * @file
 *
 * @brief  ProSoundMaker chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/aym/prosoundmaker.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_properties_helper.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <formats/chiptune/aym/prosoundmaker.h>
#include <math/numeric.h>
#include <module/players/platforms.h>
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>
// std includes
#include <optional>

namespace Module::ProSoundMaker
{
  // supported commands and parameters
  enum CmdType
  {
    // no parameters
    EMPTY,
    // r13,period,note delta
    ENVELOPE,
    // disable ornament
    NOORNAMENT,
    // envelope reinit on every note,true/false
    ENVELOPE_REINIT
  };

  using Formats::Chiptune::ProSoundMaker::Sample;
  using Formats::Chiptune::ProSoundMaker::Ornament;

  using OrderListWithTransposition = SimpleOrderListWithTransposition<Formats::Chiptune::ProSoundMaker::PositionEntry>;

  using ModuleData = AYM::ModuleData<OrderListWithTransposition, Sample, Ornament>;

  class DataBuilder : public Formats::Chiptune::ProSoundMaker::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetFrequencyTable(TABLE_PROSOUNDMAKER);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetSample(uint_t index, Formats::Chiptune::ProSoundMaker::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::ProSoundMaker::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::ProSoundMaker::Positions positions) override
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

    void SetVolume(uint_t volume) override
    {
      Patterns.GetChannel().SetVolume(volume);
    }

    void DisableOrnament() override
    {
      Patterns.GetChannel().AddCommand(NOORNAMENT);
    }

    void SetEnvelopeReinit(bool enabled) override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE_REINIT, enabled);
    }

    void SetEnvelopeTone(uint_t type, uint_t tone) override
    {
      MutableCell& channel = Patterns.GetChannel();
      Require(!channel.FindCommand(ENVELOPE));
      channel.AddCommand(ENVELOPE, int_t(type), int_t(tone), -1);
    }

    void SetEnvelopeNote(uint_t type, uint_t note) override
    {
      MutableCell& channel = Patterns.GetChannel();
      Require(!channel.FindCommand(ENVELOPE));
      channel.AddCommand(ENVELOPE, int_t(type), -1, int_t(note));
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

  struct SampleState
  {
    SampleState() = default;

    const Sample* Current = nullptr;
    uint_t Position = 0;
    uint_t LoopsCount = 0;
    bool Finished = false;
  };

  struct OrnamentState
  {
    OrnamentState() = default;

    const Ornament* Current = nullptr;
    uint_t Position = 0;
    bool Finished = false;
    bool KeepFinished = false;
  };

  struct EnvelopeState
  {
    EnvelopeState() = default;

    bool Reinit = false;
    uint_t Type = 0;
    std::optional<uint_t> Note;
    std::optional<uint_t> Tone;

    bool Enabled() const
    {
      return Note || Tone;
    }

    void Disable()
    {
      Note.reset();
      Tone.reset();
    }

    void SetNote(uint_t note)
    {
      Note = note;
      Tone.reset();
      Reinit = true;
    }

    void SetTone(uint_t tone)
    {
      Note.reset();
      Tone = tone;
    }
  };

  struct ChannelState
  {
    ChannelState() = default;
    bool Enabled = false;
    EnvelopeState Envelope;
    uint_t Note = 0;
    SampleState Smp;
    OrnamentState Orn;
    uint_t VolumeDelta = 0;
    uint_t BaseVolumeDelta = 0;
    int_t Slide = 0;
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(std::move(data))
    {
      Reset();
    }

    void Reset() override
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        ChannelState& state = PlayerState[chan];
        state = ChannelState();
        state.Smp.Current = &Data->Samples.Get(0);
        state.Orn.Current = &Data->Ornaments.Get(0);
      }
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
        const auto transposition = Data->Order->GetTransposition(state.Position());
        const auto newPattern = 0 == state.Line();
        for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
        {
          if (const auto* const src = line->GetChannel(chan))
          {
            auto& dst = PlayerState[chan];
            if (newPattern)
            {
              dst.Envelope.Reinit = false;
            }
            GetNewChannelState(transposition, *src, dst, track);
          }
        }
      }
    }

    void GetNewChannelState(uint_t transposition, const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      static const Ornament STUB;
      if (const bool* enabled = src.GetEnabled())
      {
        dst.Enabled = *enabled;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.Smp.Current = &Data->Samples.Get(*sample);
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.Orn.Current = &Data->Ornaments.Get(*ornament);
        dst.Orn.Position = 0;
        dst.Orn.Finished = dst.Orn.KeepFinished = false;
        dst.Envelope.Disable();
        dst.Envelope.Reinit = false;
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.BaseVolumeDelta = *volume;
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          dst.Envelope.Type = it->Param1;
          if (it->Param2 != -1)
          {
            track.SetEnvelopeType(it->Param1);
            track.SetEnvelopeTone(it->Param2);
            dst.Envelope.SetTone(it->Param2);
          }
          else if (it->Param3 != -1)
          {
            dst.Envelope.SetNote(it->Param3);
          }
          break;
        case NOORNAMENT:
          dst.Orn.Finished = dst.Orn.KeepFinished = true;
          dst.Orn.Position = 0;
          dst.Envelope.Disable();
          break;
        case ENVELOPE_REINIT:
          dst.Envelope.Reinit = it->Param1 != 0;
          dst.Orn.Current = &STUB;
          break;
        default:
          assert(!"Invalid command");
        }
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note + transposition;
        dst.VolumeDelta = dst.BaseVolumeDelta;
        dst.Smp.Position = 0;
        dst.Smp.Finished = false;
        dst.Slide = 0;
        dst.Smp.LoopsCount = 1;
        dst.Orn.Position = 0;
        dst.Orn.Finished = dst.Orn.KeepFinished;
        if (dst.Envelope.Reinit && dst.Envelope.Enabled())
        {
          track.SetEnvelopeType(dst.Envelope.Type);
          if (dst.Envelope.Note)
          {
            uint_t envNote = dst.Note + *dst.Envelope.Note;
            if (envNote >= 0x60)
            {
              envNote -= 0x60;
            }
            else if (envNote >= 0x30)
            {
              envNote -= 0x30;
            }
            const uint_t envFreq = track.GetFrequency((envNote & 0x7f) + 0x30);
            track.SetEnvelopeTone(envFreq & 0xff);
          }
          else
          {
            track.SetEnvelopeTone(*dst.Envelope.Tone);
          }
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

    static void SynthesizeChannel(ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      const bool hasEnvelope = dst.Envelope.Enabled();
      const Ornament& curOrnament = *dst.Orn.Current;
      const int_t ornamentLine = (hasEnvelope || dst.Orn.Finished) ? 0 : curOrnament.GetLine(dst.Orn.Position);
      const Sample& curSample = *dst.Smp.Current;
      const Sample::Line& curSampleLine = curSample.GetLine(dst.Smp.Position);

      dst.Slide += curSampleLine.Gliss;
      const auto halftones = Math::Clamp<int_t>(int_t(dst.Note) + ornamentLine, 0, 95);
      const auto tone = Math::Clamp<int_t>(track.GetFrequency(halftones) + dst.Slide, 0, 4095);
      channel.SetTone(tone);

      // emulate level construction due to possibility of envelope bit reset
      int_t level = int_t(curSampleLine.Level | (hasEnvelope ? 16 : 0)) + dst.VolumeDelta - 15;
      if (!dst.Enabled || level < 0)
      {
        level = 0;
      }
      channel.SetLevel(level & 15);
      if (level & 16)
      {
        channel.EnableEnvelope();
      }
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      if (!curSampleLine.NoiseMask && dst.Enabled)
      {
        if (level)
        {
          track.SetNoise(curSampleLine.Noise);
        }
      }
      else
      {
        channel.DisableNoise();
      }

      if (!dst.Smp.Finished && ++dst.Smp.Position >= curSample.GetSize())
      {
        const auto loop = curSample.GetLoop();
        if (loop < curSample.GetSize())
        {
          dst.Smp.Position = loop;
          if (!--dst.Smp.LoopsCount)
          {
            dst.Smp.LoopsCount = curSample.VolumeDeltaPeriod;
            dst.VolumeDelta = Math::Clamp<int_t>(int_t(dst.VolumeDelta) + curSample.VolumeDeltaValue, 0, 15);
          }
        }
        else
        {
          dst.Enabled = false;
          dst.Smp.Finished = true;
        }
      }

      if (!dst.Orn.Finished && ++dst.Orn.Position >= curOrnament.GetSize())
      {
        const auto loop = curOrnament.GetLoop();
        if (loop < curOrnament.GetSize())
        {
          dst.Orn.Position = loop;
        }
        else
        {
          dst.Orn.Finished = true;
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
    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData,
                                      Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Formats::Chiptune::ProSoundMaker::ParseCompiled(rawData, dataBuilder))
      {
        props.SetSource(*container);
        props.SetPlatform(Platforms::ZX_SPECTRUM);
        return MakePtr<AYM::TrackingChiptune<ModuleData, DataRenderer>>(dataBuilder.CaptureResult(), properties);
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
}  // namespace Module::ProSoundMaker
