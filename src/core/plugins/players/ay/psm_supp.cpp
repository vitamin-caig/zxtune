/*
Abstract:
  PSM modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "aym_base.h"
#include "aym_base_track.h"
#include "aym_plugin.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/simple_orderlist.h"
//common includes
#include <tools.h>
//library includes
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/aym/prosoundmaker.h>
#include <math/numeric.h>

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

  struct Sample : public Formats::Chiptune::ProSoundMaker::Sample
  {
    Sample()
      : Formats::Chiptune::ProSoundMaker::Sample()
    {
    }

    Sample(const Formats::Chiptune::ProSoundMaker::Sample& rh)
      : Formats::Chiptune::ProSoundMaker::Sample(rh)
    {
    }

    const uint_t* GetLoop() const
    {
      return Loop ? &*Loop : 0;
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

  struct Ornament : public Formats::Chiptune::ProSoundMaker::Ornament
  {
    Ornament()
      : Formats::Chiptune::ProSoundMaker::Ornament()
    {
    }

    Ornament(const Formats::Chiptune::ProSoundMaker::Ornament& rh)
      : Formats::Chiptune::ProSoundMaker::Ornament(rh)
    {
    }

    const uint_t* GetLoop() const
    {
      return Loop ? &*Loop : 0;
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    int_t GetLine(uint_t idx) const
    {
      return Lines.size() > idx ? Lines[idx] : 0;
    }
  };

  typedef SimpleOrderListWithTransposition<Formats::Chiptune::ProSoundMaker::PositionEntry> OrderListWithTransposition;

  class ModuleData : public TrackModel
  {
  public:
    typedef boost::shared_ptr<const ModuleData> Ptr;

    ModuleData()
      : InitialTempo()
    {
    }

    virtual uint_t GetInitialTempo() const
    {
      return InitialTempo;
    }

    virtual const OrderList& GetOrder() const
    {
      return *Order;
    }

    virtual const PatternsSet& GetPatterns() const
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
    explicit DataBuilder(PropertiesBuilder& props)
      : Data(boost::make_shared<ModuleData>())
      , Properties(props)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
    {
      Data->Patterns = Patterns.GetResult();
      Properties.SetFreqtable(TABLE_PROSOUNDMAKER);
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return Properties;
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::ProSoundMaker::Sample& sample)
    {
      Data->Samples.Add(index, Sample(sample));
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::ProSoundMaker::Ornament& ornament)
    {
      Data->Ornaments.Add(index, Ornament(ornament));
    }

    virtual void SetPositions(const std::vector<Formats::Chiptune::ProSoundMaker::PositionEntry>& positions, uint_t loop)
    {
      Data->Order = boost::make_shared<OrderListWithTransposition>(loop, positions.begin(), positions.end());
    }

    virtual Formats::Chiptune::PatternBuilder& StartPattern(uint_t index)
    {
      Patterns.SetPattern(index);
      return Patterns;
    }

    virtual void StartChannel(uint_t index)
    {
      Patterns.SetChannel(index);
    }

    virtual void SetRest()
    {
      Patterns.GetChannel().SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      Patterns.GetChannel().SetEnabled(true);
      Patterns.GetChannel().SetNote(note);
    }

    virtual void SetSample(uint_t sample)
    {
      Patterns.GetChannel().SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Patterns.GetChannel().SetOrnament(ornament);
    }

    virtual void SetVolume(uint_t volume)
    {
      Patterns.GetChannel().SetVolume(volume);
    }

    virtual void DisableOrnament()
    {
      Patterns.GetChannel().AddCommand(NOORNAMENT);
    }

    virtual void SetEnvelopeType(uint_t type)
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

    virtual void SetEnvelopeTone(uint_t tone)
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

    virtual void SetEnvelopeNote(uint_t note)
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

    ModuleData::Ptr GetResult() const
    {
      return Data;
    }
  private:
    const boost::shared_ptr<ModuleData> Data;
    PropertiesBuilder& Properties;
    PatternsBuilder Patterns;
  };

  struct SampleState
  {
    SampleState()
      : Current(0)
      , Position(0)
      , LoopsCount(0)
    {
    }

    const Sample* Current;
    uint_t Position;
    uint_t LoopsCount;
  };

  struct OrnamentState
  {
    OrnamentState()
      : Current(0)
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
       : Data(data)
    {
      Reset();
    }

    virtual void Reset()
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        ChannelState& state = PlayerState[chan];
        state = ChannelState();
        state.Smp.Current = Data->Samples.Find(0);
        state.Orn.Current = Data->Ornaments.Find(0);
      }
    }

    virtual void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track)
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
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.Smp.Current = Data->Samples.Find(*sample);
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        if (*ornament != 0x20 &&
            *ornament != 0x21)
        {
          dst.Orn.Current = Data->Ornaments.Find(*ornament);
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

      if (++dst.Smp.Position >= curSample.GetSize())
      {
        if (const uint_t* loop = curSample.GetLoop())
        {
          dst.Smp.Position = *loop;
          if (!--dst.Smp.LoopsCount)
          {
            dst.Smp.LoopsCount = curSample.VolumeDeltaPeriod;
            dst.VolumeDelta = Math::Clamp<int_t>(int_t(dst.VolumeDelta) + curSample.VolumeDeltaValue, 0, 15);
          }
        }
        else
        {
          dst.Enabled = false;
        }
      }

      if (++dst.Orn.Position >= curOrnament.GetSize())
      {
        if (const uint_t* loop = curOrnament.GetLoop())
        {
          dst.Orn.Position = *loop;
        }
        else
        {
          dst.Orn.Finished = true;
        }
      }
    }
  private:
    const ModuleData::Ptr Data;
    boost::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
  };

  class Chiptune : public AYM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, AYM::TRACK_CHANNELS))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const
    {
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const AYM::DataRenderer::Ptr renderer = boost::make_shared<DataRenderer>(Data);
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
    virtual AYM::Chiptune::Ptr CreateChiptune(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      DataBuilder dataBuilder(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ProSoundMaker::ParseCompiled(rawData, dataBuilder))
      {
        propBuilder.SetSource(*container);
        return boost::make_shared<Chiptune>(dataBuilder.GetResult(), propBuilder.GetResult());
      }
      else
      {
        return AYM::Chiptune::Ptr();
      }
    }
  };
}
}

namespace ZXTune
{
  void RegisterPSMSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'P', 'S', 'M', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProSoundMakerCompiledDecoder();
    const Module::AYM::Factory::Ptr factory = boost::make_shared<Module::ProSoundMaker::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
