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
#include <sound/impl/internal_types.h>
//common includes
#include <tools.h>
//std includes
#include <limits>
#include <numeric>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>

namespace
{
  using namespace Devices::DAC;

  const uint_t FIXED_POINT_PRECISION = 256;

  const uint_t MAX_LEVEL = 100;

  const int_t SILENT = 128;

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

  inline uint_t GainAdder(uint_t sum, uint8_t sample)
  {
    return sum + absolute(int_t(sample) - SILENT);
  }

  inline Sample scale(uint8_t inSample)
  {
    //simply shift bits
    return Sample(inSample) << (8 * (sizeof(Sample) - sizeof(inSample)) - 1);
  }

  //digital sample type
  struct DigitalSample
  {
    DigitalSample() : Size(1), Loop(1), Data(1), Gain(0)//safe sample
    {
    }

    DigitalSample(const Dump& data, uint_t loop)
      : Size(static_cast<uint_t>(data.size())), Loop(loop), Data(data)
      , Gain(MAX_LEVEL * (std::accumulate(&Data[0], &Data[0] + Size, uint_t(0), GainAdder) / Size) / std::numeric_limits<Dump::value_type>::max())
    {
    }

    uint_t Size;
    uint_t Loop;
    Dump Data;
    uint_t Gain;
  };

  //channel state type
  struct ChannelState
  {
    explicit ChannelState(const DigitalSample* sample = 0)
      : Enabled(), Note(), NoteSlide(), FreqSlide(), CurSample(sample), PosInSample(), SampleStep(1)
    {
    }

    bool Enabled;
    uint_t Note;
    //current slide in halftones
    int_t NoteSlide;
    //current slide in Hz
    int_t FreqSlide;
    //using pointer for performance
    const DigitalSample* CurSample;
    //in fixed point
    uint_t PosInSample;
    //in fixed point
    uint_t SampleStep;

    void SkipStep()
    {
      assert(CurSample);
      PosInSample += SampleStep;
      const uint_t pos = PosInSample / FIXED_POINT_PRECISION;
      if (pos >= CurSample->Size)
      {
        if (CurSample->Loop >= CurSample->Size)
        {
          Enabled = false;
        }
        else
        {
          PosInSample = CurSample->Loop * FIXED_POINT_PRECISION;
        }
      }
    }

    Sample GetValue() const
    {
      if (Enabled)
      {
        const uint_t pos = PosInSample / FIXED_POINT_PRECISION;
        assert(CurSample && pos < CurSample->Size);
        return scale(CurSample->Data[pos]);
      }
      else
      {
        return scale(SILENT);
      }
    }

    Sample GetInterpolatedValue() const
    {
      if (Enabled)
      {
        const uint_t pos = PosInSample / FIXED_POINT_PRECISION;
        assert(CurSample && pos < CurSample->Size);
        const int_t cur = scale(CurSample->Data[pos]);
        const int_t next = pos + 1 >= CurSample->Size ? cur : scale(CurSample->Data[pos + 1]);
        const int_t delta = next - cur;
        return static_cast<Sample>(cur + delta * (PosInSample % FIXED_POINT_PRECISION) / FIXED_POINT_PRECISION);
      }
      else
      {
        return scale(SILENT);
      }
    }

    ChanState Analyze(uint_t maxGain) const
    {
      ChanState result;
      if ( (result.Enabled = Enabled) )
      {
        result.Band = Note;
        assert(CurSample);
        result.LevelInPercents = CurSample->Gain * MAX_LEVEL / maxGain;
      }
      return result;
    }
  };

  template<uint_t Channels>
  class ChipImpl : public Chip
  {
  public:
    ChipImpl(uint_t samples, uint_t sampleFreq, ChipParameters::Ptr params, Receiver::Ptr target)
      : Params(params)
      , Target(target)
      , Samples(samples), MaxGain(0), CurrentTick(0)
      , SampleFreq(sampleFreq)
      , TableFreq(0)
    {
      Reset();
    }

    /// Set sample for work
    virtual void SetSample(uint_t idx, const Dump& data, uint_t loop)
    {
      assert(idx < Samples.size());
      assert(!Samples[idx].Gain);
      const DigitalSample& res = Samples[idx] = DigitalSample(data, loop);
      // used for analyze level normalization prior to maximal
      MaxGain = std::max(MaxGain, res.Gain);
    }

    virtual void RenderData(const ZXTune::Sound::RenderParameters& /*params*/,
                            const DataChunk& src)
    {
      const uint_t soundFreq = Params->SoundFreq();
      const uint64_t clockFreq = Params->ClockFreq();

      // update internal state
      std::for_each(src.Channels.begin(), src.Channels.end(),
        boost::bind(&ChipImpl::UpdateState, this, _1));

      // calculate new step
      std::for_each(State.begin(), State.end(),
        boost::bind(&ChipImpl::CalcSampleStep, this, soundFreq, _1));

      // samples to apply
      const uint_t doSamples = static_cast<uint_t>(uint64_t(src.Tick - CurrentTick) * soundFreq / clockFreq);

      const std::const_mem_fun_ref_t<Sample, ChannelState> getter = Params->Interpolate() ?
        std::mem_fun_ref(&ChannelState::GetInterpolatedValue) : std::mem_fun_ref(&ChannelState::GetValue);
      MultiSample result(Channels);
      for (uint_t smp = 0; smp != doSamples; ++smp)
      {
        std::transform(State.begin(), State.end(), result.begin(), getter);
        std::for_each(State.begin(), State.end(), std::mem_fun_ref(&ChannelState::SkipStep));
        Target->ApplyData(result);
      }
      CurrentTick = src.Tick;
    }

    virtual void GetState(ChannelsState& state) const
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
      assert(state.Channel < State.size());
      ChannelState& chan(State[state.Channel]);
      //'enabled' field changed
      if (0 != (state.Mask & DataChunk::ChannelData::MASK_ENABLED))
      {
        chan.Enabled = state.Enabled;
      }
      //note changed
      if (0 != (state.Mask & DataChunk::ChannelData::MASK_NOTE))
      {
        chan.Note = state.Note;
      }
      //note slide changed
      if (0 != (state.Mask & DataChunk::ChannelData::MASK_NOTESLIDE))
      {
        chan.NoteSlide = state.NoteSlide;
      }
      //frequency slide changed
      if (0 != (state.Mask & DataChunk::ChannelData::MASK_FREQSLIDE))
      {
        chan.FreqSlide = state.FreqSlideHz;
      }
      //sample changed
      if (0 != (state.Mask & DataChunk::ChannelData::MASK_SAMPLE))
      {
        assert(state.SampleNum < Samples.size());
        chan.CurSample = &Samples[state.SampleNum];
      }
      //position in sample changed
      if (0 != (state.Mask & DataChunk::ChannelData::MASK_POSITION))
      {
        assert(chan.CurSample);
        chan.PosInSample = FIXED_POINT_PRECISION * std::min(state.PosInSample, chan.CurSample->Size - 1);
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
      const int_t toneStep = static_cast<int_t>(FreqTable[clamp<int_t>(int_t(state.Note) + state.NoteSlide,
        0, MaxNotes - 1)]);
      state.SampleStep = state.FreqSlide
        ? clamp<int_t>(toneStep + sign(state.FreqSlide) * static_cast<int_t>(GetStepByFrequency(double(absolute(state.FreqSlide)), TableFreq, SampleFreq)),
          int_t(FreqTable.front()), int_t(FreqTable.back()))
        : toneStep;
      assert(state.SampleStep);
    }
  private:
    const ChipParameters::Ptr Params;
    const Receiver::Ptr Target;
    std::vector<DigitalSample> Samples;
    uint_t MaxGain;
    uint64_t CurrentTick;
    boost::array<ChannelState, Channels> State;

    const uint_t SampleFreq;
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
    Chip::Ptr CreateChip(uint_t channels, uint_t samples, uint_t sampleFreq, ChipParameters::Ptr params, Receiver::Ptr target)
    {
      switch (channels)
      {
      case 4:
        return Chip::Ptr(new ChipImpl<4>(samples, sampleFreq, params, target));
      default:
        assert(!"Invalid channels count");
        return Chip::Ptr();
      }
    }
  }
}
