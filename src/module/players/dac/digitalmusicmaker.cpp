/**
* 
* @file
*
* @brief  DigitalMusicMaker chiptune factory
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "digitalmusicmaker.h"
#include "dac_properties_helper.h"
#include "dac_simple.h"
//common includes
#include <make_ptr.h>
//library includes
#include <devices/dac/sample_factories.h>
#include <formats/chiptune/digital/digitalmusicmaker.h>
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>
#include <module/players/tracking.h>

namespace Module
{
namespace DigitalMusicMaker
{
  const std::size_t CHANNELS_COUNT = 3;

  const uint64_t Z80_FREQ = 3500000;
  //119+116+111+10=356 ticks/out cycle = 9831 outs/sec (AY)
  const uint_t TICKS_PER_CYCLE = 119 + 116 + 111 + 10;
  //C-1 step 44/256 32.7Hz = ~1689 samples/sec
  const uint_t C_1_STEP = 44;
  const uint_t SAMPLES_FREQ = Z80_FREQ * C_1_STEP / TICKS_PER_CYCLE / 256;
  const uint_t RENDERS_PER_SEC = Z80_FREQ / TICKS_PER_CYCLE;

  inline int_t StepToHz(int_t step)
  {
    //C-1 frequency is 32.7Hz
    //step * 32.7 / c-1_step
    return step * 3270 / int_t(C_1_STEP * 100);
  }

  //supported tracking commands
  enum CmdType
  {
    //no parameters
    EMPTY_CMD,
    //2 param: direction, step
    FREQ_FLOAT,
    //3 params: isApply, step, period
    VIBRATO,
    //3 params: isApply, step, period
    ARPEGGIO,
    //3 param: direction, step, period
    TONE_SLIDE,
    //2 params: isApply, period
    DOUBLE_NOTE,
    //3 params: isApply, limit, period
    VOL_ATTACK,
    //3 params: isApply, limit, period
    VOL_DECAY,
    //1 param
    MIX_SAMPLE,
  };

  class ModuleData : public DAC::SimpleModuleData
  {
  public:
    typedef std::shared_ptr<const ModuleData> Ptr;
    typedef std::shared_ptr<ModuleData> RWPtr;

    struct MixedChannel
    {
      MutableCell Mixin;
      uint_t Period;

      MixedChannel()
        : Period()
      {
      }
    };

    boost::array<MixedChannel, 64> Mixes;
  };

  class ChannelBuilder : public Formats::Chiptune::DigitalMusicMaker::ChannelBuilder
  {
  public:
    explicit ChannelBuilder(MutableCell& cell)
      : Target(cell)
    {
    }

    virtual void SetRest()
    {
      Target.SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      Target.SetEnabled(true);
      Target.SetNote(note);
    }

    virtual void SetSample(uint_t sample)
    {
      Target.SetSample(sample);
    }

    virtual void SetVolume(uint_t volume)
    {
      Target.SetVolume(volume);
    }

    virtual void SetFloat(int_t direction)
    {
      Target.AddCommand(FREQ_FLOAT, direction);
    }

    virtual void SetFloatParam(uint_t step)
    {
      Target.AddCommand(FREQ_FLOAT, 0, step);
    }

    virtual void SetVibrato()
    {
      Target.AddCommand(VIBRATO, true);
    }

    virtual void SetVibratoParams(uint_t step, uint_t period)
    {
      Target.AddCommand(VIBRATO, false, step, period);
    }

    virtual void SetArpeggio()
    {
      Target.AddCommand(ARPEGGIO, true);
    }

    virtual void SetArpeggioParams(uint_t step, uint_t period)
    {
      Target.AddCommand(ARPEGGIO, false, step, period);
    }

    virtual void SetSlide(int_t direction)
    {
      Target.AddCommand(TONE_SLIDE, direction);
    }

    virtual void SetSlideParams(uint_t step, uint_t period)
    {
      Target.AddCommand(TONE_SLIDE, 0, step, period);
    }

    virtual void SetDoubleNote()
    {
      Target.AddCommand(DOUBLE_NOTE, true);
    }

    virtual void SetDoubleNoteParam(uint_t period)
    {
      Target.AddCommand(DOUBLE_NOTE, false, period);
    }

    virtual void SetVolumeAttack()
    {
      Target.AddCommand(VOL_ATTACK, true);
    }

    virtual void SetVolumeAttackParams(uint_t limit, uint_t period)
    {
      Target.AddCommand(VOL_ATTACK, false, limit, period);
    }

    virtual void SetVolumeDecay()
    {
      Target.AddCommand(VOL_DECAY, true);
    }

    virtual void SetVolumeDecayParams(uint_t limit, uint_t period)
    {
      Target.AddCommand(VOL_DECAY, false, limit, period);
    }

    virtual void SetMixSample(uint_t idx)
    {
      Target.AddCommand(MIX_SAMPLE, idx);
    }

    virtual void SetNoEffects()
    {
      Target.AddCommand(EMPTY_CMD);
    }
  private:
    MutableCell& Target;
  };

  class DataBuilder : public Formats::Chiptune::DigitalMusicMaker::Builder
  {
  public:
    explicit DataBuilder(DAC::PropertiesHelper& props)
      : Data(MakeRWPtr<ModuleData>())
      , Properties(props)
      , Meta(props)
      , Patterns(PatternsBuilder::Create<CHANNELS_COUNT>())
    {
      Data->Patterns = Patterns.GetResult();
      Properties.SetSamplesFrequency(SAMPLES_FREQ);
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return Meta;
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr sample)
    {
      Data->Samples.Add(index, Devices::DAC::CreateU4PackedSample(sample, loop));
    }

    virtual std::unique_ptr<Formats::Chiptune::DigitalMusicMaker::ChannelBuilder> SetSampleMixin(uint_t index, uint_t period)
    {
      ModuleData::MixedChannel& dst = Data->Mixes[index];
      dst.Period = period;
      return std::unique_ptr<Formats::Chiptune::DigitalMusicMaker::ChannelBuilder>(new ChannelBuilder(dst.Mixin));
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      Data->Order = MakePtr<SimpleOrderList>(loop, positions.begin(), positions.end());
    }

    virtual Formats::Chiptune::PatternBuilder& StartPattern(uint_t index)
    {
      Patterns.SetPattern(index);
      return Patterns;
    }

    virtual std::unique_ptr<Formats::Chiptune::DigitalMusicMaker::ChannelBuilder> StartChannel(uint_t index)
    {
      Patterns.SetChannel(index);
      return std::unique_ptr<Formats::Chiptune::DigitalMusicMaker::ChannelBuilder>(new ChannelBuilder(Patterns.GetChannel()));
    }

    ModuleData::Ptr GetResult() const
    {
      return Data;
    }
  private:
    const ModuleData::RWPtr Data;
    DAC::PropertiesHelper& Properties;
    MetaProperties Meta;
    PatternsBuilder Patterns;
  };

  class ChannelState
  {
  public:
    ChannelState()
      //values are from player' defaults
      : FreqSlideStep(1)
      , VibratoPeriod(4)
      , VibratoStep(3)
      , ArpeggioPeriod(1)
      , ArpeggioStep(18)
      , NoteSlidePeriod(2)
      , NoteSlideStep(12)
      , NoteDoublePeriod(3)
      , AttackPeriod(1)
      , AttackLimit(15)
      , DecayPeriod(1)
      , DecayLimit(1)
      , MixPeriod(3)
      , Counter(0)
      , Note(0)
      , NoteSlide(0)
      , FreqSlide(0)
      , Volume(15)
      , Sample(0)
      , Effect(&ChannelState::NoEffect)
    {
    }

    void OnFrame(DAC::ChannelDataBuilder& builder)
    {
      (this->*Effect)(builder);
    }

    void OnNote(const Cell& src, const ModuleData& data, DAC::ChannelDataBuilder& builder)
    {
      //if has new sample, start from it, else use previous sample
      const uint_t oldPos = src.GetSample() ? 0 : builder.GetState().PosInSample;
      ParseNote(src, builder);
      CommandsIterator it = src.GetCommands();
      if (!it)
      {
        return;
      }
      OldData = src;
      for (; it; ++it)
      {
        switch (it->Type)
        {
        case EMPTY_CMD:
          DisableEffect();
          break;
        case FREQ_FLOAT:
          if (it->Param1)
          {
            Effect = &ChannelState::FreqFloat;
            FreqSlideStep = it->Param1;
          }
          else
          {
            FreqSlideStep *= it->Param2;
          }
          break;
        case VIBRATO:
          if (it->Param1)
          {
            Effect = &ChannelState::Vibrato;
          }
          else
          {
            VibratoStep = it->Param2;
            VibratoPeriod = it->Param3;
          }
          break;
        case ARPEGGIO:
          if (it->Param1)
          {
            Effect = &ChannelState::Arpeggio;
          }
          else
          {
            ArpeggioStep = it->Param2;
            ArpeggioPeriod = it->Param3;
          }
          break;
        case TONE_SLIDE:
          if (it->Param1)
          {
            Effect = &ChannelState::NoteFloat;
            NoteSlideStep = it->Param1;
          }
          else
          {
            NoteSlideStep *= it->Param2;
            NoteSlidePeriod = it->Param3;
          }
          break;
        case DOUBLE_NOTE:
          if (it->Param1)
          {
            Effect = &ChannelState::DoubleNote;
          }
          else
          {
            NoteDoublePeriod = it->Param2;
          }
          break;
        case VOL_ATTACK:
          if (it->Param1)
          {
            Effect = &ChannelState::Attack;
          }
          else
          {
            AttackLimit = it->Param2;
            AttackPeriod = it->Param3;
          }
          break;
        case VOL_DECAY:
          if (it->Param1)
          {
            Effect = &ChannelState::Decay;
          }
          else
          {
            DecayLimit = it->Param2;
            DecayPeriod = it->Param3;
          }
          break;
        case MIX_SAMPLE:
          {
            DacState = builder.GetState();
            DacState.PosInSample = oldPos;
            const ModuleData::MixedChannel& mix = data.Mixes[it->Param1];
            ParseNote(mix.Mixin, builder);
            MixPeriod = mix.Period;
            Effect = &ChannelState::Mix;
          }
          break;
        }
      }
    }

    void GetState(DAC::ChannelDataBuilder& builder)
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

    void NoEffect(DAC::ChannelDataBuilder& /*builder*/)
    {
    }

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
        //restore a6ll
        const uint_t prevStep = GetStep() + FreqSlide;
        const uint_t FPS = 50;//TODO
        const uint_t skipped = MixPeriod * prevStep * RENDERS_PER_SEC / FPS / 256;
        Devices::DAC::ChannelData& dst = builder.GetState();
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
      static const uint_t STEPS[] =
      {
        44, 47, 50, 53, 56, 59, 63, 66, 70, 74, 79, 83,
        88, 94, 99, 105, 111, 118, 125, 133, 140, 149, 158, 167,
        177, 187, 199, 210, 223, 236, 250, 265, 281, 297, 315, 334,
        354, 375, 397, 421, 446, 472, 500, 530, 561, 595, 630, 668,
        707, 749, 794, 841, 891, 944, 1001, 1060, 1123, 1189, 1216, 1335
      };
      return STEPS[Note + NoteSlide];
    }

    void DisableEffect()
    {
      Effect = &ChannelState::NoEffect;
    }
  private:
    int_t FreqSlideStep;

    uint_t VibratoPeriod;//VBT_x
    int_t VibratoStep;//VBF_x * VBA1/VBA2
    
    uint_t ArpeggioPeriod;//APT_x
    int_t ArpeggioStep;//APF_x * APA1/APA2

    uint_t NoteSlidePeriod;//SUT_x/SDT_x
    int_t NoteSlideStep;

    uint_t NoteDoublePeriod;//DUT_x

    uint_t AttackPeriod;//ATT_x
    uint_t AttackLimit;//ATL_x

    uint_t DecayPeriod;//DYT_x
    uint_t DecayLimit;//DYL_x

    uint_t MixPeriod;

    uint_t Counter;//COUN_x
    uint_t Note;  //NOTN_x
    uint_t NoteSlide;
    uint_t FreqSlide;
    uint_t Volume;//pVOL_x
    uint_t Sample;

    Cell OldData;
    Devices::DAC::ChannelData DacState;

    typedef void (ChannelState::*EffectFunc)(DAC::ChannelDataBuilder&);
    EffectFunc Effect;
  };

  class DataRenderer : public DAC::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(data)
    {
      Reset();
    }

    virtual void Reset()
    {
      std::fill(Chans.begin(), Chans.end(), ChannelState());
    }

    virtual void SynthesizeData(const TrackModelState& state, DAC::TrackBuilder& track)
    {
      const Line::Ptr line = 0 == state.Quirk() ? state.LineObject() : Line::Ptr();
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        DAC::ChannelDataBuilder builder = track.GetChannel(chan);

        ChannelState& chanState = Chans[chan];
        chanState.OnFrame(builder);
        //begin note
        if (line)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            chanState.OnNote(*src, *Data, builder);
          }
        }
        chanState.GetState(builder);
      }
    }
  private:
    const ModuleData::Ptr Data;
    boost::array<ChannelState, CHANNELS_COUNT> Chans;
  };

  class Chiptune : public DAC::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, CHANNELS_COUNT))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual DAC::DataIterator::Ptr CreateDataIterator() const
    {
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const DAC::DataRenderer::Ptr renderer = MakePtr<DataRenderer>(Data);
      return DAC::CreateDataIterator(iterator, renderer);
    }

    virtual void GetSamples(Devices::DAC::Chip::Ptr chip) const
    {
      for (uint_t idx = 0, lim = Data->Samples.Size(); idx != lim; ++idx)
      {
        chip->SetSample(idx, Data->Samples.Get(idx));
      }
    }
  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
    const Information::Ptr Info;
  };

  class Factory : public DAC::Factory
  {
  public:
    virtual DAC::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const
    {
      DAC::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::DigitalMusicMaker::Parse(rawData, dataBuilder))
      {
        props.SetSource(*container);
        return MakePtr<Chiptune>(dataBuilder.GetResult(), properties);
      }
      else
      {
        return DAC::Chiptune::Ptr();
      }
    }
  };
  
  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}
}
