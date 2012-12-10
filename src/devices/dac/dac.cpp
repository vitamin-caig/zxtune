/*
Abstract:
  DAC implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <devices/dac.h>
//common includes
#include <tools.h>
//library includes
#include <math/numeric.h>
//std includes
#include <cmath>
#include <limits>
#include <numeric>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace Devices::DAC;

  const uint_t FIXED_POINT_PRECISION = 256;

  const uint_t MAX_LEVEL = 100;

  const uint_t NOTES = 64;
  //table in Hz
  const boost::array<double, NOTES> FREQ_TABLE =
  {
    {
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
    1046.5, 1108.6, 1174.7, 1244.5/*, 1318.6, 1396.8, 1480.0, 1568.0, 1661.1, 1760.0, 1864.6, 1975.4*/
    }
  };

  template<class T>
  inline T sign(T val)
  {
    return val > 0 ? 1 : (val < 0 ? -1 : 0);
  }

  inline uint_t GetStepByFrequency(double note, uint_t soundFreq, uint_t sampleFreq)
  {
    return static_cast<uint_t>(note * FIXED_POINT_PRECISION *
      sampleFreq / (FREQ_TABLE[0] * soundFreq * 2));
  }

  class StubSample : public Sample
  {
  public:
    virtual SoundSample Get(std::size_t /*pos*/) const
    {
      return SILENT;
    }

    virtual std::size_t Size() const
    {
      return 1;
    }

    virtual std::size_t Loop() const
    {
      return 1;
    }

    virtual SoundSample Average() const
    {
      return SILENT;
    }
  };

  inline uint_t AvgToGain(SoundSample avg)
  {
    return Math::Absolute(int_t(avg) - SILENT);
  }

  class SamplesStorage
  {
  public:
    SamplesStorage()
      : MaxGain()
    {
    }

    void Add(std::size_t idx, Sample::Ptr sample)
    {
      if (sample)
      {
        Content.resize(std::max(Content.size(), idx + 1));
        const uint_t gain = AvgToGain(sample->Average());
        Content[idx] = sample;
        MaxGain = std::max(MaxGain, gain);
      }
    }

    const Sample* Get(std::size_t idx) const
    {
      static const StubSample STUB;
      const Sample::Ptr val = idx < Content.size() ? Content[idx] : Sample::Ptr();
      return val
        ? val.get()
        : &STUB;
    }

    uint_t GetMaxGain() const
    {
      return MaxGain;
    }
  private:
    uint_t MaxGain;
    std::vector<Sample::Ptr> Content;
  };

  struct LinearInterpolation
  {
    static uint_t MapInterpolation(uint_t fract)
    {
      return fract;
    }
  };

  struct CosineInterpolation
  {
    static uint_t MapInterpolation(uint_t fract)
    {
      static const CosineTable TABLE;
      return TABLE[fract];
    }
  private:
    class CosineTable
    {
    public:
      CosineTable()
      {
        for (uint_t idx = 0; idx != FIXED_POINT_PRECISION; ++idx)
        {
          const double rad = idx * 3.14159265358 / FIXED_POINT_PRECISION;
          Table[idx] = static_cast<uint_t>(FIXED_POINT_PRECISION * (1.0 - cos(rad)) / 2.0);
        }
      }

      uint_t operator[](uint_t idx) const
      {
        return Table[idx];
      }
    private:
      boost::array<uint_t, FIXED_POINT_PRECISION> Table;
    };
  };

  //channel state type
  struct ChannelState
  {
    ChannelState(const Sample* sample = 0, uint_t sampleIndex = 0)
      : Enabled(), Level(MAX_LEVEL), Note(), NoteSlide(), FreqSlide(), CurSample(sample), CurSampleIndex(sampleIndex), PosInSample(), SampleStep(1)
    {
    }

    bool Enabled;
    uint_t Level;
    uint_t Note;
    //current slide in halftones
    int_t NoteSlide;
    //current slide in Hz
    int_t FreqSlide;
    //using pointer for performance
    const Sample* CurSample;
    uint_t CurSampleIndex;
    //in fixed point
    uint_t PosInSample;
    //in fixed point
    uint_t SampleStep;

    void SkipStep()
    {
      assert(CurSample);
      PosInSample += SampleStep;
      const uint_t pos = PosInSample / FIXED_POINT_PRECISION;
      if (pos >= CurSample->Size())
      {
        if (CurSample->Loop() >= CurSample->Size())
        {
          Enabled = false;
          PosInSample = 0;
        }
        else
        {
          PosInSample = CurSample->Loop() * FIXED_POINT_PRECISION;
        }
      }
    }

    SoundSample GetValue() const
    {
      if (Enabled)
      {
        const uint_t pos = PosInSample / FIXED_POINT_PRECISION;
        assert(CurSample);
        return Amplify(CurSample->Get(pos));
      }
      else
      {
        return SILENT;
      }
    }

    template<class Policy>
    SoundSample GetInterpolatedValue() const
    {
      if (Enabled)
      {
        const uint_t pos = PosInSample / FIXED_POINT_PRECISION;
        assert(CurSample);
        const SoundSample cur = CurSample->Get(pos);
        if (const uint_t fract = PosInSample % FIXED_POINT_PRECISION)
        {
          const SoundSample next = CurSample->Get(pos + 1);
          const int_t delta = int_t(next) - int_t(cur);
          const int_t mappedFract = Policy::MapInterpolation(fract);
          const SoundSample value = static_cast<SoundSample>(int_t(cur) + delta * mappedFract / int_t(FIXED_POINT_PRECISION));
          return Amplify(value);
        }
        else
        {
          return Amplify(cur);
        }
      }
      else
      {
        return SILENT;
      }
    }

    ChanState Analyze(uint_t maxGain) const
    {
      ChanState result;
      if ( (result.Enabled = Enabled) )
      {
        result.Band = Note;
        assert(CurSample);
        const uint_t gain = AvgToGain(CurSample->Average());
        result.LevelInPercents = gain * Level / maxGain;
      }
      return result;
    }
  private:
    SoundSample Amplify(SoundSample val) const
    {
      assert(Level <= MAX_LEVEL);
      const int_t mid = SILENT;
      const int64_t delta = int_t(val) - mid;
      const int_t scaledDelta = static_cast<int_t>(delta * int_t(Level) / int_t(MAX_LEVEL));
      assert(Math::Absolute(scaledDelta) <= Math::Absolute(delta));
      return static_cast<SoundSample>(mid + scaledDelta);
    }
  };

  class ChipImpl : public Chip
  {
  public:
    ChipImpl(uint_t channels, uint_t sampleFreq, ChipParameters::Ptr params, Receiver::Ptr target)
      : Channels(channels)
      , SampleFreq(sampleFreq)
      , Params(params)
      , Target(target)
      , CurrentTime(0)
      , State(Channels)
      , TableFreq(0)
    {
      Reset();
    }

    /// Set sample for work
    virtual void SetSample(uint_t idx, Sample::Ptr sample)
    {
      Samples.Add(idx, sample);
    }

    virtual void RenderData(const DataChunk& src)
    {
      const uint_t soundFreq = Params->SoundFreq();

      // update internal state
      std::for_each(src.Channels.begin(), src.Channels.end(),
        boost::bind(&ChipImpl::UpdateState, this, _1));

      // calculate new step
      std::for_each(State.begin(), State.end(),
        boost::bind(&ChipImpl::CalcSampleStep, this, soundFreq, _1));

      // samples to apply
      const uint_t doSamples = static_cast<uint_t>(uint64_t(src.TimeStamp.Get() - CurrentTime.Get()) * soundFreq / CurrentTime.PER_SECOND);

      const std::const_mem_fun_ref_t<SoundSample, ChannelState> getter = Params->Interpolate()
        ? std::const_mem_fun_ref_t<SoundSample, ChannelState>(&ChannelState::GetInterpolatedValue<CosineInterpolation>)
        : std::mem_fun_ref(&ChannelState::GetValue);
      MultiSoundSample result(Channels);
      for (uint_t smp = 0; smp != doSamples; ++smp)
      {
        std::transform(State.begin(), State.end(), result.begin(), getter);
        std::for_each(State.begin(), State.end(), std::mem_fun_ref(&ChannelState::SkipStep));
        Target->ApplyData(result);
      }
      CurrentTime = src.TimeStamp;
    }

    virtual void GetChannelState(uint_t chan, DataChunk::ChannelData& dst) const
    {
      const ChannelState& src = State[chan];
      dst.Enabled = src.Enabled;
      dst.Note = src.Note;
      dst.NoteSlide = src.NoteSlide;
      dst.FreqSlideHz = src.FreqSlide;
      dst.SampleNum = src.CurSampleIndex;
      dst.PosInSample = src.PosInSample / FIXED_POINT_PRECISION;
      dst.LevelInPercents = src.Level;
    }

    virtual void GetState(ChannelsState& state) const
    {
      state.resize(State.size());
      std::transform(State.begin(), State.end(), state.begin(),
        std::bind2nd(std::mem_fun_ref(&ChannelState::Analyze), Samples.GetMaxGain()));
    }

    /// reset internal state to initial
    virtual void Reset()
    {
      CurrentTime = Time::Microseconds();
      std::fill(State.begin(), State.end(), ChannelState(Samples.Get(0), 0));
    }

  private:
    void UpdateState(const DataChunk::ChannelData& state)
    {
      assert(state.Channel < State.size());
      ChannelState& chan(State[state.Channel]);
      //'enabled' field changed
      if (state.Enabled)
      {
        chan.Enabled = *state.Enabled;
      }
      //note changed
      if (state.Note)
      {
        chan.Note = *state.Note;
      }
      //note slide changed
      if (state.NoteSlide)
      {
        chan.NoteSlide = *state.NoteSlide;
      }
      //frequency slide changed
      if (state.FreqSlideHz)
      {
        chan.FreqSlide = *state.FreqSlideHz;
      }
      //sample changed
      if (state.SampleNum)
      {
        chan.CurSampleIndex = *state.SampleNum;
        chan.CurSample = Samples.Get(chan.CurSampleIndex);
        chan.PosInSample = std::min<uint_t>(chan.PosInSample, FIXED_POINT_PRECISION * chan.CurSample->Size() - 1);
      }
      //position in sample changed
      if (state.PosInSample)
      {
        assert(chan.CurSample);
        chan.PosInSample = FIXED_POINT_PRECISION * std::min<uint_t>(*state.PosInSample, chan.CurSample->Size() - 1);
      }
      //level changed
      if (state.LevelInPercents)
      {
        assert(*state.LevelInPercents <= MAX_LEVEL);
        chan.Level = *state.LevelInPercents;
      }
    }

    void CalcSampleStep(uint_t freq, ChannelState& state)
    {
      // cached frequency precalc table
      if (TableFreq != freq)
      {
        TableFreq = freq;
        std::transform(FREQ_TABLE.begin(), FREQ_TABLE.end(), FreqTable.begin(),
          boost::bind(GetStepByFrequency, _1, TableFreq, SampleFreq));
        //determine maximal notes count by zero limiter in table
        MaxNotes = static_cast<uint_t>(std::distance(FreqTable.begin(), std::find(FreqTable.begin(), FreqTable.end(), 0)));
      }
      const int_t toneStep = static_cast<int_t>(FreqTable[Math::Clamp<int_t>(int_t(state.Note) + state.NoteSlide,
        0, MaxNotes - 1)]);
      state.SampleStep = state.FreqSlide
        ? Math::Clamp<int_t>(toneStep + sign(state.FreqSlide) * static_cast<int_t>(GetStepByFrequency(double(Math::Absolute(state.FreqSlide)), TableFreq, SampleFreq)),
          int_t(FreqTable.front()), int_t(FreqTable.back()))
        : toneStep;
      assert(state.SampleStep);
    }
  private:
    const uint_t Channels;
    const uint_t SampleFreq;
    const ChipParameters::Ptr Params;
    const Receiver::Ptr Target;
    SamplesStorage Samples;
    Time::Microseconds CurrentTime;
    std::vector<ChannelState> State;

    //steps calc
    uint_t TableFreq;
    uint_t MaxNotes;
    boost::array<uint_t, NOTES> FreqTable;
  };
}

namespace Devices
{
  namespace DAC
  {
    Chip::Ptr CreateChip(uint_t channels, uint_t sampleFreq, ChipParameters::Ptr params, Receiver::Ptr target)
    {
      return boost::make_shared<ChipImpl>(channels, sampleFreq, params, target);
    }
  }
}
