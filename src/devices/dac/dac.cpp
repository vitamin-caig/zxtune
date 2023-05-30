/**
 *
 * @file
 *
 * @brief  DAC support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include <devices/dac.h>
#include <devices/details/freq_table.h>
// common includes
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <math/numeric.h>
#include <parameters/tracking_helper.h>
// std includes
#include <array>
#include <cmath>

namespace Devices::DAC
{
  const uint_t NO_INDEX = uint_t(-1);

  class FastSample
  {
  public:
    using Ptr = std::shared_ptr<const FastSample>;

    // use additional sample for interpolation
    explicit FastSample(std::size_t idx, const Sample& in)
      : Index(static_cast<uint_t>(idx))
      , Data(new Sound::Sample::Type[in.Size() + 1])
      , Size(in.Size())
      , Loop(std::min(Size, in.Loop()))
    {
      for (std::size_t pos = 0; pos != Size; ++pos)
      {
        Data[pos] = in.Get(pos);
      }
      Data[Size] = Data[Size - 1];
    }

    FastSample()
      : Index(NO_INDEX)
      , Data(new Sound::Sample::Type[2])

    {
      Data[0] = Data[1] = 0;
    }

    uint_t GetIndex() const
    {
      return Index;
    }

    using Position = Math::FixedPoint<uint_t, 256>;

    class Iterator
    {
    public:
      Iterator() = default;

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
        const Position newPos = Position(pos);
        if (newPos < Pos)
        {
          Pos = newPos;
        }
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
      const Sound::Sample::Type* Data = nullptr;
      Position Step;
      Position Limit;
      Position Loop;
      Position Pos;
    };

  private:
    friend class Iterator;
    const uint_t Index;
    const std::unique_ptr<Sound::Sample::Type[]> Data;
    const std::size_t Size = 1;
    const std::size_t Loop = 1;
  };

  class ClockSource
  {
  public:
    ClockSource()
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
      const auto halftonesLim = Math::Clamp<int_t>(halftones, 0, Details::FreqTable::SIZE - 1);
      const Details::Frequency baseFreq = Details::FreqTable::GetHalftoneFrequency(0);
      const Details::Frequency freq =
          Details::FreqTable::GetHalftoneFrequency(halftonesLim) + Details::Frequency(slideHz);
      // step 1 is for first note
      return FastSample::Position(int64_t((freq * SampleFreq).Integer()), (baseFreq * SoundFreq).Integer());
    }

    Stamp GetCurrentTime() const
    {
      return CurrentTime;
    }

    uint_t Advance(Stamp nextTime)
    {
      const Stamp now = CurrentTime;
      CurrentTime = nextTime;
      return static_cast<uint_t>(uint64_t(nextTime.Get() - now.Get()) * SoundFreq / CurrentTime.PER_SECOND);
    }

  private:
    uint_t SampleFreq = 0;
    uint_t SoundFreq = 0;
    Stamp CurrentTime;
  };

  class SamplesStorage
  {
  public:
    void Add(std::size_t idx, const Sample::Ptr& sample)
    {
      if (sample)
      {
        Content.resize(std::max(Content.size(), idx + 1));
        Content[idx] = MakePtr<FastSample>(idx, *sample);
      }
    }

    FastSample::Ptr Get(std::size_t idx) const
    {
      static FastSample STUB;
      if (const FastSample::Ptr val = idx < Content.size() ? Content[idx] : FastSample::Ptr())
      {
        return val;
      }
      return MakeSingletonPointer(STUB);
    }

  private:
    std::vector<FastSample::Ptr> Content;
  };

  using SignedLevelType = Math::FixedPoint<int, ChannelData::LevelType::PRECISION>;

  // channel state type
  struct ChannelState
  {
    ChannelState()
      : Level(1)
    {}

    explicit ChannelState(FastSample::Ptr sample)
      : Source(std::move(sample))
      , Level(1)
    {}

    bool Enabled = false;
    uint_t Note = 0;
    // current slide in halftones
    int_t NoteSlide = 0;
    // current slide in Hz
    int_t FreqSlide = 0;
    // sample
    FastSample::Ptr Source;
    FastSample::Iterator Iterator;
    SignedLevelType Level;

    void Update(const SamplesStorage& samples, const ClockSource& clock, const ChannelData& state)
    {
      //'enabled' field changed
      if (const bool* enabled = state.GetEnabled())
      {
        Enabled = *enabled;
      }
      // note changed
      if (const uint_t* note = state.GetNote())
      {
        Note = *note;
      }
      // note slide changed
      if (const int_t* noteSlide = state.GetNoteSlide())
      {
        NoteSlide = *noteSlide;
      }
      // frequency slide changed
      if (const int_t* freqSlideHz = state.GetFreqSlideHz())
      {
        FreqSlide = *freqSlideHz;
      }
      // sample changed
      if (const uint_t* sampleNum = state.GetSampleNum())
      {
        Source = samples.Get(*sampleNum);
        Iterator.SetSample(*Source);
      }
      // position in sample changed
      if (const uint_t* posInSample = state.GetPosInSample())
      {
        assert(Source);
        Iterator.SetPosition(*posInSample);
      }
      // level changed
      if (const auto* level = state.GetLevel())
      {
        Level = *level;
      }
      if (0 != (state.Mask & (ChannelData::NOTE | ChannelData::NOTESLIDE | ChannelData::FREQSLIDEHZ)))
      {
        Iterator.SetStep(clock.GetStep(Note + NoteSlide, FreqSlide));
      }
    }

    Sound::Sample::Type GetNearest() const
    {
      return Enabled ? Amplify(Iterator.GetNearest()) : Sound::Sample::MID;
    }

    Sound::Sample::Type GetInterpolated(const uint_t* lookup) const
    {
      return Enabled ? Amplify(Iterator.GetInterpolated(lookup)) : Sound::Sample::MID;
    }

    void Next()
    {
      if (Enabled)
      {
        Iterator.Next();
        Enabled = Iterator.IsValid();
      }
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
    virtual ~Renderer() = default;

    virtual Sound::Chunk RenderData(uint_t samples) = 0;
  };

  template<unsigned Channels>
  class LQRenderer : public Renderer
  {
  public:
    LQRenderer(const Sound::FixedChannelsMixer<Channels>& mixer, ChannelState* state)
      : Mixer(mixer)
      , State(state)
    {}

    Sound::Chunk RenderData(uint_t samples) override
    {
      Sound::Chunk chunk;
      chunk.reserve(samples);
      typename Sound::MultichannelSample<Channels>::Type result;
      for (uint_t counter = samples; counter != 0; --counter)
      {
        for (uint_t chan = 0; chan != Channels; ++chan)
        {
          ChannelState& state = State[chan];
          result[chan] = state.GetNearest();
          state.Next();
        }
        chunk.push_back(Mixer.ApplyData(result));
      }
      return chunk;
    }

  private:
    const Sound::FixedChannelsMixer<Channels>& Mixer;
    ChannelState* const State;
  };

  template<unsigned Channels>
  class MQRenderer : public Renderer
  {
  public:
    MQRenderer(const Sound::FixedChannelsMixer<Channels>& mixer, ChannelState* state)
      : Mixer(mixer)
      , State(state)
    {}

    Sound::Chunk RenderData(uint_t samples) override
    {
      static const CosineTable COSTABLE;
      Sound::Chunk chunk;
      chunk.reserve(samples);
      typename Sound::MultichannelSample<Channels>::Type result;
      for (uint_t counter = samples; counter != 0; --counter)
      {
        for (uint_t chan = 0; chan != Channels; ++chan)
        {
          ChannelState& state = State[chan];
          result[chan] = state.GetInterpolated(COSTABLE.Get());
          state.Next();
        }
        chunk.push_back(Mixer.ApplyData(result));
      }
      return chunk;
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
        return Table.data();
      }

    private:
      std::array<uint_t, FastSample::Position::PRECISION> Table;
    };

  private:
    const Sound::FixedChannelsMixer<Channels>& Mixer;
    ChannelState* const State;
  };

  template<unsigned Channels>
  class RenderersSet
  {
  public:
    RenderersSet(const Sound::FixedChannelsMixer<Channels>& mixer, ChannelState* state)
      : LQ(mixer, state)
      , MQ(mixer, state)
      , Current()
      , State(state)
    {}

    void Reset()
    {
      Current = nullptr;
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

    Sound::Chunk RenderData(uint_t samples)
    {
      return Current->RenderData(samples);
    }

    void DropData(uint_t samples)
    {
      for (auto state = State, lim = State + Channels; state != lim; ++state)
      {
        for (uint_t count = 0; count != samples; ++count)
        {
          state->Next();
        }
      }
    }

  private:
    LQRenderer<Channels> LQ;
    MQRenderer<Channels> MQ;
    Renderer* Current;
    ChannelState* const State;
  };

  template<unsigned Channels>
  class FixedChannelsChip : public Chip
  {
  public:
    FixedChannelsChip(ChipParameters::Ptr params, typename Sound::FixedChannelsMixer<Channels>::Ptr mixer)
      : Params(std::move(params))
      , Mixer(std::move(mixer))
      , Renderers(*Mixer, State.data())
    {
      FixedChannelsChip::Reset();
    }

    /// Set sample for work
    void SetSample(uint_t idx, Sample::Ptr sample) override
    {
      Samples.Add(idx, sample);
    }

    void RenderData(const DataChunk& src) override
    {
      // to simplify
      Require(!(Clock.GetCurrentTime() < src.TimeStamp));
      UpdateChannelState(src);
    }

    void UpdateState(const DataChunk& src) override
    {
      if (Clock.GetCurrentTime() < src.TimeStamp)
      {
        DropChunksTill(src.TimeStamp);
      }
      UpdateChannelState(src);
    }

    Sound::Chunk RenderTill(Stamp stamp) override
    {
      const uint_t samples = Clock.Advance(stamp);
      Require(samples);
      auto result = Renderers.RenderData(samples);
      SynchronizeParameters();
      return result;
    }

    /// reset internal state to initial
    void Reset() override
    {
      Params.Reset();
      Clock.Reset();
      Renderers.Reset();
      std::fill(State.begin(), State.end(), ChannelState(Samples.Get(0)));
      SynchronizeParameters();
    }

  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        Clock.SetFreq(Params->BaseSampleFreq(), Params->SoundFreq());
        Renderers.SetInterpolation(Params->Interpolate());
      }
    }

    void DropChunksTill(Stamp stamp)
    {
      const uint_t samples = Clock.Advance(stamp);
      Renderers.DropData(samples);
    }

    void UpdateChannelState(const DataChunk& chunk)
    {
      for (const auto& state : chunk.Data)
      {
        assert(state.Channel < State.size());
        State[state.Channel].Update(Samples, Clock, state);
      }
    }

  private:
    Parameters::TrackingHelper<ChipParameters> Params;
    const typename Sound::FixedChannelsMixer<Channels>::Ptr Mixer;
    SamplesStorage Samples;
    ClockSource Clock;
    std::array<ChannelState, Channels> State;
    RenderersSet<Channels> Renderers;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::ThreeChannelsMixer::Ptr mixer)
  {
    return MakePtr<FixedChannelsChip<3> >(std::move(params), std::move(mixer));
  }

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::FourChannelsMixer::Ptr mixer)
  {
    return MakePtr<FixedChannelsChip<4> >(std::move(params), std::move(mixer));
  }
}  // namespace Devices::DAC
