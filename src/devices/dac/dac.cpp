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
#include <math/fixedpoint.h>
#include <math/numeric.h>
//std includes
#include <cmath>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_array.hpp>

namespace
{
  using namespace Devices::DAC;

  class FastSample
  {
  public:
    typedef boost::shared_ptr<const FastSample> Ptr;

    explicit FastSample(std::size_t idx, Sample::Ptr in)
      : Index(static_cast<uint_t>(idx))
      , Rms(in->Rms())
      , Data(new Sound::Sample::Type[in->Size()])
      , Size(in->Size())
      , Loop(std::min(Size, in->Loop()))
    {
      for (std::size_t pos = 0; pos != Size; ++pos)
      {
        Data[pos] = in->Get(pos);
      }
    }

    FastSample()
      : Index(-1)
      , Rms(0)
      , Data(new Sound::Sample::Type[1])
      , Size(1)
      , Loop(1)
    {
    }

    uint_t GetIndex() const
    {
      return Index;
    }

    uint_t GetRms() const
    {
      return Rms;
    }

    typedef Math::FixedPoint<uint_t, 256> Position;
    typedef Math::FixedPoint<int_t, 100> Level;

    class Iterator
    {
    public:
      Iterator()
        : Data()
        , Step()
        , Limit()
        , Loop()
        , Pos()
      {
      }

      bool IsValid() const
      {
        return Data && Pos < Limit;
      }

      uint_t GetPosition() const
      {
        return Pos.Integer();
      }

      Sound::Sample::Type GetNearest() const
      {
        assert(IsValid());
        return Data[Pos.Integer()];
      }

      void Reset()
      {
        Pos = 0;
      }

      void Next()
      {
        Pos = GetNextPos();
      }

      void SetPosition(uint_t pos)
      {
        Pos = pos;
      }

      void SetStep(Position step)
      {
        Step = step;
      }

      void SetSample(const FastSample& sample)
      {
        Data = sample.Data.get();
        Limit = sample.Size;
        Loop = sample.Loop;
        Pos = std::min(Pos, Limit);
      }
    private:
      Position GetNextPos() const
      {
        const Position res = Pos + Step;
        return res < Limit ? res : Loop;
      }
    private:
      const Sound::Sample::Type* Data;
      Position Step;
      Position Limit;
      Position Loop;
      Position Pos;
    };
  private:
    friend Iterator;
    const uint_t Index;
    const uint_t Rms;
    const boost::scoped_array<Sound::Sample::Type> Data;
    const std::size_t Size;
    const std::size_t Loop;
  };

  class ClockSource
  {
  public:
    explicit ClockSource(uint_t sampleFreq)
      : SampleFreq(sampleFreq)
      , SoundFreq()
    {
      Reset();
    }

    void Reset()
    {
      CurrentTime = Stamp();
    }

    void SetSoundFreq(uint_t soundFreq)
    {
      SoundFreq = soundFreq;
    }

    FastSample::Position GetStep(int_t halftones, int_t slideHz) const
    {
      //http://www.phy.mtu.edu/~suits/notefreqs.html
      static const Frequency NOTES[] =
      {
      //C           C#/Db       D           D#/Eb       E           F           F#/Gb       G           G#/Ab       A           A#/Bb       B
        //octave1
          CHz(3270),  CHz(3465),  CHz(3671),  CHz(3889),  CHz(4120),  CHz(4365),  CHz(4625),  CHz(4900),  CHz(5191),  CHz(5500),  CHz(5827),  CHz(6174),
        //octave2
          CHz(6541),  CHz(6930),  CHz(7342),  CHz(7778),  CHz(8241),  CHz(8731),  CHz(9250),  CHz(9800), CHz(10383), CHz(11000), CHz(11654), CHz(12347),
        //octave3
         CHz(13081), CHz(13859), CHz(14683), CHz(15556), CHz(16481), CHz(17461), CHz(18500), CHz(19600), CHz(20765), CHz(22000), CHz(23308), CHz(24694),
        //octave4
         CHz(26163), CHz(27718), CHz(29366), CHz(31113), CHz(32963), CHz(34923), CHz(36999), CHz(39200), CHz(41530), CHz(44000), CHz(46616), CHz(49388),
        //octave5
         CHz(52325), CHz(55437), CHz(58733), CHz(62225), CHz(65926), CHz(69846), CHz(73999), CHz(78399), CHz(83061), CHz(88000), CHz(93233), CHz(98777),
        //octave6
        CHz(104650),CHz(110873),CHz(117466),CHz(124451),CHz(131851),CHz(139691),CHz(147998),CHz(156798),CHz(166122),CHz(176000),CHz(186466),CHz(197553),
        //octave7
        CHz(209300),CHz(221746),CHz(234932),CHz(248902),CHz(263702),CHz(279383),CHz(295996),CHz(313596),CHz(332244),CHz(352000),CHz(372931),CHz(395107),
        //octave8
        CHz(418601),CHz(443492),CHz(469864),CHz(497803)
      };
      const int_t notesCount = ArraySize(NOTES);
      const Frequency freq = NOTES[Math::Clamp<int_t>(halftones, 0, notesCount - 1)] + Frequency(slideHz);
      //step 1 is for first note
      return FastSample::Position(int64_t((freq * SampleFreq).Integer()), (NOTES[0] * SoundFreq).Integer());
    }

    uint_t Advance(Stamp nextTime)
    {
      const Stamp now = CurrentTime;
      CurrentTime = nextTime;
      return static_cast<uint_t>(uint64_t(nextTime.Get() - now.Get()) * SoundFreq / CurrentTime.PER_SECOND);
    }
  private:
    //in centiHz
    typedef Math::FixedPoint<int_t, 100> Frequency;

    static inline Frequency CHz(uint_t frq)
    {
      return Frequency(frq, Frequency::PRECISION);
    }
  private:
    const uint_t SampleFreq;
    uint_t SoundFreq;
    Stamp CurrentTime;
  };

  class SamplesStorage
  {
  public:
    SamplesStorage()
      : MaxRms()
    {
    }

    void Add(std::size_t idx, Sample::Ptr sample)
    {
      if (sample)
      {
        Content.resize(std::max(Content.size(), idx + 1));
        const FastSample::Ptr fast = boost::make_shared<FastSample>(idx, sample);
        Content[idx] = fast;
        MaxRms = std::max(MaxRms, fast->GetRms());
      }
    }

    FastSample::Ptr Get(std::size_t idx) const
    {
      static FastSample STUB;
      if (const FastSample::Ptr val = idx < Content.size() ? Content[idx] : FastSample::Ptr())
      {
        return val;
      }
      return FastSample::Ptr(&STUB, NullDeleter<FastSample>());
    }

    uint_t GetMaxRms() const
    {
      return MaxRms;
    }
  private:
    uint_t MaxRms;
    std::vector<FastSample::Ptr> Content;
  };

  /*
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
  */

  //channel state type
  struct ChannelState
  {
    ChannelState()
      : Enabled()
      , Note()
      , NoteSlide()
      , FreqSlide()
      , Level(1)
    {
    }

    explicit ChannelState(FastSample::Ptr sample)
      : Enabled()
      , Note()
      , NoteSlide()
      , FreqSlide()
      , Source(sample)
      , Level(1)
    {
    }

    bool Enabled;
    uint_t Note;
    //current slide in halftones
    int_t NoteSlide;
    //current slide in Hz
    int_t FreqSlide;
    //sample
    FastSample::Ptr Source;
    FastSample::Iterator Iterator;
    FastSample::Level Level;

    void Update(const SamplesStorage& samples, const ClockSource& clock, const DataChunk::ChannelData& state)
    {
      //'enabled' field changed
      if (const bool* enabled = state.GetEnabled())
      {
        Enabled = *enabled;
      }
      //note changed
      if (const uint_t* note = state.GetNote())
      {
        Note = *note;
      }
      //note slide changed
      if (const int_t* noteSlide = state.GetNoteSlide())
      {
        NoteSlide = *noteSlide;
      }
      //frequency slide changed
      if (const int_t* freqSlideHz = state.GetFreqSlideHz())
      {
        FreqSlide = *freqSlideHz;
      }
      //sample changed
      if (const uint_t* sampleNum = state.GetSampleNum())
      {
        Source = samples.Get(*sampleNum);
        Iterator.SetSample(*Source);
      }
      //position in sample changed
      if (const uint_t* posInSample = state.GetPosInSample())
      {
        assert(Source);
        Iterator.SetPosition(*posInSample);
      }
      //level changed
      if (const uint_t* levelInPercents = state.GetLevelInPercents())
      {
        assert(*levelInPercents <= FastSample::Level::PRECISION);
        Level = FastSample::Level(*levelInPercents, FastSample::Level::PRECISION);
      }
      if (0 != (state.Mask & (DataChunk::ChannelData::NOTE | DataChunk::ChannelData::NOTESLIDE | DataChunk::ChannelData::FREQSLIDEHZ)))
      {
        Iterator.SetStep(clock.GetStep(Note + NoteSlide, FreqSlide));
      }
    }

    Sound::Sample::Type GetNearest() const
    {
      if (Enabled)
      {
        return Amplify(Iterator.GetNearest());
      }
      else
      {
        return Sound::Sample::MID;
      }
    }

    void Next()
    {
      if (Enabled)
      {
        Iterator.Next();
        Enabled = Iterator.IsValid();
      }
    }

    /*
    template<class Policy>
    SoundSample GetInterpolatedValue() const
    {
      if (Position.IsEnabled())
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
    */

    ChanState Analyze(uint_t maxRms) const
    {
      ChanState result;
      if ( (result.Enabled = Enabled) )
      {
        result.Band = Note + NoteSlide;
        const uint_t rms = Source->GetRms();
        result.LevelInPercents = (Level * rms / maxRms).Fraction();
      }
      return result;
    }
  private:
    Sound::Sample::Type Amplify(Sound::Sample::Type val) const
    {
      return (Level * val).Integer();
    }
  };

  template<unsigned Channels>
  class FixedChannelsChip : public Chip
  {
  public:
    FixedChannelsChip(uint_t sampleFreq, ChipParameters::Ptr params, typename Sound::FixedChannelsStreamMixer<Channels>::Ptr target)
      : Params(params)
      , Target(target)
      , Clock(sampleFreq)
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
      Clock.SetSoundFreq(Params->SoundFreq());

      // update internal state
      std::for_each(src.Channels.begin(), src.Channels.end(),
        boost::bind(&FixedChannelsChip::UpdateState, this, _1));

      if (const uint_t doSamples = Clock.Advance(src.TimeStamp))
      {
        typename Sound::MultichannelSample<Channels>::Type result;
        for (uint_t smp = 0; smp != doSamples; ++smp)
        {
          for (uint_t chan = 0; chan != Channels; ++chan)
          {
            ChannelState& state = State[chan];
            result[chan] = state.GetNearest();
            state.Next();
          }
          Target->ApplyData(result);
        }
      }
    }

    virtual void Flush()
    {
      Target->Flush();
    }

    virtual void GetChannelState(uint_t chan, DataChunk::ChannelData& dst) const
    {
      const ChannelState& src = State[chan];
      dst.Channel = chan;
      dst.Mask = DataChunk::ChannelData::ALL_PARAMETERS;
      dst.Enabled = src.Enabled;
      dst.Note = src.Note;
      dst.NoteSlide = src.NoteSlide;
      dst.FreqSlideHz = src.FreqSlide;
      dst.SampleNum = src.Source->GetIndex();
      dst.PosInSample = src.Iterator.GetPosition();
      dst.LevelInPercents = src.Level.Fraction();
    }

    virtual void GetState(ChannelsState& state) const
    {
      state.resize(State.size());
      std::transform(State.begin(), State.end(), state.begin(),
        std::bind2nd(std::mem_fun_ref(&ChannelState::Analyze), Samples.GetMaxRms()));
    }

    /// reset internal state to initial
    virtual void Reset()
    {
      Clock.Reset();
      std::fill(State.begin(), State.end(), ChannelState(Samples.Get(0)));
    }

  private:
    void UpdateState(const DataChunk::ChannelData& state)
    {
      assert(state.Channel < State.size());
      ChannelState& chan(State[state.Channel]);
      chan.Update(Samples, Clock, state);
    }
  private:
    const ChipParameters::Ptr Params;
    const typename Sound::FixedChannelsStreamMixer<Channels>::Ptr Target;
    SamplesStorage Samples;
    ClockSource Clock;
    boost::array<ChannelState, Channels> State;
  };
}

namespace Devices
{
  namespace DAC
  {
    Chip::Ptr CreateChip(uint_t sampleFreq, ChipParameters::Ptr params, Sound::ThreeChannelsStreamMixer::Ptr target)
    {
      return boost::make_shared<FixedChannelsChip<3> >(sampleFreq, params, target);
    }

    Chip::Ptr CreateChip(uint_t sampleFreq, ChipParameters::Ptr params, Sound::FourChannelsStreamMixer::Ptr target)
    {
      return boost::make_shared<FixedChannelsChip<4> >(sampleFreq, params, target);
    }
  }
}
