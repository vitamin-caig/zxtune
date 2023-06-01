/**
 *
 * @file
 *
 * @brief  ETracker chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/saa/etracker.h"
#include "module/players/saa/saa_base.h"
// common includes
#include <make_ptr.h>
// library includes
#include <formats/chiptune/saa/etracker.h>
#include <module/players/platforms.h>
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>

namespace Module::ETracker
{
  // supported commands and parameters
  enum CmdType
  {
    // no parameters
    EMPTY,
    // yes/no
    SWAPCHANNELS,
    // tone
    ENVELOPE,
    // type
    NOISE
  };

  using Formats::Chiptune::ETracker::Sample;
  using Formats::Chiptune::ETracker::Ornament;

  using OrderListWithTransposition = SimpleOrderListWithTransposition<Formats::Chiptune::ETracker::PositionEntry>;

  class ModuleData : public TrackModel
  {
  public:
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;

    ModuleData() = default;

    uint_t GetChannelsCount() const override
    {
      return SAA::TRACK_CHANNELS;
    }

    uint_t GetInitialTempo() const override
    {
      return InitialTempo;
    }

    const OrderList& GetOrder() const override
    {
      return *Order;
    }

    const PatternsSet& GetPatterns() const override
    {
      return *Patterns;
    }

    uint_t InitialTempo = 0;
    OrderListWithTransposition::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<Sample> Samples;
    SparsedObjectsStorage<Ornament> Ornaments;
  };

  class DataBuilder : public Formats::Chiptune::ETracker::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Meta(props)
      , Patterns(PatternsBuilder::Create<SAA::TRACK_CHANNELS>())
      , Data(MakeRWPtr<ModuleData>())
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, Formats::Chiptune::ETracker::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::ETracker::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::ETracker::Positions positions) override
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
      Patterns.GetChannel().SetNote(note);
    }

    void SetSample(uint_t sample) override
    {
      Patterns.GetChannel().SetSample(sample);
    }

    void SetOrnament(uint_t ornament) override
    {
      Patterns.GetChannel().SetOrnament(ornament);
    }

    void SetAttenuation(uint_t vol) override
    {
      Patterns.GetChannel().SetVolume(15 - vol);
    }

    void SetSwapSampleChannels(bool swapChannels) override
    {
      Patterns.GetChannel().AddCommand(SWAPCHANNELS, swapChannels);
    }

    void SetEnvelope(uint_t value) override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE, value);
    }

    void SetNoise(uint_t type) override
    {
      Patterns.GetChannel().AddCommand(NOISE, type);
    }

    ModuleData::Ptr CaptureResult()
    {
      Data->Patterns = Patterns.CaptureResult();
      return std::move(Data);
    }

  private:
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
      Obj = &obj;
      Reset();
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
      return Obj->FindLine(Position);
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

  // enabled state via sample
  struct ChannelState
  {
    ChannelState() = default;

    uint_t Note = 0;
    ObjectLinesIterator<Sample> SampleIterator;
    ObjectLinesIterator<Ornament> OrnamentIterator;
    uint_t Attenuation = 0;
    bool SwapSampleChannels = false;
  };

  class DataRenderer : public SAA::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(std::move(data))
      , Noise()
      , Transposition()
    {
      Reset();
    }

    void Reset() override
    {
      static const Sample STUB_SAMPLE;
      static const Ornament STUB_ORNAMENT;
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        ChannelState& dst = PlayerState[chan];
        dst = ChannelState();
        dst.SampleIterator.Set(STUB_SAMPLE);
        dst.SampleIterator.Disable();
        dst.OrnamentIterator.Set(STUB_ORNAMENT);
        dst.OrnamentIterator.Reset();
      }
      std::fill(Noise.begin(), Noise.end(), 0);
      Transposition = 0;
    }

    void SynthesizeData(const TrackModelState& state, SAA::TrackBuilder& track) override
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
    void GetNewLineState(const TrackModelState& state, SAA::TrackBuilder& track)
    {
      if (const auto* const line = state.LineObject())
      {
        for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
        {
          if (const auto* const src = line->GetChannel(chan))
          {
            SAA::ChannelBuilder channel = track.GetChannel(chan);
            GetNewChannelState(chan, *src, PlayerState[chan], channel);
          }
        }
      }
    }

    void GetNewChannelState(uint_t idx, const Cell& src, ChannelState& dst, SAA::ChannelBuilder& channel)
    {
      static const uint_t ENVELOPE_TABLE[] = {0x00, 0x96, 0x9e, 0x9a, 0x086, 0x8e, 0x8a,
                                              0x97, 0x9f, 0x9b, 0x87, 0x8f,  0x8b};
      if (const bool* enabled = src.GetEnabled())
      {
        if (!*enabled)
        {
          dst.SampleIterator.Disable();
          dst.OrnamentIterator.Reset();
        }
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
        dst.SampleIterator.Reset();
        dst.OrnamentIterator.Reset();
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleIterator.Set(Data->Samples.Get(*sample));
        dst.OrnamentIterator.Reset();
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentIterator.Set(Data->Ornaments.Get(*ornament));
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Attenuation = 15 - *volume;
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          channel.SetEnvelope(ENVELOPE_TABLE[it->Param1]);
          break;
        case SWAPCHANNELS:
          dst.SwapSampleChannels = it->Param1 != 0;
          break;
        case NOISE:
          Noise[idx >= 3] = it->Param1;
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }

    void SynthesizeChannelsData(SAA::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        SAA::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(chan, channel);
        if (const uint_t noise = Noise[chan >= 3])
        {
          channel.AddNoise(noise);
        }
      }
    }

    class Note
    {
    public:
      Note() = default;

      void Set(uint_t halfTones)
      {
        const uint_t TONES_PER_OCTAVE = 12;
        static const uint_t TONE_TABLE[TONES_PER_OCTAVE] = {0x5,  0x21, 0x3c, 0x55, 0x6d, 0x84,
                                                            0x99, 0xad, 0xc0, 0xd2, 0xe3, 0xf3};
        Value = ((halfTones / TONES_PER_OCTAVE) << 8) + TONE_TABLE[halfTones % TONES_PER_OCTAVE];
      }

      void Add(uint_t delta)
      {
        Value += delta;
      }

      uint_t Octave() const
      {
        return (Value >> 8) & 7;
      }

      uint_t Number() const
      {
        return Value & 0xff;
      }

    private:
      uint_t Value = 0x7ff;
    };

    void SynthesizeChannel(uint_t idx, SAA::ChannelBuilder& channel)
    {
      ChannelState& dst = PlayerState[idx];

      Sample::Line curLine;
      if (const Sample::Line* curSampleLine = dst.SampleIterator.GetLine())
      {
        curLine = *curSampleLine;
        dst.SampleIterator.Next();
      }
      uint_t halfTones = dst.Note;
      if (const uint_t* curOrnamentLine = dst.OrnamentIterator.GetLine())
      {
        halfTones += *curOrnamentLine;
        dst.OrnamentIterator.Next();
      }
      // set tone
      Note note;
      if (halfTones != 0x5f)
      {
        halfTones += Transposition;
        if (halfTones >= 0x100)
        {
          halfTones -= 0x60;
        }
        note.Set(halfTones);
      }
      note.Add(curLine.ToneDeviation);
      channel.SetTone(note.Octave(), note.Number());
      if (curLine.ToneEnabled)
      {
        channel.EnableTone();
      }

      // set levels
      if (dst.SwapSampleChannels)
      {
        std::swap(curLine.LeftLevel, curLine.RightLevel);
      }
      channel.SetVolume(int_t(curLine.LeftLevel) - dst.Attenuation, int_t(curLine.RightLevel) - dst.Attenuation);

      // set noise
      if (curLine.NoiseEnabled)
      {
        channel.EnableNoise();
      }
      if (curLine.NoiseEnabled || 0 == idx)
      {
        channel.SetNoise(curLine.NoiseFreq);
      }
    }

  private:
    const ModuleData::Ptr Data;
    std::array<ChannelState, SAA::TRACK_CHANNELS> PlayerState;
    std::array<uint_t, 2> Noise;
    uint_t Transposition;
  };

  class Chiptune : public SAA::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(std::move(data))
      , Properties(std::move(properties))
    {}

    Time::Microseconds GetFrameDuration() const override
    {
      return SAA::BASE_FRAME_DURATION;
    }

    TrackModel::Ptr GetTrackModel() const override
    {
      return Data;
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    SAA::DataIterator::Ptr CreateDataIterator() const override
    {
      auto iterator = CreateTrackStateIterator(GetFrameDuration(), Data);
      auto renderer = MakePtr<DataRenderer>(Data);
      return SAA::CreateDataIterator(std::move(iterator), std::move(renderer));
    }

  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
  };

  class Factory : public Module::Factory
  {
  public:
    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData,
                             Parameters::Container::Ptr properties) const override
    {
      PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Formats::Chiptune::ETracker::Parse(rawData, dataBuilder))
      {
        props.SetSource(*container);
        props.SetPlatform(Platforms::SAM_COUPE);
        auto chiptune = MakePtr<Chiptune>(dataBuilder.CaptureResult(), std::move(properties));
        return SAA::CreateHolder(std::move(chiptune));
      }
      return {};
    }
  };

  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::ETracker
