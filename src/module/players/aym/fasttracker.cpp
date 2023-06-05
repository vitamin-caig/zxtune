/**
 *
 * @file
 *
 * @brief  FastTracker chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/aym/fasttracker.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_properties_helper.h"
// common includes
#include <make_ptr.h>
// library includes
#include <formats/chiptune/aym/fasttracker.h>
#include <math/numeric.h>
#include <module/players/platforms.h>
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>

namespace Module::FastTracker
{
  // supported commands and their parameters
  enum CmdType
  {
    // no parameters
    EMPTY,
    // r13,tone
    ENVELOPE,
    // no parameters
    ENVELOPE_OFF,
    // r5
    NOISE,
    // step
    SLIDE,
    // step,note
    SLIDE_NOTE
  };

  using Formats::Chiptune::FastTracker::Sample;
  using Formats::Chiptune::FastTracker::Ornament;

  using OrderListWithTransposition = SimpleOrderListWithTransposition<Formats::Chiptune::FastTracker::PositionEntry>;
  using ModuleData = AYM::ModuleData<OrderListWithTransposition, Sample, Ornament>;

  class DataBuilder : public Formats::Chiptune::FastTracker::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
      , Data(MakeRWPtr<ModuleData>())
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetNoteTable(Formats::Chiptune::FastTracker::NoteTable table) override
    {
      switch (table)
      {
      case Formats::Chiptune::FastTracker::NoteTable::SOUNDTRACKER:
        Properties.SetFrequencyTable(TABLE_SOUNDTRACKER);
        break;
      case Formats::Chiptune::FastTracker::NoteTable::FASTTRACKER:
        Properties.SetFrequencyTable(TABLE_FASTTRACKER);
        break;
      default:
        Properties.SetFrequencyTable(TABLE_PROTRACKER2);
        break;
      }
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, Formats::Chiptune::FastTracker::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::FastTracker::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::FastTracker::Positions positions) override
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
      MutableCell& channel = Patterns.GetChannel();
      channel.SetEnabled(true);
      if (Command* cmd = channel.FindCommand(SLIDE_NOTE))
      {
        cmd->Param2 = note;
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

    void SetEnvelope(uint_t type, uint_t tone) override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE, int_t(type), int_t(tone));
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE_OFF);
    }

    void SetNoise(uint_t val) override
    {
      Patterns.GetChannel().AddCommand(NOISE, val);
    }

    void SetSlide(uint_t step) override
    {
      Patterns.GetChannel().AddCommand(SLIDE, int_t(step));
    }

    void SetNoteSlide(uint_t step) override
    {
      Patterns.GetChannel().AddCommand(SLIDE_NOTE, int_t(step));
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

  template<class Object>
  class ObjectLinesIterator
  {
  public:
    ObjectLinesIterator()
      : Obj()
    {}

    void Set(const Object& obj)
    {
      // do not reset for original position update algo
      Obj = &obj;
    }

    void Reset()
    {
      Position = 0;
    }

    void Disable()
    {
      Position = Obj->GetSize();
    }

    const typename Object::Line* GetLine() const
    {
      return Position < Obj->GetSize() ? &Obj->GetLine(Position) : nullptr;
    }

    void Next()
    {
      if (++Position == Obj->GetSize())
      {
        Position = Obj->GetLoop();
      }
    }

  private:
    const Object* Obj;
    uint_t Position = 0;
  };

  class Accumulator
  {
  public:
    Accumulator() = default;

    void Reset()
    {
      Value = 0;
    }

    int_t Update(int_t delta, bool accumulate)
    {
      const int_t res = Value + delta;
      if (accumulate)
      {
        Value = res;
      }
      return res;
    }

  private:
    int_t Value = 0;
  };

  class ToneSlider
  {
  public:
    ToneSlider() = default;

    void SetSlide(int_t slide)
    {
      Sliding = slide;
    }

    void SetGlissade(int_t gliss)
    {
      Glissade = gliss;
    }

    void SetSlideDirection(int_t direction)
    {
      Direction = direction;
    }

    int_t Update()
    {
      Sliding += Glissade;
      if ((Direction > 0 && Sliding >= 0) || (Direction < 0 && Sliding < 0))
      {
        Sliding = 0;
        Glissade = 0;
      }
      return Sliding;
    }

    void Reset()
    {
      Sliding = Glissade = Direction = 0;
    }

  private:
    int_t Sliding = 0;
    int_t Glissade = 0;
    int_t Direction = 0;
  };

  struct ChannelState
  {
    ChannelState() = default;

    uint_t Note = 0;
    uint_t Envelope = 0;
    bool EnvelopeEnabled = false;
    uint_t Volume = 15;
    int_t VolumeSlide = 0;
    int_t Noise = 0;
    ObjectLinesIterator<Sample> SampleIterator;
    ObjectLinesIterator<Ornament> OrnamentIterator;
    Accumulator NoteAccumulator;
    Accumulator ToneAccumulator;
    Accumulator NoiseAccumulator;
    Accumulator SampleNoiseAccumulator;
    Accumulator EnvelopeAccumulator;
    int_t ToneAddon = 0;
    ToneSlider ToneSlide;
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
      const Sample& stubSample = Data->Samples.Get(0);
      const Ornament& stubOrnament = Data->Ornaments.Get(0);
      for (auto& dst : PlayerState)
      {
        dst = ChannelState();
        dst.SampleIterator.Set(stubSample);
        dst.SampleIterator.Disable();
        dst.OrnamentIterator.Set(stubOrnament);
        dst.OrnamentIterator.Reset();
      }
      Transposition = 0;
    }

    void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track) override
    {
      if (0 == state.Quirk())
      {
        if (0 == state.Line())
        {
          Transposition = Data->Order->GetTransposition(state.Position());
        }
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

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        if (*enabled)
        {
          dst.SampleIterator.Reset();
        }
        else
        {
          dst.SampleIterator.Disable();
        }
        dst.SampleNoiseAccumulator.Reset();
        dst.VolumeSlide = 0;
        dst.NoiseAccumulator.Reset();
        dst.NoteAccumulator.Reset();
        dst.OrnamentIterator.Reset();
        dst.ToneAccumulator.Reset();
        dst.EnvelopeAccumulator.Reset();
        dst.ToneSlide.Reset();
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note + Transposition;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleIterator.Set(Data->Samples.Get(*sample));
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentIterator.Set(Data->Ornaments.Get(*ornament));
        dst.OrnamentIterator.Reset();
        dst.NoiseAccumulator.Reset();
        dst.NoteAccumulator.Reset();
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
          dst.Envelope = it->Param2;
          dst.EnvelopeEnabled = true;
          break;
        case ENVELOPE_OFF:
          dst.EnvelopeEnabled = false;
          break;
        case NOISE:
          dst.Noise = it->Param1;
          break;
        case SLIDE:
          dst.ToneSlide.SetGlissade(it->Param1);
          break;
        case SLIDE_NOTE:
        {
          const int_t slide = track.GetSlidingDifference(it->Param2, dst.Note);
          const int_t gliss = slide >= 0 ? -it->Param1 : it->Param1;
          const int_t direction = slide >= 0 ? -1 : +1;
          dst.ToneSlide.SetSlide(slide);
          dst.ToneSlide.SetGlissade(gliss);
          dst.ToneSlide.SetSlideDirection(direction);
          dst.Note = it->Param2 + Transposition;
        }
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

    static void SynthesizeChannel(ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      const Ornament::Line& curOrnamentLine = *dst.OrnamentIterator.GetLine();
      const int_t noteAddon = dst.NoteAccumulator.Update(curOrnamentLine.NoteAddon, curOrnamentLine.KeepNoteAddon);
      const int_t noiseAddon = dst.NoiseAccumulator.Update(curOrnamentLine.NoiseAddon, curOrnamentLine.KeepNoiseAddon);
      dst.OrnamentIterator.Next();

      if (const Sample::Line* curSampleLine = dst.SampleIterator.GetLine())
      {
        // apply noise
        const int_t sampleNoiseAddon =
            dst.SampleNoiseAccumulator.Update(curSampleLine->Noise, curSampleLine->AccumulateNoise);
        if (curSampleLine->NoiseMask)
        {
          channel.DisableNoise();
        }
        else
        {
          track.SetNoise(dst.Noise + noiseAddon + sampleNoiseAddon);
        }
        // apply tone
        dst.ToneAddon = dst.ToneAccumulator.Update(curSampleLine->Tone, curSampleLine->AccumulateTone);
        if (curSampleLine->ToneMask)
        {
          channel.DisableTone();
        }
        // apply level
        dst.VolumeSlide += curSampleLine->VolSlide;
        const auto volume = Math::Clamp<int_t>(curSampleLine->Level + dst.VolumeSlide, 0, 15);
        const uint_t level = ((dst.Volume * 17 + (dst.Volume > 7)) * volume + 128) / 256;
        channel.SetLevel(level);

        const uint_t envelope =
            dst.EnvelopeAccumulator.Update(curSampleLine->EnvelopeAddon, curSampleLine->AccumulateEnvelope);
        if (curSampleLine->EnableEnvelope && dst.EnvelopeEnabled)
        {
          channel.EnableEnvelope();
          track.SetEnvelopeTone(dst.Envelope - envelope);
        }

        dst.SampleIterator.Next();
      }
      else
      {
        channel.SetLevel(0);
        channel.DisableTone();
        channel.DisableNoise();
      }
      channel.SetTone(dst.Note + noteAddon, dst.ToneAddon + dst.ToneSlide.Update());
    }

  private:
    const ModuleData::Ptr Data;
    std::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
    int_t Transposition = 0;
  };

  class Factory : public AYM::Factory
  {
  public:
    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData,
                                      Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Formats::Chiptune::FastTracker::Parse(rawData, dataBuilder))
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
}  // namespace Module::FastTracker
