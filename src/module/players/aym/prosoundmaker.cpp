/**
* 
* @file
*
* @brief  ProSoundMaker chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "prosoundmaker.h"
#include "aym_base.h"
#include "aym_base_track.h"
#include "aym_properties_helper.h"
//common includes
#include <make_ptr.h>
//library includes
#include <formats/chiptune/aym/prosoundmaker.h>
#include <math/numeric.h>
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>
//boost includes
#include <boost/optional.hpp>

namespace Module
{
namespace ProSoundMaker
{
  //supported commands and parameters
  enum CmdType
  {
    //no parameters
    EMPTY,
    //r13,period,note delta
    ENVELOPE,
    //disable ornament
    NOORNAMENT,
  };

  using Formats::Chiptune::ProSoundMaker::Sample;
  using Formats::Chiptune::ProSoundMaker::Ornament;

  typedef SimpleOrderListWithTransposition<Formats::Chiptune::ProSoundMaker::PositionEntry> OrderListWithTransposition;

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

  class DataBuilder : public Formats::Chiptune::ProSoundMaker::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetFrequencyTable(TABLE_PROSOUNDMAKER);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetSample(uint_t index, Formats::Chiptune::ProSoundMaker::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::ProSoundMaker::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(std::vector<Formats::Chiptune::ProSoundMaker::PositionEntry> positions, uint_t loop) override
    {
      Data->Order = MakePtr<OrderListWithTransposition>(loop, std::move(positions));
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

    void SetVolume(uint_t volume) override
    {
      Patterns.GetChannel().SetVolume(volume);
    }

    void DisableOrnament() override
    {
      Patterns.GetChannel().AddCommand(NOORNAMENT);
    }

    void SetEnvelopeType(uint_t type) override
    {
      MutableCell& channel = Patterns.GetChannel();
      if (Command* cmd = channel.FindCommand(ENVELOPE))
      {
        cmd->Param1 = int_t(type);
      }
      else
      {
        channel.AddCommand(ENVELOPE, int_t(type), -1, -1);
      }
    }

    void SetEnvelopeTone(uint_t tone) override
    {
      MutableCell& channel = Patterns.GetChannel();
      if (Command* cmd = channel.FindCommand(ENVELOPE))
      {
        cmd->Param2 = int_t(tone);
      }
      else
      {
        channel.AddCommand(ENVELOPE, -1, int_t(tone), -1);
      }
    }

    void SetEnvelopeNote(uint_t note) override
    {
      MutableCell& channel = Patterns.GetChannel();
      if (Command* cmd = channel.FindCommand(ENVELOPE))
      {
        cmd->Param3 = int_t(note);
      }
      else
      {
        channel.AddCommand(ENVELOPE, -1, -1, int_t(note));
      }
    }

    ModuleData::Ptr CaptureResult()
    {
      Data->Patterns = Patterns.CaptureResult();
      return std::move(Data);
    }
  private:
    AYM::PropertiesHelper& Properties;
    MetaProperties Meta;
    PatternsBuilder Patterns;
    ModuleData::RWPtr Data;
  };

  struct SampleState
  {
    SampleState()
      : Current(nullptr)
      , Position(0)
      , LoopsCount(0)
      , Finished(false)
    {
    }

    const Sample* Current;
    uint_t Position;
    uint_t LoopsCount;
    bool Finished;
  };

  struct OrnamentState
  {
    OrnamentState()
      : Current(nullptr)
      , Position(0)
      , Finished()
      , Disabled()
    {
    }

    const Ornament* Current;
    uint_t Position;
    bool Finished;
    bool Disabled;
  };

  struct ChannelState
  {
    ChannelState()
      : Enabled(false), Envelope(false)
      , Note()
      , VolumeDelta(), BaseVolumeDelta()
      , Slide()
    {
    }
    bool Enabled;
    bool Envelope;
    boost::optional<uint_t> EnvelopeNote;
    boost::optional<uint_t> Note;
    SampleState Smp;
    OrnamentState Orn;
    uint_t VolumeDelta;
    uint_t BaseVolumeDelta;
    int_t Slide;
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
       : Data(std::move(data))
    {
      Reset();
    }

    void Reset() override
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        ChannelState& state = PlayerState[chan];
        state = ChannelState();
        state.Smp.Current = &Data->Samples.Get(0);
        state.Orn.Current = &Data->Ornaments.Get(0);
      }
    }

    void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track) override
    {
      if (0 == state.Quirk())
      {
        //start pattern
        if (0 == state.Line())
        {
          ResetNotes();
        }
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(state, track);
    }
  private:
    void ResetNotes()
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        PlayerState[chan].Note.reset();
      }
    }

    void GetNewLineState(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (const auto line = state.LineObject())
      {
        for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
        {
          if (const auto src = line->GetChannel(chan))
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
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.Smp.Current = &Data->Samples.Get(*sample);
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        if (*ornament != 0x20 &&
            *ornament != 0x21)
        {
          dst.Orn.Current = &Data->Ornaments.Get(*ornament);
          dst.Orn.Position = 0;
          dst.Orn.Finished = dst.Orn.Disabled = false;
          dst.Envelope = false;
        }
        else
        {
          static const Ornament STUB_ORNAMENT;
          dst.Orn.Current = &STUB_ORNAMENT;
        }
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.BaseVolumeDelta = *volume;
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          if (it->Param1 != -1)
          {
            track.SetEnvelopeType(it->Param1);
          }
          if (it->Param2 != -1)
          {
            track.SetEnvelopeTone(it->Param2);
            dst.EnvelopeNote.reset();
          }
          else if (it->Param3 != -1)
          {
            dst.EnvelopeNote = it->Param3;
          }
          dst.Envelope = true;
          break;
        case NOORNAMENT:
          dst.Envelope = false;
          dst.Orn.Finished = dst.Orn.Disabled = true;
          break;
        default:
          assert(!"Invalid command");
        }
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
        dst.VolumeDelta = dst.BaseVolumeDelta;
        dst.Smp.Position = 0;
        dst.Smp.Finished = false;
        dst.Slide = 0;
        dst.Smp.LoopsCount = 1;
        dst.Orn.Position = 0;
        if (!dst.Orn.Disabled)
        {
          dst.Orn.Finished = false;
        }
        if (dst.Envelope && dst.EnvelopeNote)
        {
          uint_t envNote = *dst.Note + *dst.EnvelopeNote;
          if (envNote >= 0x60)
          {
            envNote -= 0x60;
          }
          else if (envNote >= 0x30)
          {
            envNote -= 0x30;
          }
          const uint_t envFreq = track.GetFrequency((envNote & 0x7f) + 0x30);
          track.SetEnvelopeTone(envFreq & 0xff);
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
      const Ornament& curOrnament = *dst.Orn.Current;
      const int_t ornamentLine = (dst.Orn.Finished || dst.Envelope) ? 0 : curOrnament.GetLine(dst.Orn.Position);
      const Sample& curSample = *dst.Smp.Current;
      const Sample::Line& curSampleLine = curSample.GetLine(dst.Smp.Position);

      dst.Slide += curSampleLine.Gliss;
      const int_t curNote = dst.Note ? int_t(*dst.Note) + Data->Order->GetTransposition(state.Position()) : 0;
      const int_t halftones = Math::Clamp<int_t>(curNote + ornamentLine, 0, 95);
      const int_t tone = Math::Clamp<int_t>(track.GetFrequency(halftones) + dst.Slide, 0, 4095);
      channel.SetTone(tone);
      
      //emulate level construction due to possibility of envelope bit reset
      int_t level = int_t(curSampleLine.Level) + (dst.Envelope ? 16 : 0) + dst.VolumeDelta - 15;
      if (!dst.Enabled || level < 0)
      {
        level = 0;
      }
      channel.SetLevel(level & 15);
      if (level & 16)
      {
        channel.EnableEnvelope();
      }
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      if (!curSampleLine.NoiseMask && dst.Enabled)
      {
        if (level)
        {
          track.SetNoise(curSampleLine.Noise);
        }
      }
      else
      {
        channel.DisableNoise();
      }

      if (!dst.Smp.Finished && ++dst.Smp.Position >= curSample.GetSize())
      {
        const auto loop = curSample.GetLoop();
        if (loop < curSample.GetSize())
        {
          dst.Smp.Position = loop;
          if (!--dst.Smp.LoopsCount)
          {
            dst.Smp.LoopsCount = curSample.VolumeDeltaPeriod;
            dst.VolumeDelta = Math::Clamp<int_t>(int_t(dst.VolumeDelta) + curSample.VolumeDeltaValue, 0, 15);
          }
        }
        else
        {
          dst.Enabled = false;
          dst.Smp.Finished = true;
        }
      }

      if (!dst.Orn.Finished && ++dst.Orn.Position >= curOrnament.GetSize())
      {
        const auto loop = curOrnament.GetLoop();
        if (loop < curOrnament.GetSize())
        {
          dst.Orn.Position = loop;
        }
        else
        {
          dst.Orn.Finished = true;
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
    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ProSoundMaker::ParseCompiled(rawData, dataBuilder))
      {
        props.SetSource(*container);
        return MakePtr<Chiptune>(dataBuilder.CaptureResult(), properties);
      }
      else
      {
        return AYM::Chiptune::Ptr();
      }
    }
  };

  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}
}
