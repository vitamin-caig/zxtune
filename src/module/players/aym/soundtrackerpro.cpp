/**
* 
* @file
*
* @brief  SoundTrackerPro-based chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "soundtrackerpro.h"
#include "aym_base_track.h"
#include "aym_properties_helper.h"
//common includes
#include <make_ptr.h>
//library includes
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>
#include <module/players/simple_ornament.h>

namespace Module
{
namespace SoundTrackerPro
{
  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
    GLISS,        //1p
  };

  struct Sample : public Formats::Chiptune::SoundTrackerPro::Sample
  {
    Sample()
      : Formats::Chiptune::SoundTrackerPro::Sample()
    {
    }

    Sample(Formats::Chiptune::SoundTrackerPro::Sample rh)
      : Formats::Chiptune::SoundTrackerPro::Sample(std::move(rh))
    {
    }

    int_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    const Line& GetLine(uint_t idx) const
    {
      static const Line STUB;
      return Lines.size() > idx ? Lines[idx] : STUB;
    }
  };

  typedef SimpleOrderListWithTransposition<Formats::Chiptune::SoundTrackerPro::PositionEntry> OrderListWithTransposition;
  typedef SimpleOrnament Ornament;

  class ModuleData : public TrackModel
  {
  public:
    typedef std::shared_ptr<const ModuleData> Ptr;
    typedef std::shared_ptr<ModuleData> RWPtr;

    ModuleData()
      : InitialTempo()
    {
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

    uint_t InitialTempo;
    OrderListWithTransposition::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<Sample> Samples;
    SparsedObjectsStorage<Ornament> Ornaments;
  };

  class DataBuilder : public Formats::Chiptune::SoundTrackerPro::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Data(MakeRWPtr<ModuleData>())
      , Properties(props)
      , Meta(props)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
    {
      Data->Patterns = Patterns.GetResult();
      Properties.SetFrequencyTable(TABLE_SOUNDTRACKER_PRO);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, const Formats::Chiptune::SoundTrackerPro::Sample& sample) override
    {
      Data->Samples.Add(index, sample);
    }

    void SetOrnament(uint_t index, const Formats::Chiptune::SoundTrackerPro::Ornament& ornament) override
    {
      Data->Ornaments.Add(index, Ornament(ornament.Loop, ornament.Lines.begin(), ornament.Lines.end()));
    }

    void SetPositions(const std::vector<Formats::Chiptune::SoundTrackerPro::PositionEntry>& positions, uint_t loop) override
    {
      Data->Order = MakePtr<OrderListWithTransposition>(loop, positions.begin(), positions.end());
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
      Patterns.GetChannel().SetEnabled(true);
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

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE, type, value);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().AddCommand(NOENVELOPE);
    }

    void SetGliss(uint_t target) override
    {
      Patterns.GetChannel().AddCommand(GLISS, target);
    }
    
    void SetVolume(uint_t vol) override
    {
      Patterns.GetChannel().SetVolume(vol);
    }

    virtual ModuleData::Ptr GetResult() const
    {
      return Data;
    }
  private:
    const ModuleData::RWPtr Data;
    AYM::PropertiesHelper& Properties;
    MetaProperties Meta;
    PatternsBuilder Patterns;
  };

  struct ChannelState
  {
    ChannelState()
      : Enabled(false), Envelope(false), Volume(0)
      , Note(0), SampleNum(Formats::Chiptune::SoundTrackerPro::DEFAULT_SAMPLE), PosInSample(0)
      , OrnamentNum(Formats::Chiptune::SoundTrackerPro::DEFAULT_ORNAMENT), PosInOrnament(0)
      , TonSlide(0), Glissade(0)
    {
    }
    bool Enabled;
    bool Envelope;
    uint_t Volume;
    uint_t Note;
    uint_t SampleNum;
    uint_t PosInSample;
    uint_t OrnamentNum;
    uint_t PosInOrnament;
    int_t TonSlide;
    int_t Glissade;
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(std::move(data))
    {
    }

    void Reset() override
    {
      std::fill(PlayerState.begin(), PlayerState.end(), ChannelState());
    }

    void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track) override
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(state, track);
    }

  private:
    void GetNewLineState(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            GetNewChannelState(*src, PlayerState[chan], track);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        dst.Enabled = *enabled;
        dst.PosInSample = 0;
        dst.PosInOrnament = 0;
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
        dst.PosInSample = 0;
        dst.PosInOrnament = 0;
        dst.TonSlide = 0;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleNum = *sample;
        dst.PosInSample = 0;
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentNum = *ornament;
        dst.PosInOrnament = 0;
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Volume = *volume;
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          if (it->Param1)
          {
            track.SetEnvelopeType(it->Param1);
            track.SetEnvelopeTone(it->Param2);
          }
          dst.Envelope = true;
          break;
        case NOENVELOPE:
          dst.Envelope = false;
          break;
        case GLISS:
          dst.Glissade = it->Param1;
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }

    void SynthesizeChannelsData(const TrackState& state, AYM::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(state, PlayerState[chan], channel, track);
      }
    }

    void SynthesizeChannel(const TrackState& state, ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        channel.DisableTone();
        channel.DisableNoise();
        return;
      }

      const Sample& curSample = Data->Samples.Get(dst.SampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data->Ornaments.Get(dst.OrnamentNum);

      //calculate tone
      dst.TonSlide += dst.Glissade;

      //apply level
      channel.SetLevel(int_t(curSampleLine.Level) - dst.Volume);
      //apply envelope
      if (curSampleLine.EnvelopeMask && dst.Envelope)
      {
        channel.EnableEnvelope();
      }
      //apply tone
      const int_t halftones = int_t(dst.Note) +
                              Data->Order->GetTransposition(state.Position()) +
                              (dst.Envelope ? 0 : curOrnament.GetLine(dst.PosInOrnament));
      channel.SetTone(halftones, dst.TonSlide + curSampleLine.Vibrato);
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      //apply noise
      if (!curSampleLine.NoiseMask)
      {
        track.SetNoise(curSampleLine.Noise);
      }
      else
      {
        channel.DisableNoise();
      }

      if (++dst.PosInOrnament >= curOrnament.GetSize())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }

      if (++dst.PosInSample >= curSample.GetSize())
      {
        if (curSample.GetLoop() >= 0)
        {
          dst.PosInSample = curSample.GetLoop();
        }
        else
        {
          dst.Enabled = false;
        }
      }
    }
  private:
    const ModuleData::Ptr Data;
    std::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
  };

  class Chiptune : public AYM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(std::move(data))
      , Properties(std::move(properties))
      , Info(CreateTrackInfo(Data, AYM::TRACK_CHANNELS))
    {
    }

    Information::Ptr GetInformation() const override
    {
      return Info;
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const override
    {
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const AYM::DataRenderer::Ptr renderer = MakePtr<DataRenderer>(Data);
      return AYM::CreateDataIterator(trackParams, iterator, renderer);
    }
  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
    const Information::Ptr Info;
  };

  class Factory : public AYM::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::SoundTrackerPro::Decoder::Ptr decoder)
      : Decoder(std::move(decoder))
    {
    }

    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const Formats::Chiptune::Container::Ptr container = Decoder->Parse(rawData, dataBuilder))
      {
        props.SetSource(*container);
        return MakePtr<Chiptune>(dataBuilder.GetResult(), properties);
      }
      else
      {
        return AYM::Chiptune::Ptr();
      }
    }
  private:
    const Formats::Chiptune::SoundTrackerPro::Decoder::Ptr Decoder;
  };

  Factory::Ptr CreateFactory(Formats::Chiptune::SoundTrackerPro::Decoder::Ptr decoder)
  {
    return MakePtr<Factory>(decoder);
  }
}
}
