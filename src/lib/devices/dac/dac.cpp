#include "dac.h"

#include <tools.h>

#include <numeric>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::DAC;

  const int SILENT = 128;

  const std::size_t NOTES = 64;
  //table in Hz
  const double FREQ_TABLE[NOTES] = {
    //octave1
    32.70,  34.65,  36.71,  38.89,  41.20,  43.65,  46.25,  49.00,  51.91,  55.00,  58.27,  61.73,
    //octave2
    65.41,  69.29,  73.42,  77.78,  82.41,  87.30,  92.50,  98.00, 103.82, 110.00, 116.54, 123.46,
    //octave3
    130.82, 138.58, 146.84, 155.56, 164.82, 174.60, 185.00, 196.00, 207.64, 220.00, 233.08, 246.92,
    //octave4
    261.64, 277.16, 293.68, 311.12, 329.64, 349.20, 370.00, 392.00, 415.28, 440.00, 466.16, 493.84,
    //octave5
    523.28, 554.32, 587.36, 622.24, 659.28, 698.40, 740.00, 784.00, 830.56, 880.00, 932.32, 987.68,
    //octave6
    1046.50, 1108.60, 1174.70, 1244.50
  };

  template<class T>
  inline T sign(T val)
  {
    return val > 0 ? 1 : (val < 0 ? -1 : 0);
  }

  inline std::size_t GetStepByFrequency(double note, std::size_t soundFreq, std::size_t sampleFreq)
  {
    return static_cast<std::size_t>(note * Sound::FIXED_POINT_PRECISION *
      sampleFreq / (FREQ_TABLE[0] * soundFreq * 2));
  }

  inline uint32_t GainAdder(uint32_t sum, uint8_t sample)
  {
    return sum + abs(int(sample) - SILENT);
  }

  inline Sound::Sample scale(uint8_t inSample)
  {
    return inSample << (8 * (sizeof(Sound::Sample) - sizeof(inSample)) - 1);
  }

  struct Sample
  {
    Sample() : Size(1), Loop(1), Data(1), Gain(0)//safe sample
    {
    }

    Sample(const Dump& data, std::size_t loop)
      : Size(data.size()), Loop(loop), Data(data)
      , Gain(Sound::Analyze::LevelType(std::accumulate(&Data[0], &Data[0] + Size, uint32_t(0), GainAdder) / Size))
    {
    }

    std::size_t Size;
    std::size_t Loop;
    Dump Data;
    Sound::Analyze::LevelType Gain;
  };

  struct ChannelState
  {
    explicit ChannelState(const Sample& sample)
      : Enabled(), Note(), NoteSlide(), FreqSlide(), CurSample(&sample), PosInSample(), SampleStep(1)
    {
    }

    bool Enabled;
    std::size_t Note;
    signed NoteSlide;
    signed FreqSlide;
    const Sample* CurSample;
    std::size_t PosInSample;//in fixed point
    std::size_t SampleStep;

    std::size_t GetConstSteps() const
    {
      return (Sound::FIXED_POINT_PRECISION * (PosInSample / Sound::FIXED_POINT_PRECISION + 1) - PosInSample) /
        SampleStep;
    }

    void SkipConstSteps(std::size_t steps)
    {
      PosInSample += steps * SampleStep;
      const std::size_t pos(PosInSample / Sound::FIXED_POINT_PRECISION);
      assert(CurSample);
      if (pos >= CurSample->Size)
      {
        Enabled = false;
      }
    }

    Sound::Sample GetValue() const
    {
      if (Enabled)
      {
        const std::size_t pos(PosInSample / Sound::FIXED_POINT_PRECISION);
        assert(CurSample && pos < CurSample->Size);
        return scale(CurSample->Data[pos]);
      }
      else
      {
        return scale(SILENT);
      }
    }

    Sound::Analyze::Channel Analyze(Sound::Analyze::LevelType maxGain) const
    {
      Sound::Analyze::Channel result;
      if ((result.Enabled = Enabled))
      {
        result.Band = Note;
        assert(CurSample);
        result.Level = CurSample->Gain * std::numeric_limits<Sound::Analyze::LevelType>::max() / maxGain;
      }
      return result;
    }
  };

  class ChipImpl : public Chip
  {
  public:
    ChipImpl(std::size_t channels, std::size_t samples, std::size_t sampleFreq)
      : Samples(samples), MaxGain(0), CurrentTick(0), State(channels, ChannelState(Samples.front()))
      , SampleFreq(sampleFreq)
    {
    }

    /// Set sample for work
    virtual void SetSample(std::size_t idx, const Dump& data, std::size_t loop)
    {
      if (idx >= Samples.size())
        throw 1;//TODO
      assert(!Samples[idx].Gain);
      const Sample& res(Samples[idx] = Sample(data, loop));
      MaxGain = std::max(MaxGain, res.Gain);
    }

    /// render single data chunk
    virtual void RenderData(const Sound::Parameters& params,
      const DataChunk& src,
      Sound::Receiver& dst)
    {
      std::for_each(src.Channels.begin(), src.Channels.end(),
        boost::bind(&ChipImpl::UpdateState, this, _1));

      std::for_each(State.begin(), State.end(),
        boost::bind(&ChipImpl::CalcSampleStep, this, params.SoundFreq, _1));

      const uint64_t ticksPerSample(params.ClockFreq / params.SoundFreq);
      std::vector<Sound::Sample> result(State.size());

      std::vector<std::size_t> constSteps(State.size());
      while (CurrentTick < src.Tick)
      {
        std::transform(State.begin(), State.end(), result.begin(),
          std::mem_fun_ref(&ChannelState::GetValue));
        std::transform(State.begin(), State.end(), constSteps.begin(), 
          std::mem_fun_ref(&ChannelState::GetConstSteps));
        std::size_t safeSkips(1 + std::min(std::size_t((src.Tick - CurrentTick) / ticksPerSample),
          *std::min_element(constSteps.begin(), constSteps.end())));
        CurrentTick += safeSkips * ticksPerSample;
        std::for_each(State.begin(), State.end(), 
          std::bind2nd(std::mem_fun_ref(&ChannelState::SkipConstSteps), safeSkips));
        while (safeSkips--)
        {
          dst.ApplySample(&result[0], result.size());
        }
      }
    }

    virtual void GetState(Sound::Analyze::ChannelsState& state) const
    {
      state.resize(State.size());
      std::transform(State.begin(), State.end(), state.begin(),
        std::bind2nd(std::mem_fun_ref(&ChannelState::Analyze), MaxGain));
    }

    /// reset internal state to initial
    virtual void Reset()
    {
      //TODO
    }

  private:
    void UpdateState(const DataChunk::ChannelData& state)
    {
      if (state.Channel >= State.size())
      {
        throw 1;//TODO
      }
      ChannelState& chan(State[state.Channel]);
      if (state.Mask & DataChunk::ChannelData::MASK_ENABLED)
      {
        chan.Enabled = state.Enabled;
      }
      if (state.Mask & DataChunk::ChannelData::MASK_NOTE)
      {
        chan.Note = state.Note;
      }
      if (state.Mask & DataChunk::ChannelData::MASK_NOTESLIDE)
      {
        chan.NoteSlide = state.NoteSlide;
      }
      if (state.Mask & DataChunk::ChannelData::MASK_FREQSLIDE)
      {
        chan.FreqSlide = state.FreqSlideHz;
      }
      if (state.Mask & DataChunk::ChannelData::MASK_SAMPLE)
      {
        if (state.SampleNum >= Samples.size())
        {
          throw 1; //TODO
        }
        chan.CurSample = &Samples[state.SampleNum];
      }
      if (state.Mask & DataChunk::ChannelData::MASK_POSITION)
      {
        assert(chan.CurSample);
        chan.PosInSample = Sound::FIXED_POINT_PRECISION * std::min(state.PosInSample, chan.CurSample->Size - 1);
      }
    }

    void CalcSampleStep(std::size_t freq, ChannelState& state)
    {
      if (TableFreq != freq)
      {
        TableFreq = freq;
        std::transform(FREQ_TABLE, ArrayEnd(FREQ_TABLE), FreqTable.begin(),
          boost::bind(GetStepByFrequency, _1, TableFreq, SampleFreq));
        MaxNotes = std::distance(FreqTable.begin(), std::find(FreqTable.begin(), FreqTable.end(), 0));
      }
      const int toneStep = static_cast<int>(FreqTable[clamp<signed>(signed(state.Note) + state.NoteSlide, 
        0, MaxNotes - 1)]);
      state.SampleStep = state.FreqSlide ?
        clamp<int>(toneStep + sign(state.FreqSlide) * GetStepByFrequency(abs(state.FreqSlide),
          TableFreq, SampleFreq), int(FreqTable.front()), int(FreqTable.back()))
        :
        toneStep;
      assert(state.SampleStep);
    }
  private:
    std::vector<Sample> Samples;
    Sound::Analyze::LevelType MaxGain;
    uint64_t CurrentTick;
    std::vector<ChannelState> State;
    const std::size_t SampleFreq;
    //steps calc
    std::size_t TableFreq;
    std::size_t MaxNotes;
    boost::array<std::size_t, NOTES> FreqTable;
  };
}

namespace ZXTune
{
  namespace DAC
  {
    Chip::Ptr CreateChip(std::size_t channels, std::size_t samples, std::size_t sampleFreq)
    {
      return Chip::Ptr(new ChipImpl(channels, samples, sampleFreq));
    }
  }
}