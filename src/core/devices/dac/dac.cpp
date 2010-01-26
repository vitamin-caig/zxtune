/*
Abstract:
  DAC implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <tools.h>
#include <core/devices/dac.h>
#include <sound/sound_types.h>
#include <sound/render_params.h>
#include <sound/impl/internal_types.h>

#include <limits>
#include <numeric>

#include <boost/array.hpp>
#include <boost/bind.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::DAC;

  const int SILENT = 128;

  const unsigned NOTES = 64;
  //table in Hz
  const boost::array<double, NOTES> FREQ_TABLE = {
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

  inline unsigned GetStepByFrequency(unsigned note, unsigned soundFreq, unsigned sampleFreq)
  {
    return static_cast<unsigned>(note * Sound::FIXED_POINT_PRECISION *
      sampleFreq / (FREQ_TABLE[0] * soundFreq * 2));
  }

  inline unsigned GainAdder(unsigned sum, uint8_t sample)
  {
    return sum + abs(int(sample) - SILENT);
  }

  inline Sound::Sample scale(uint8_t inSample)
  {
    return Sound::Sample(inSample) << (8 * (sizeof(Sound::Sample) - sizeof(inSample)) - 1);
  }

  struct Sample
  {
    Sample() : Size(1), Loop(1), Data(1), Gain(0)//safe sample
    {
    }

    Sample(const Dump& data, unsigned loop)
      : Size(static_cast<unsigned>(data.size())), Loop(loop), Data(data)
      , Gain(Module::Analyze::LevelType(std::accumulate(&Data[0], &Data[0] + Size, unsigned(0), GainAdder) / Size))
    {
    }

    unsigned Size;
    unsigned Loop;
    Dump Data;
    Module::Analyze::LevelType Gain;
  };

  struct ChannelState
  {
    explicit ChannelState(const Sample* sample = 0)
      : Enabled(), Note(), NoteSlide(), FreqSlide(), CurSample(sample), PosInSample(), SampleStep(1)
    {
    }

    bool Enabled;
    unsigned Note;
    signed NoteSlide;
    signed FreqSlide;
    const Sample* CurSample;
    unsigned PosInSample;//in fixed point
    unsigned SampleStep;

    void SkipStep()
    {
      assert(CurSample);
      PosInSample += SampleStep;
      const unsigned pos(PosInSample / Sound::FIXED_POINT_PRECISION);
      if (pos >= CurSample->Size)
      {
        if (CurSample->Loop >= CurSample->Size)
        {
          Enabled = false;
        }
        else
        {
          PosInSample = CurSample->Loop * Sound::FIXED_POINT_PRECISION;
        }
      }
    }

    Sound::Sample GetValue() const
    {
      if (Enabled)
      {
        const unsigned pos(PosInSample / Sound::FIXED_POINT_PRECISION);
        assert(CurSample && pos < CurSample->Size);
        return scale(CurSample->Data[pos]);
      }
      else
      {
        return scale(SILENT);
      }
    }
    
    Sound::Sample GetInterpolatedValue() const
    {
      if (Enabled)
      {
        const unsigned pos(PosInSample / Sound::FIXED_POINT_PRECISION);
        assert(CurSample && pos < CurSample->Size);
        const Sound::Sample cur(scale(CurSample->Data[pos]));
        const Sound::Sample next(pos >= CurSample->Size ? cur : scale(CurSample->Data[pos + 1]));
        const int delta = int(next) - cur;
        return cur + delta * (PosInSample % Sound::FIXED_POINT_PRECISION) / Sound::FIXED_POINT_PRECISION;
      }
      else
      {
        return scale(SILENT);
      }
    }
    
    Module::Analyze::Channel Analyze(Module::Analyze::LevelType maxGain) const
    {
      Module::Analyze::Channel result;
      if ((result.Enabled = Enabled))
      {
        result.Band = Note;
        assert(CurSample);
        result.Level = CurSample->Gain * std::numeric_limits<Module::Analyze::LevelType>::max() / maxGain;
      }
      return result;
    }
  };

  template<unsigned Channels>
  class ChipImpl : public Chip
  {
  public:
    ChipImpl(unsigned samples, unsigned sampleFreq)
      : Samples(samples), MaxGain(0), CurrentTick(0)
      , SampleFreq(sampleFreq)
    {
      Reset();
    }

    /// Set sample for work
    virtual void SetSample(unsigned idx, const Dump& data, unsigned loop)
    {
      if (idx >= Samples.size())
        throw 1;//TODO
      assert(!Samples[idx].Gain);
      const Sample& res(Samples[idx] = Sample(data, loop));
      MaxGain = std::max(MaxGain, res.Gain);
    }

    virtual void RenderData(const Sound::RenderParameters& params,
                            const DataChunk& src,
                            Sound::MultichannelReceiver& dst)
    {
      std::for_each(src.Channels.begin(), src.Channels.end(),
        boost::bind(&ChipImpl::UpdateState, this, _1));

      std::for_each(State.begin(), State.end(),
        boost::bind(&ChipImpl::CalcSampleStep, this, params.SoundFreq, _1));

      const uint64_t ticksPerSample(params.ClockFreq / params.SoundFreq);
      const unsigned doSamples(static_cast<unsigned>((src.Tick - CurrentTick) * params.SoundFreq / params.ClockFreq));

      const std::const_mem_fun_ref_t<Sound::Sample, ChannelState> getter = src.Interpolate ?
        std::mem_fun_ref(&ChannelState::GetValue) : std::mem_fun_ref(&ChannelState::GetInterpolatedValue);
      std::vector<Sound::Sample> result(Channels);
      for (unsigned smp = 0; smp != doSamples; ++smp)
      {
        std::transform(State.begin(), State.end(), result.begin(), getter);
        std::for_each(State.begin(), State.end(), std::mem_fun_ref(&ChannelState::SkipStep));
        dst.ApplySample(result);
      }
      CurrentTick += doSamples * ticksPerSample;
    }

    virtual void GetState(Module::Analyze::ChannelsState& state) const
    {
      state.resize(State.size());
      std::transform(State.begin(), State.end(), state.begin(),
        std::bind2nd(std::mem_fun_ref(&ChannelState::Analyze), MaxGain));
    }

    /// reset internal state to initial
    virtual void Reset()
    {
      CurrentTick = 0;
      std::fill(State.begin(), State.end(), ChannelState(&Samples.front()));
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

    void CalcSampleStep(unsigned freq, ChannelState& state)
    {
      if (TableFreq != freq)
      {
        TableFreq = freq;
        std::transform(FREQ_TABLE.begin(), FREQ_TABLE.end(), FreqTable.begin(),
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
    Module::Analyze::LevelType MaxGain;
    uint64_t CurrentTick;
    boost::array<ChannelState, Channels> State;
    
    const unsigned SampleFreq;
    //steps calc
    unsigned TableFreq;
    unsigned MaxNotes;
    boost::array<unsigned, NOTES> FreqTable;
  };
}

namespace ZXTune
{
  namespace DAC
  {
    Chip::Ptr CreateChip(unsigned channels, unsigned samples, unsigned sampleFreq)
    {
      switch (channels)
      {
      case 4:
        return Chip::Ptr(new ChipImpl<4>(samples, sampleFreq));
      default:
        assert(!"Invalid channels count");
        return Chip::Ptr();
      }
    }
  }
}
