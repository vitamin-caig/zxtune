/**
 *
 * @file
 *
 * @brief  DigitalMusicMaker chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/digitalmusicmaker.h"

#include "devices/dac/sample_factories.h"
#include "module/players/dac/dac_properties_helper.h"
#include "module/players/dac/dac_simple.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"
#include "module/players/tracking.h"

#include "binary/container.h"
#include "parameters/container.h"

#include "make_ptr.h"

#include <array>

namespace Module::DigitalMusicMaker
{
  const std::size_t CHANNELS_COUNT = 3;

  const uint_t Z80_FREQ = 3500000;
  // 119+116+111+10=356 ticks/out cycle = 9831 outs/sec (AY)
  const uint_t TICKS_PER_CYCLE = 119 + 116 + 111 + 10;
  // C-1 step 44/256 32.7Hz = ~1689 samples/sec
  const uint_t C_1_STEP = 44;
  const uint_t SAMPLES_FREQ = Z80_FREQ * C_1_STEP / TICKS_PER_CYCLE / 256;
  const uint_t RENDERS_PER_SEC = Z80_FREQ / TICKS_PER_CYCLE;

  inline int_t StepToHz(int_t step)
  {
    // C-1 frequency is 32.7Hz
    // step * 32.7 / c-1_step
    return step * 3270 / int_t(C_1_STEP * 100);
  }

  // per-channel commands and their parameters
  // Presentation: distinct notation commands get distinct values; don't pack variants.
  enum CmdType
  {
    // no parameters
    EMPTY_CMD,
    // direction
    FREQ_FLOAT,
    // step
    FREQ_FLOAT_PARAMS,
    // no parameters
    VIBRATO,
    // step, period
    VIBRATO_PARAMS,
    // no parameters
    ARPEGGIO,
    // step, period
    ARPEGGIO_PARAMS,
    // direction
    TONE_SLIDE,
    // step, period
    TONE_SLIDE_PARAMS,
    // no parameters
    DOUBLE_NOTE,
    // period
    DOUBLE_NOTE_PARAMS,
    // no parameters
    VOL_ATTACK,
    // limit, period
    VOL_ATTACK_PARAMS,
    // no parameters
    VOL_DECAY,
    // limit, period
    VOL_DECAY_PARAMS,
    // index
    MIX_SAMPLE
  };

  class ModuleData : public DAC::SimpleModuleData
  {
  public:
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;

    ModuleData()
      : DAC::SimpleModuleData(CHANNELS_COUNT)
    {}

    struct MixedChannel
    {
      MutableCell Mixin;
      uint_t Period = 0;

      MixedChannel() = default;
    };

    std::unique_ptr<MutableCommandsPool> MixinCommands = std::make_unique<MutableCommandsPool>();
    std::array<MixedChannel, 64> Mixes;
  };

  class ChannelBuilder : public Formats::Chiptune::DigitalMusicMaker::ChannelBuilder
  {
  public:
    explicit ChannelBuilder(MutableCell& cell)
      : Target(cell)
    {}

    void SetRest() override
    {
      Target.SetEnabled(false);
    }

    void SetNote(uint_t note) override
    {
      Target.SetEnabled(true);
      Target.SetNote(note);
    }

    void SetSample(uint_t sample) override
    {
      Target.SetSample(sample);
    }

    void SetVolume(uint_t volume) override
    {
      Target.SetVolume(volume);
    }

    void SetFloat(int_t direction) override
    {
      Target.GetCommands().AddOrUpdate(FREQ_FLOAT, direction);
    }

    void SetFloatParam(uint_t step) override
    {
      Target.GetCommands().AddOrUpdate(FREQ_FLOAT_PARAMS, step);
    }

    void SetVibrato() override
    {
      Target.GetCommands().AddOrUpdate(VIBRATO);
    }

    void SetVibratoParams(uint_t step, uint_t period) override
    {
      Target.GetCommands().AddOrUpdate(VIBRATO_PARAMS, period, step);
    }

    void SetArpeggio() override
    {
      Target.GetCommands().AddOrUpdate(ARPEGGIO);
    }

    void SetArpeggioParams(uint_t step, uint_t period) override
    {
      Target.GetCommands().AddOrUpdate(ARPEGGIO_PARAMS, period, step);
    }

    void SetSlide(int_t direction) override
    {
      Target.GetCommands().AddOrUpdate(TONE_SLIDE, direction);
    }

    void SetSlideParams(uint_t step, uint_t period) override
    {
      Target.GetCommands().AddOrUpdate(TONE_SLIDE_PARAMS, period, step);
    }

    void SetDoubleNote() override
    {
      Target.GetCommands().AddOrUpdate(DOUBLE_NOTE);
    }

    void SetDoubleNoteParam(uint_t period) override
    {
      Target.GetCommands().AddOrUpdate(DOUBLE_NOTE_PARAMS, period);
    }

    void SetVolumeAttack() override
    {
      Target.GetCommands().AddOrUpdate(VOL_ATTACK);
    }

    void SetVolumeAttackParams(uint_t limit, uint_t period) override
    {
      Target.GetCommands().AddOrUpdate(VOL_ATTACK_PARAMS, limit, period);
    }

    void SetVolumeDecay() override
    {
      Target.GetCommands().AddOrUpdate(VOL_DECAY);
    }

    void SetVolumeDecayParams(uint_t limit, uint_t period) override
    {
      Target.GetCommands().AddOrUpdate(VOL_DECAY_PARAMS, limit, period);
    }

    void SetMixSample(uint_t idx) override
    {
      Target.GetCommands().AddOrUpdate(MIX_SAMPLE, idx);
    }

    void SetNoEffects() override
    {
      Target.GetCommands().AddOrUpdate(EMPTY_CMD);
    }

  private:
    MutableCell& Target;
  };

  class DataBuilder : public Formats::Chiptune::DigitalMusicMaker::Builder
  {
  public:
    explicit DataBuilder(DAC::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetSamplesFrequency(SAMPLES_FREQ);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, std::size_t loop, Binary::View sample) override
    {
      Data->Samples.Add(index, Devices::DAC::CreateU4PackedSample(sample, loop));
    }

    std::unique_ptr<Formats::Chiptune::DigitalMusicMaker::ChannelBuilder> SetSampleMixin(uint_t index,
                                                                                         uint_t period) override
    {
      auto& dst = Data->Mixes[index];
      dst.Period = period;
      dst.Mixin.SetCommandsPool(Data->MixinCommands.get());
      return std::unique_ptr<Formats::Chiptune::DigitalMusicMaker::ChannelBuilder>(new ChannelBuilder(dst.Mixin));
    }

    void SetPositions(Formats::Chiptune::DigitalMusicMaker::Positions positions) override
    {
      Data->Order = MakePtr<SimpleOrderList>(positions.Loop, std::move(positions.Lines));
    }

    Formats::Chiptune::PatternBuilder& StartPattern(uint_t index) override
    {
      Patterns.SetPattern(index);
      return Patterns;
    }

    std::unique_ptr<Formats::Chiptune::DigitalMusicMaker::ChannelBuilder> StartChannel(uint_t index) override
    {
      Patterns.SetChannel(index);
      return std::unique_ptr<Formats::Chiptune::DigitalMusicMaker::ChannelBuilder>(
          new ChannelBuilder(Patterns.GetChannel()));
    }

    ModuleData::Ptr CaptureResult()
    {
      Data->Patterns = Patterns.CaptureResult();
      return std::move(Data);
    }

  private:
    DAC::PropertiesHelper& Properties;
    MetaProperties Meta;
    PatternsBuilder Patterns;
    ModuleData::RWPtr Data;
  };

  class ChannelState
  {
  public:
    ChannelState()
      // values are from player' defaults
      : Effect(&ChannelState::NoEffect)
    {}

    void OnFrame(DAC::ChannelDataBuilder& builder)
    {
      (this->*Effect)(builder);
    }

    void OnNote(const Cell& src, const ModuleData& data, DAC::ChannelDataBuilder& builder)
    {
      // if has new sample, start from it, else use previous sample
      const uint_t oldPos = src.GetSample() ? 0 : builder.GetState().PosInSample;
      ParseNote(src, builder);
      if (src.GetCommands().empty())
      {
        return;
      }
      OldData = src;
      for (const auto& cmd : src.GetCommands())
      {
        switch (cmd.Type)
        {
        case EMPTY_CMD:
          DisableEffect();
          break;
        case FREQ_FLOAT:
          Effect = &ChannelState::FreqFloat;
          FreqSlideStep = static_cast<int8_t>(cmd.Param1);
          break;
        case FREQ_FLOAT_PARAMS:
          FreqSlideStep *= cmd.Param1;
          break;
        case VIBRATO:
          Effect = &ChannelState::Vibrato;
          break;
        case VIBRATO_PARAMS:
          VibratoStep = cmd.Param2;
          VibratoPeriod = cmd.Param1;
          break;
        case ARPEGGIO:
          Effect = &ChannelState::Arpeggio;
          break;
        case ARPEGGIO_PARAMS:
          ArpeggioStep = cmd.Param2;
          ArpeggioPeriod = cmd.Param1;
          break;
        case TONE_SLIDE:
          Effect = &ChannelState::NoteFloat;
          NoteSlideStep = static_cast<int8_t>(cmd.Param1);
          break;
        case TONE_SLIDE_PARAMS:
          NoteSlideStep *= cmd.Param2;
          NoteSlidePeriod = cmd.Param1;
          break;
        case DOUBLE_NOTE:
          Effect = &ChannelState::DoubleNote;
          break;
        case DOUBLE_NOTE_PARAMS:
          NoteDoublePeriod = cmd.Param1;
          break;
        case VOL_ATTACK:
          Effect = &ChannelState::Attack;
          break;
        case VOL_ATTACK_PARAMS:
          AttackLimit = cmd.Param1;
          AttackPeriod = cmd.Param2;
          break;
        case VOL_DECAY:
          Effect = &ChannelState::Decay;
          break;
        case VOL_DECAY_PARAMS:
          DecayLimit = cmd.Param1;
          DecayPeriod = cmd.Param2;
          break;
        case MIX_SAMPLE:
        {
          DacState = builder.GetState();
          DacState.PosInSample = oldPos;
          const auto& mix = data.Mixes[cmd.Param1];
          ParseNote(mix.Mixin, builder);
          MixPeriod = mix.Period;
          Effect = &ChannelState::Mix;
        }
        break;
        default:
          break;
        }
      }
    }

    void GetState(DAC::ChannelDataBuilder& builder) const
    {
      builder.SetNote(Note);
      builder.SetNoteSlide(NoteSlide);
      builder.SetFreqSlideHz(StepToHz(FreqSlide));
      builder.SetSampleNum(Sample);
      builder.SetLevelInPercents(Volume * 100 / 15);
    }

  private:
    void ParseNote(const Cell& src, DAC::ChannelDataBuilder& builder)
    {
      if (const uint_t* note = src.GetNote())
      {
        Counter = 0;
        VibratoStep = ArpeggioStep = 0;
        Note = *note;
        NoteSlide = FreqSlide = 0;
      }
      if (const uint_t* volume = src.GetVolume())
      {
        Volume = *volume;
      }
      if (const bool* enabled = src.GetEnabled())
      {
        builder.SetEnabled(*enabled);
        if (!*enabled)
        {
          NoteSlide = FreqSlide = 0;
        }
      }
      if (const uint_t* sample = src.GetSample())
      {
        Sample = *sample;
        builder.SetPosInSample(0);
      }
      else
      {
        builder.DropPosInSample();
      }
    }

    void NoEffect(DAC::ChannelDataBuilder& /*builder*/) {}

    void FreqFloat(DAC::ChannelDataBuilder& /*builder*/)
    {
      SlideFreq(FreqSlideStep);
    }

    void Vibrato(DAC::ChannelDataBuilder& /*builder*/)
    {
      if (Step(VibratoPeriod))
      {
        VibratoStep = -VibratoStep;
        SlideFreq(VibratoStep);
      }
    }

    void Arpeggio(DAC::ChannelDataBuilder& /*builder*/)
    {
      if (Step(ArpeggioPeriod))
      {
        ArpeggioStep = -ArpeggioStep;
        NoteSlide += ArpeggioStep;
        FreqSlide = 0;
      }
    }

    void NoteFloat(DAC::ChannelDataBuilder& /*builder*/)
    {
      if (Step(NoteSlidePeriod))
      {
        NoteSlide += NoteSlideStep;
        FreqSlide = 0;
      }
    }

    void DoubleNote(DAC::ChannelDataBuilder& builder)
    {
      if (Step(NoteDoublePeriod))
      {
        ParseNote(OldData, builder);
        DisableEffect();
      }
    }

    void Attack(DAC::ChannelDataBuilder& /*builder*/)
    {
      if (Step(AttackPeriod))
      {
        ++Volume;
        if (AttackLimit < Volume)
        {
          --Volume;
          DisableEffect();
        }
      }
    }

    void Decay(DAC::ChannelDataBuilder& /*builder*/)
    {
      if (Step(DecayPeriod))
      {
        --Volume;
        if (DecayLimit > Volume)
        {
          ++Volume;
          DisableEffect();
        }
      }
    }

    void Mix(DAC::ChannelDataBuilder& builder)
    {
      if (Step(MixPeriod))
      {
        ParseNote(OldData, builder);
        // restore a6ll
        const uint_t prevStep = GetStep() + FreqSlide;
        const uint_t FPS = 50;  // TODO
        const uint_t skipped = MixPeriod * prevStep * RENDERS_PER_SEC / FPS / 256;
        auto& dst = builder.GetState();
        dst = DacState;
        builder.SetPosInSample(dst.PosInSample + skipped);

        DisableEffect();
      }
    }

    bool Step(uint_t period)
    {
      if (++Counter == period)
      {
        Counter = 0;
        return true;
      }
      return false;
    }

    void SlideFreq(int_t step)
    {
      const int_t nextStep = GetStep() + FreqSlide + step;
      if (nextStep <= 0 || nextStep >= 0x0c00)
      {
        DisableEffect();
      }
      else
      {
        FreqSlide += step;
        NoteSlide = 0;
      }
    }

    uint_t GetStep() const
    {
      static const uint_t STEPS[] = {44,  47,  50,  53,  56,  59,  63,  66,  70,  74,   79,   83,   88,   94,   99,
                                     105, 111, 118, 125, 133, 140, 149, 158, 167, 177,  187,  199,  210,  223,  236,
                                     250, 265, 281, 297, 315, 334, 354, 375, 397, 421,  446,  472,  500,  530,  561,
                                     595, 630, 668, 707, 749, 794, 841, 891, 944, 1001, 1060, 1123, 1189, 1216, 1335};
      return STEPS[Note + NoteSlide];
    }

    void DisableEffect()
    {
      Effect = &ChannelState::NoEffect;
    }

  private:
    int_t FreqSlideStep = 1;

    uint_t VibratoPeriod = 4;  // VBT_x
    int_t VibratoStep = 3;     // VBF_x * VBA1/VBA2

    uint_t ArpeggioPeriod = 1;  // APT_x
    int_t ArpeggioStep = 18;    // APF_x * APA1/APA2

    uint_t NoteSlidePeriod = 2;  // SUT_x/SDT_x
    int_t NoteSlideStep = 12;

    uint_t NoteDoublePeriod = 3;  // DUT_x

    uint_t AttackPeriod = 1;  // ATT_x
    uint_t AttackLimit = 15;  // ATL_x

    uint_t DecayPeriod = 1;  // DYT_x
    uint_t DecayLimit = 1;   // DYL_x

    uint_t MixPeriod = 3;

    uint_t Counter = 0;  // COUN_x
    uint_t Note = 0;     // NOTN_x
    uint_t NoteSlide = 0;
    uint_t FreqSlide = 0;
    uint_t Volume = 15;  // pVOL_x
    uint_t Sample = 0;

    Cell OldData;
    Devices::DAC::ChannelData DacState;

    using EffectFunc = void (ChannelState::*)(DAC::ChannelDataBuilder&);
    EffectFunc Effect;
  };

  class DataRenderer
  {
  public:
    explicit DataRenderer(const ModuleData& data)
      : Data(data)
    {
      Reset();
    }

    void Reset()
    {
      std::fill(Chans.begin(), Chans.end(), ChannelState());
    }

    void SynthesizeData(const TrackState& state, DAC::TrackBuilder& track)
    {
      const auto* const line = 0 == state.Quirk ? Data.GetLine(state) : nullptr;
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        auto builder = track.GetChannel(chan);

        auto& chanState = Chans[chan];
        chanState.OnFrame(builder);
        // begin note
        if (line)
        {
          if (const auto* const src = line->GetChannel(chan))
          {
            chanState.OnNote(*src, Data, builder);
          }
        }
        chanState.GetState(builder);
      }
    }

  private:
    const ModuleData& Data;
    std::array<ChannelState, CHANNELS_COUNT> Chans;
  };

}  // namespace Module::DigitalMusicMaker

namespace Module::DAC
{
  Chiptune::Ptr CreateDigitalMusicMakerChiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    props.SetChannels(DigitalMusicMaker::CHANNELS_COUNT);
    DigitalMusicMaker::DataBuilder dataBuilder(props);
    if (const auto container = Formats::Chiptune::DigitalMusicMaker::Parse(rawData, dataBuilder))
    {
      props.SetSource(*container);
      return CreateTrackingChiptune<DigitalMusicMaker::DataRenderer>(dataBuilder.CaptureResult());
    }
    return {};
  }
}  // namespace Module::DAC
