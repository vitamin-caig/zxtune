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
#include <devices/details/parameters_helper.h>
//common includes
#include <tools.h>
//library includes
#include <math/numeric.h>
#include <sound/chunk_builder.h>
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

  const uint_t NO_INDEX = uint_t(-1);

  class FastSample
  {
  public:
    typedef boost::shared_ptr<const FastSample> Ptr;

    //use additional sample for interpolation
    explicit FastSample(std::size_t idx, Sample::Ptr in)
      : Index(static_cast<uint_t>(idx))
      , Rms(in->Rms())
      , Data(new Sound::Sample::Type[in->Size() + 1])
      , Size(in->Size())
      , Loop(std::min(Size, in->Loop()))
    {
      for (std::size_t pos = 0; pos != Size; ++pos)
      {
        Data[pos] = in->Get(pos);
      }
      Data[Size] = Data[Size - 1];
    }

    FastSample()
      : Index(NO_INDEX)
      , Rms(0)
      , Data(new Sound::Sample::Type[2])
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

      Sound::Sample::Type GetInterpolated(const uint_t* lookup) const
      {
        assert(IsValid());
        if (const int_t fract = Pos.Fraction())
        {
          const Sound::Sample::Type* const cur = Data + Pos.Integer();
          const int_t curVal = *cur;
          const int_t nextVal = *(cur + 1);
          const int_t delta = nextVal - curVal;
          return static_cast<Sound::Sample::Type>(curVal + delta * lookup[fract] / Position::PRECISION);
        }
        else
        {
          return Data[Pos.Integer()];
        }
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
    friend class Iterator;
    const uint_t Index;
    const uint_t Rms;
    const boost::scoped_array<Sound::Sample::Type> Data;
    const std::size_t Size;
    const std::size_t Loop;
  };

  class ClockSource
  {
  public:
    ClockSource()
      : SampleFreq()
      , SoundFreq()
    {
      Reset();
    }

    void Reset()
    {
      CurrentTime = Stamp();
    }

    void SetFreq(uint_t sampleFreq, uint_t soundFreq)
    {
      SampleFreq = sampleFreq;
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

    uint_t SamplesTill(Stamp nextTime) const
    {
      //TODO
      return static_cast<uint_t>(uint64_t(nextTime.Get() - CurrentTime.Get()) * SoundFreq / CurrentTime.PER_SECOND) + 2;
    }
  private:
    //in centiHz
    typedef Math::FixedPoint<int_t, 100> Frequency;

    static inline Frequency CHz(uint_t frq)
    {
      return Frequency(frq, Frequency::PRECISION);
    }
  private:
    uint_t SampleFreq;
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
    LevelType Level;

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
      if (const LevelType* level = state.GetLevel())
      {
        Level = *level;
      }
      if (0 != (state.Mask & (DataChunk::ChannelData::NOTE | DataChunk::ChannelData::NOTESLIDE | DataChunk::ChannelData::FREQSLIDEHZ)))
      {
        Iterator.SetStep(clock.GetStep(Note + NoteSlide, FreqSlide));
      }
    }

    Sound::Sample::Type GetNearest() const
    {
      return Enabled
        ? Amplify(Iterator.GetNearest())
        : Sound::Sample::MID;
    }

    Sound::Sample::Type GetInterpolated(const uint_t* lookup) const
    {
      return Enabled
        ? Amplify(Iterator.GetInterpolated(lookup))
        : Sound::Sample::MID;
    }

    void Next()
    {
      if (Enabled)
      {
        Iterator.Next();
        Enabled = Iterator.IsValid();
      }
    }

    ChanState Analyze(uint_t maxRms) const
    {
      ChanState result;
      if ( (result.Enabled = Enabled) )
      {
        result.Band = Note + NoteSlide;
        const uint_t rms = Source->GetRms();
        result.LevelInPercents = (Level * rms * 100 / maxRms).Round();
      }
      return result;
    }
  private:
    Sound::Sample::Type Amplify(Sound::Sample::Type val) const
    {
      return (Level * val).Round();
    }
  };

  class Renderer
  {
  public:
    virtual ~Renderer() {}

    virtual void RenderData(const Stamp tillTime, Sound::ChunkBuilder& target) = 0;
  };

  template<unsigned Channels>
  class LQRenderer : public Renderer
  {
  public:
    LQRenderer(ClockSource& clock, const Sound::FixedChannelsMixer<Channels>& mixer, ChannelState* state)
      : Clock(clock)
      , Mixer(mixer)
      , State(state)
    {
    }

    virtual void RenderData(const Stamp tillTime, Sound::ChunkBuilder& target)
    {
      typename Sound::MultichannelSample<Channels>::Type result;
      for (uint_t counter = Clock.Advance(tillTime); counter != 0; --counter)
      {
        for (uint_t chan = 0; chan != Channels; ++chan)
        {
          ChannelState& state = State[chan];
          result[chan] = state.GetNearest();
          state.Next();
        }
        target.Add(Mixer.ApplyData(result));
      }
    }
  private:
    ClockSource& Clock;
    const Sound::FixedChannelsMixer<Channels>& Mixer;
    ChannelState* const State;
  };

  template<unsigned Channels>
  class MQRenderer : public Renderer
  {
  public:
    MQRenderer(ClockSource& clock, const Sound::FixedChannelsMixer<Channels>& mixer, ChannelState* state)
      : Clock(clock)
      , Mixer(mixer)
      , State(state)
    {
    }

    virtual void RenderData(const Stamp tillTime, Sound::ChunkBuilder& target)
    {
      static const CosineTable COSTABLE;
      typename Sound::MultichannelSample<Channels>::Type result;
      for (uint_t counter = Clock.Advance(tillTime); counter != 0; --counter)
      {
        for (uint_t chan = 0; chan != Channels; ++chan)
        {
          ChannelState& state = State[chan];
          result[chan] = state.GetInterpolated(COSTABLE.Get());
          state.Next();
        }
        target.Add(Mixer.ApplyData(result));
      }
    }
  private:
    class CosineTable
    {
    public:
      CosineTable()
      {
        for (uint_t idx = 0; idx != FastSample::Position::PRECISION; ++idx)
        {
          const double rad = 3.14159265358 * idx / FastSample::Position::PRECISION;
          Table[idx] = static_cast<uint_t>(FastSample::Position::PRECISION * (1.0 - cos(rad)) / 2.0);
        }
      }

      const uint_t* Get() const
      {
        return &Table[0];
      }
    private:
      boost::array<uint_t, FastSample::Position::PRECISION> Table;
    };
  private:
    ClockSource& Clock;
    const Sound::FixedChannelsMixer<Channels>& Mixer;
    ChannelState* const State;
  };

  class DataCache
  {
  public:
    void Add(const DataChunk& src)
    {
      Buffer.push_back(src);
    }

    const DataChunk* GetBegin() const
    {
      return &Buffer.front();
    }
    
    const DataChunk* GetEnd() const
    {
      return &Buffer.back() + 1;
    }

    void Reset()
    {
      Buffer.clear();
    }

    Stamp GetTillTime() const
    {
      return Buffer.empty() ? Stamp() : Buffer.back().TimeStamp;
    }
  private:
    std::vector<DataChunk> Buffer;
  };

  template<unsigned Channels>
  class RenderersSet
  {
  public:
    RenderersSet(ClockSource& clock, const Sound::FixedChannelsMixer<Channels>& mixer, ChannelState* state)
      : Clock(clock)
      , LQ(clock, mixer, state)
      , MQ(clock, mixer, state)
      , Current()
    {
    }

    void Reset()
    {
      Clock.Reset();
      Current = 0;
    }

    void SetFrequency(uint_t sampleFreq, uint_t soundFreq)
    {
      Clock.SetFreq(sampleFreq, soundFreq);
    }

    void SetInterpolation(bool type)
    {
      if (type)
      {
        Current = &MQ;
      }
      else
      {
        Current = &LQ;
      }
    }

    Renderer& Get() const
    {
      return *Current;
    }
  private:
    ClockSource& Clock;
    LQRenderer<Channels> LQ;
    MQRenderer<Channels> MQ;
    Renderer* Current;
  };

  template<unsigned Channels>
  class FixedChannelsChip : public Chip
  {
  public:
    FixedChannelsChip(ChipParameters::Ptr params, typename Sound::FixedChannelsMixer<Channels>::Ptr mixer, Sound::Receiver::Ptr target)
      : Params(params)
      , Mixer(mixer)
      , Target(target)
      , Clock()
      , Renderers(Clock, *Mixer, &State[0])
    {
      FixedChannelsChip::Reset();
    }

    /// Set sample for work
    virtual void SetSample(uint_t idx, Sample::Ptr sample)
    {
      Samples.Add(idx, sample);
    }

    virtual void RenderData(const DataChunk& src)
    {
      BufferedData.Add(src);
    }

    virtual void Flush()
    {
      const Stamp till = BufferedData.GetTillTime();
      if (!(till == Stamp(0)))
      {
        SynchronizeParameters();
        Sound::ChunkBuilder builder;
        builder.Reserve(Clock.SamplesTill(till));
        RenderChunks(builder);
        Target->ApplyData(builder.GetResult());
      }
      Target->Flush();
    }

    virtual void GetChannelState(uint_t chan, DataChunk::ChannelData& dst) const
    {
      const ChannelState& src = State[chan];
      dst = DataChunk::ChannelData();
      dst.Channel = chan;
      const uint_t samIdx = src.Source->GetIndex();
      if (samIdx != NO_INDEX)
      {
        dst.Mask = DataChunk::ChannelData::ALL_PARAMETERS;
        dst.Enabled = src.Enabled;
        dst.Note = src.Note;
        dst.NoteSlide = src.NoteSlide;
        dst.FreqSlideHz = src.FreqSlide;
        dst.SampleNum = samIdx;
        dst.PosInSample = src.Iterator.GetPosition();
        dst.Level = src.Level;
      }
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
      Params.Reset();
      Renderers.Reset();
      std::fill(State.begin(), State.end(), ChannelState(Samples.Get(0)));
      BufferedData.Reset();
    }

  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        Renderers.SetFrequency(Params->BaseSampleFreq(), Params->SoundFreq());
        Renderers.SetInterpolation(Params->Interpolate());
      }
    }

    void RenderChunks(Sound::ChunkBuilder& builder)
    {
      Renderer& render = Renderers.Get();
      for (const DataChunk* it = BufferedData.GetBegin(), *lim = BufferedData.GetEnd(); it != lim; ++it)
      {
        const DataChunk& chunk = *it;
        std::for_each(chunk.Channels.begin(), chunk.Channels.end(),
          boost::bind(&FixedChannelsChip::UpdateState, this, _1));
        render.RenderData(chunk.TimeStamp, builder);
      }
      BufferedData.Reset();
    }

    void UpdateState(const DataChunk::ChannelData& state)
    {
      assert(state.Channel < State.size());
      State[state.Channel].Update(Samples, Clock, state);
    }
  private:
    Devices::Details::ParametersHelper<ChipParameters> Params;
    const typename Sound::FixedChannelsMixer<Channels>::Ptr Mixer;
    const Sound::Receiver::Ptr Target;
    SamplesStorage Samples;
    ClockSource Clock;
    boost::array<ChannelState, Channels> State;
    RenderersSet<Channels> Renderers;
    DataCache BufferedData;
  };
}

namespace Devices
{
  namespace DAC
  {
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::ThreeChannelsMixer::Ptr mixer, Sound::Receiver::Ptr target)
    {
      return boost::make_shared<FixedChannelsChip<3> >(params, mixer, target);
    }

    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::FourChannelsMixer::Ptr mixer, Sound::Receiver::Ptr target)
    {
      return boost::make_shared<FixedChannelsChip<4> >(params, mixer, target);
    }
  }
}
