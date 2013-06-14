/*
Abstract:
  ASC modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/simple_orderlist.h"
//common includes
#include <error_tools.h>
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <core/conversion/aym.h>
#include <formats/chiptune/aym/ascsoundmaster.h>
#include <math/numeric.h>

namespace ASCSoundMaster
{
  //supported commands and their parameters
  enum CmdType
  {
    //no parameters
    EMPTY,
    //r13,envL
    ENVELOPE,
    //no parameters
    ENVELOPE_ON,
    //no parameters
    ENVELOPE_OFF,
    //value
    NOISE,
    //no parameters
    CONT_SAMPLE,
    //no parameters
    CONT_ORNAMENT,
    //glissade value
    GLISS,
    //steps
    SLIDE,
    //steps,note
    SLIDE_NOTE,
    //period,delta
    AMPLITUDE_SLIDE,
    //no parameter
    BREAK_SAMPLE
  };

  struct Sample : public Formats::Chiptune::ASCSoundMaster::Sample
  {
    Sample()
      : Formats::Chiptune::ASCSoundMaster::Sample()
    {
    }

    Sample(const Formats::Chiptune::ASCSoundMaster::Sample& rh)
      : Formats::Chiptune::ASCSoundMaster::Sample(rh)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetLoopLimit() const
    {
      return LoopLimit;
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

  struct Ornament : public Formats::Chiptune::ASCSoundMaster::Ornament
  {
    Ornament()
      : Formats::Chiptune::ASCSoundMaster::Ornament()
    {
    }

    Ornament(const Formats::Chiptune::ASCSoundMaster::Ornament& rh)
      : Formats::Chiptune::ASCSoundMaster::Ornament(rh)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetLoopLimit() const
    {
      return LoopLimit;
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

  using namespace ZXTune;
  using namespace ZXTune::Module;

  class ModuleData : public TrackModel
  {
  public:
    typedef boost::shared_ptr<ModuleData> RWPtr;
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
    OrderList::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<Sample> Samples;
    SparsedObjectsStorage<Ornament> Ornaments;
  };

  AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties);
}

namespace ASCSoundMaster
{
  class DataBuilder : public Formats::Chiptune::ASCSoundMaster::Builder
  {
  public:
    DataBuilder(ModuleData& data, PropertiesBuilder& props)
      : Data(data)
      , Properties(props)
      , Builder(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
    {
      Data.Patterns = Builder.GetPatterns();
      Properties.SetFreqtable(TABLE_ASM);
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return Properties;
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data.InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::ASCSoundMaster::Sample& sample)
    {
      Data.Samples.Add(index, Sample(sample));
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::ASCSoundMaster::Ornament& ornament)
    {
      Data.Ornaments.Add(index, Ornament(ornament));
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      Data.Order = boost::make_shared<SimpleOrderList>(loop, positions.begin(), positions.end());
    }

    virtual Formats::Chiptune::PatternBuilder& StartPattern(uint_t index)
    {
      Builder.SetPattern(index);
      return Builder;
    }

    virtual void StartChannel(uint_t index)
    {
      Builder.SetChannel(index);
    }

    virtual void SetRest()
    {
      Builder.GetChannel().SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      if (!Builder.GetChannel().FindCommand(BREAK_SAMPLE))
      {
        Builder.GetChannel().SetEnabled(true);
      }
      if (Command* cmd = Builder.GetChannel().FindCommand(SLIDE))
      {
        //set slide to note
        cmd->Type = SLIDE_NOTE;
        cmd->Param2 = note;
      }
      else
      {
        Builder.GetChannel().SetNote(note);
      }
    }

    virtual void SetSample(uint_t sample)
    {
      Builder.GetChannel().SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Builder.GetChannel().SetOrnament(ornament);
    }

    virtual void SetVolume(uint_t vol)
    {
      Builder.GetChannel().SetVolume(vol);
    }

    virtual void SetEnvelopeType(uint_t type)
    {
      if (Command* cmd = Builder.GetChannel().FindCommand(ENVELOPE))
      {
        cmd->Param1 = int_t(type);
      }
      else
      {
        Builder.GetChannel().AddCommand(ENVELOPE, int_t(type), -1);
      }
    }

    virtual void SetEnvelopeTone(uint_t tone)
    {
      if (Command* cmd = Builder.GetChannel().FindCommand(ENVELOPE))
      {
        cmd->Param2 = int_t(tone);
      }
      else
      {
        //strange situation
        Builder.GetChannel().AddCommand(ENVELOPE, -1, int_t(tone));
      }
    }

    virtual void SetEnvelope()
    {
      Builder.GetChannel().AddCommand(ENVELOPE_ON);
    }

    virtual void SetNoEnvelope()
    {
      Builder.GetChannel().AddCommand(ENVELOPE_OFF);
    }

    virtual void SetNoise(uint_t val)
    {
      Builder.GetChannel().AddCommand(NOISE, val);
    }

    virtual void SetContinueSample()
    {
      Builder.GetChannel().AddCommand(CONT_SAMPLE);
    }

    virtual void SetContinueOrnament()
    {
      Builder.GetChannel().AddCommand(CONT_ORNAMENT);
    }

    virtual void SetGlissade(int_t val)
    {
      Builder.GetChannel().AddCommand(GLISS, val);
    }

    virtual void SetSlide(int_t steps)
    {
      Builder.GetChannel().AddCommand(SLIDE, steps);
    }

    virtual void SetVolumeSlide(uint_t period, int_t delta)
    {
      Builder.GetChannel().AddCommand(AMPLITUDE_SLIDE, period, delta);
    }

    virtual void SetBreakSample()
    {
      Builder.GetChannel().AddCommand(BREAK_SAMPLE);
    }

  private:
    ModuleData& Data;
    PropertiesBuilder& Properties;
    PatternsBuilder Builder;
  };

  const uint_t LIMITER(~uint_t(0));

  struct ChannelState
  {
    ChannelState()
      : Enabled(false), Envelope(false), BreakSample(false)
      , Volume(15), VolumeAddon(0), VolSlideDelay(0), VolSlideAddon(), VolSlideCounter(0)
      , BaseNoise(0), CurrentNoise(0)
      , Note(0), NoteAddon(0)
      , SampleNum(0), CurrentSampleNum(0), PosInSample(0)
      , OrnamentNum(0), CurrentOrnamentNum(0), PosInOrnament(0)
      , ToneDeviation(0)
      , SlidingSteps(0), Sliding(0), SlidingTargetNote(LIMITER), Glissade(0)
    {
    }
    bool Enabled;
    bool Envelope;
    bool BreakSample;
    uint_t Volume;
    int_t VolumeAddon;
    uint_t VolSlideDelay;
    int_t VolSlideAddon;
    uint_t VolSlideCounter;
    int_t BaseNoise;
    int_t CurrentNoise;
    uint_t Note;
    int_t NoteAddon;
    uint_t SampleNum;
    uint_t CurrentSampleNum;
    uint_t PosInSample;
    uint_t OrnamentNum;
    uint_t CurrentOrnamentNum;
    uint_t PosInOrnament;
    int_t ToneDeviation;
    int_t SlidingSteps;//may be infinite (negative)
    int_t Sliding;
    uint_t SlidingTargetNote;
    int_t Glissade;

    void ResetBaseNoise()
    {
      BaseNoise = 0;
    }
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(data)
      , EnvelopeTone(0)
    {
    }

    virtual void Reset()
    {
      std::fill(PlayerState.begin(), PlayerState.end(), ChannelState());
    }

    virtual void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(track);
    }
  private:
    void GetNewLineState(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Line())
      {
        std::for_each(PlayerState.begin(), PlayerState.end(), std::mem_fun_ref(&ChannelState::ResetBaseNoise));
      }
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
      dst.VolSlideCounter = 0;
      dst.SlidingSteps = 0;
      bool contSample = false, contOrnament = false;
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          if (-1 != it->Param1)
          {
            track.SetEnvelopeType(it->Param1);
          }
          if (-1 != it->Param2)
          {
            EnvelopeTone = it->Param2;
            track.SetEnvelopeTone(EnvelopeTone);
          }
          break;
        case ENVELOPE_ON:
          dst.Envelope = true;
          break;
        case ENVELOPE_OFF:
          dst.Envelope = false;
          break;
        case NOISE:
          dst.BaseNoise = it->Param1;
          break;
        case CONT_SAMPLE:
          contSample = true;
          break;
        case CONT_ORNAMENT:
          contOrnament = true;
          break;
        case GLISS:
          dst.Glissade = it->Param1;
          dst.SlidingSteps = -1;//infinite sliding
          break;
        case SLIDE:
        {
          dst.SlidingSteps = it->Param1;
          const int_t newSliding = (dst.Sliding | 0xf) ^ 0xf;
          dst.Glissade = -newSliding / (dst.SlidingSteps ? dst.SlidingSteps : 1);
          dst.Sliding = dst.Glissade * dst.SlidingSteps;
          break;
        }
        case SLIDE_NOTE:
        {
          dst.SlidingSteps = it->Param1;
          dst.SlidingTargetNote = it->Param2;
          const int_t absoluteSliding = track.GetSlidingDifference(dst.Note, dst.SlidingTargetNote);
          const int_t newSliding = absoluteSliding - (contSample ? dst.Sliding / 16 : 0);
          dst.Glissade = 16 * newSliding / (dst.SlidingSteps ? dst.SlidingSteps : 1);
          break;
        }
        case AMPLITUDE_SLIDE:
          dst.VolSlideCounter = dst.VolSlideDelay = it->Param1;
          dst.VolSlideAddon = it->Param2;
          break;
        case BREAK_SAMPLE:
          dst.BreakSample = true;
          break;
        default:
          assert(!"Invalid cmd");
          break;
        }
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentNum = *ornament;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleNum = *sample;
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
        dst.CurrentNoise = dst.BaseNoise;
        if (dst.SlidingSteps <= 0)
        {
          dst.Sliding = 0;
        }
        if (!contSample)
        {
          dst.CurrentSampleNum = dst.SampleNum;
          dst.PosInSample = 0;
          dst.VolumeAddon = 0;
          dst.ToneDeviation = 0;
          dst.BreakSample = false;
        }
        if (!contOrnament)
        {
          dst.CurrentOrnamentNum = dst.OrnamentNum;
          dst.PosInOrnament = 0;
          dst.NoteAddon = 0;
        }
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Volume = *volume;
      }
    }

    void SynthesizeChannelsData(AYM::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(PlayerState[chan], channel, track);
      }
    }

    void SynthesizeChannel(ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        return;
      }

      const Sample& curSample = Data->Samples.Get(dst.CurrentSampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data->Ornaments.Get(dst.CurrentOrnamentNum);
      const Ornament::Line& curOrnamentLine = curOrnament.GetLine(dst.PosInOrnament);

      //calculate volume addon
      if (dst.VolSlideCounter >= 2)
      {
        dst.VolSlideCounter--;
      }
      else if (dst.VolSlideCounter)
      {
        dst.VolumeAddon += dst.VolSlideAddon;
        dst.VolSlideCounter = dst.VolSlideDelay;
      }
      dst.VolumeAddon += curSampleLine.VolSlide;
      dst.VolumeAddon = Math::Clamp<int_t>(dst.VolumeAddon, -15, 15);

      //calculate tone
      dst.ToneDeviation += curSampleLine.ToneDeviation;
      dst.NoteAddon += curOrnamentLine.NoteAddon;
      const int_t halfTone = int_t(dst.Note) + dst.NoteAddon;
      const int_t toneAddon = dst.ToneDeviation + dst.Sliding / 16;
      //apply tone
      channel.SetTone(halfTone, toneAddon);

      //apply level
      channel.SetLevel((dst.Volume + 1) * Math::Clamp<int_t>(dst.VolumeAddon + curSampleLine.Level, 0, 15) / 16);
      //apply envelope
      if (dst.Envelope && curSampleLine.EnableEnvelope)
      {
        channel.EnableEnvelope();
      }

      //calculate noise
      dst.CurrentNoise += curOrnamentLine.NoiseAddon;

      //mixer
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      if (curSampleLine.NoiseMask && curSampleLine.EnableEnvelope)
      {
        EnvelopeTone += curSampleLine.Adding;
        track.SetEnvelopeTone(EnvelopeTone);
      }
      else
      {
        dst.CurrentNoise += curSampleLine.Adding;
      }

      if (!curSampleLine.NoiseMask)
      {
        track.SetNoise((dst.CurrentNoise + dst.Sliding / 256) & 0x1f);
      }
      else
      {
        channel.DisableNoise();
      }

      //recalc positions
      if (dst.SlidingSteps != 0)
      {
        if (dst.SlidingSteps > 0)
        {
          if (!--dst.SlidingSteps &&
              LIMITER != dst.SlidingTargetNote) //finish slide to note
          {
            dst.Note = dst.SlidingTargetNote;
            dst.SlidingTargetNote = LIMITER;
            dst.Sliding = dst.Glissade = 0;
          }
        }
        dst.Sliding += dst.Glissade;
      }

      if (dst.PosInSample++ >= curSample.GetLoopLimit())
      {
        if (!dst.BreakSample)
        {
          dst.PosInSample = curSample.GetLoop();
        }
        else if (dst.PosInSample >= curSample.GetSize())
        {
          dst.Enabled = false;
        }
      }
      if (dst.PosInOrnament++ >= curOrnament.GetLoopLimit())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }
    }
  private:
    const ModuleData::Ptr Data;
    uint_t EnvelopeTone;
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
}

namespace ASCSoundMaster
{
  AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
  {
    return boost::make_shared<Chiptune>(data, properties);
  }
}

namespace ASC
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID_0[] = {'A', 'S', '0', 0};
  const Char ID_1[] = {'A', 'S', 'C', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | SupportedAYMFormatConvertors;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder)
      : Decoder(decoder)
    {
    }

    virtual bool Check(const Binary::Container& data) const
    {
      return Decoder->Check(data);
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Holder::Ptr CreateModule(PropertiesBuilder& properties, Binary::Container::Ptr rawData) const
    {
      const ::ASCSoundMaster::ModuleData::RWPtr modData = boost::make_shared< ::ASCSoundMaster::ModuleData>();
      ::ASCSoundMaster::DataBuilder dataBuilder(*modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Decoder->Parse(*rawData, dataBuilder))
      {
        properties.SetSource(container);
        const AYM::Chiptune::Ptr chiptune = ::ASCSoundMaster::CreateChiptune(modData, properties.GetResult());
        return AYM::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::ASCSoundMaster::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterASCSupport(PlayerPluginsRegistrator& registrator)
  {
    {
      const Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder = Formats::Chiptune::ASCSoundMaster::Ver0::CreateDecoder();
      const ModulesFactory::Ptr factory = boost::make_shared<ASC::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ASC::ID_0, decoder->GetDescription(), ASC::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
    {
      const Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder = Formats::Chiptune::ASCSoundMaster::Ver1::CreateDecoder();
      const ModulesFactory::Ptr factory = boost::make_shared<ASC::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ASC::ID_1, decoder->GetDescription(), ASC::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
