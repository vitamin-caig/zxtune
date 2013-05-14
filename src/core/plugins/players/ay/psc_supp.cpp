/*
Abstract:
  PSC modules playback support

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
#include <byteorder.h>
#include <error_tools.h>
#include <range_checker.h>
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <core/conversion/aym.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/aym/prosoundcreator.h>
#include <math/numeric.h>

namespace ProSoundCreator
{
  /*
    Do not use GLISS_NOTE command due to possible ambiguation while parsing
  */
  //supported commands and parameters
  enum CmdType
  {
    //no parameters
    EMPTY,
    //r13,period or nothing
    ENVELOPE,
    //no parameters
    NOENVELOPE,
    //noise base
    NOISE_BASE,
    //no params
    BREAK_SAMPLE,
    //no params
    BREAK_ORNAMENT,
    //no params
    NO_ORNAMENT,
    //absolute gliss
    GLISS,
    //step
    SLIDE,
    //period, delta
    VOLUME_SLIDE
  };

  struct Sample : public Formats::Chiptune::ProSoundCreator::Sample
  {
    Sample()
      : Formats::Chiptune::ProSoundCreator::Sample()
    {
    }

    Sample(const Formats::Chiptune::ProSoundCreator::Sample& rh)
      : Formats::Chiptune::ProSoundCreator::Sample(rh)
    {
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

  struct Ornament : public Formats::Chiptune::ProSoundCreator::Ornament
  {
    Ornament()
      : Formats::Chiptune::ProSoundCreator::Ornament()
    {
    }

    Ornament(const Formats::Chiptune::ProSoundCreator::Ornament& rh)
      : Formats::Chiptune::ProSoundCreator::Ornament(rh)
    {
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

  template<class Object>
  class ObjectLinesIterator
  {
  public:
    ObjectLinesIterator()
      : Obj()
      , Position()
      , LoopPosition()
      , Break()
    {
    }

    void Set(const Object* obj)
    {
      //do not reset for original position update algo
      Obj = obj;
    }

    void Reset()
    {
      Position = 0;
      Break = false;
    }

    void Disable()
    {
      Position = Obj->GetSize();
    }

    const typename Object::Line* GetLine() const
    {
      return Position < Obj->GetSize()
        ? &Obj->GetLine(Position)
        : 0;
    }

    void SetBreakLoop(bool brk)
    {
      Break = brk;
    }

    void Next()
    {
      const typename Object::Line& curLine = Obj->GetLine(Position);
      if (curLine.LoopBegin)
      {
        LoopPosition = Position;
      }
      if (curLine.LoopEnd)
      {
        if (!Break)
        {
          Position = LoopPosition;
        }
        else
        {
          Break = false;
          ++Position;
        }
      }
      else
      {
        ++Position;
      }
    }
  private:
    const Object* Obj;
    uint_t Position;
    uint_t LoopPosition;
    bool Break;
  };

  class ToneSlider
  {
  public:
    ToneSlider()
      : Sliding()
      , Glissade()
      , Steps()
    {
    }

    void SetSliding(int_t sliding)
    {
      Sliding = sliding;
    }

    void SetSlidingSteps(uint_t steps)
    {
      Steps = steps;
    }

    void SetGlissade(int_t glissade)
    {
      Glissade = glissade;
    }

    void Reset()
    {
      Glissade = 0;
    }

    int_t GetSliding() const
    {
      return Sliding;
    }

    int_t Update()
    {
      if (Glissade)
      {
        Sliding += Glissade;
        if (Steps && !--Steps)
        {
          Glissade = 0;
        }
        return Sliding;
      }
      else
      {
        return 0;
      }
    }
  private:
    int_t Sliding;
    int_t Glissade;
    uint_t Steps;
  };

  class VolumeSlider
  {
  public:
    VolumeSlider()
      : Counter()
      , Period()
      , Delta()
    {
    }

    void Reset()
    {
      Counter = 0;
    }

    void SetParams(uint_t period, int_t delta)
    {
      Period = Counter = period;
      Delta = delta;
    }

    int_t Update()
    {
      if (Counter && !--Counter)
      {
        Counter = Period;
        return Delta;
      }
      else
      {
        return 0;
      }
    }
  private:
    uint_t Counter;
    uint_t Period;
    int_t Delta;
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

  std::auto_ptr<Formats::Chiptune::ProSoundCreator::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props);
  AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties);
}

namespace ProSoundCreator
{
  class DataBuilder : public Formats::Chiptune::ProSoundCreator::Builder
  {
  public:
    DataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
      : Data(data)
      , Properties(props)
      , Builder(PatternsBuilder::Create<Devices::AYM::CHANNELS>())
    {
      Data->Patterns = Builder.GetPatterns();
      Properties->SetFreqtable(TABLE_ASM);
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return *Properties;
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::ProSoundCreator::Sample& sample)
    {
      Data->Samples.Add(index, Sample(sample));
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::ProSoundCreator::Ornament& ornament)
    {
      Data->Ornaments.Add(index, Ornament(ornament));
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      Data->Order = boost::make_shared<SimpleOrderList>(loop, positions.begin(), positions.end());
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
      MutableCell& channel = Builder.GetChannel();
      channel.SetEnabled(true);
      channel.SetNote(note);
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

    virtual void SetEnvelope(uint_t type, uint_t value)
    {
      Builder.GetChannel().AddCommand(ENVELOPE, type, value);
    }

    virtual void SetEnvelope()
    {
      Builder.GetChannel().AddCommand(ENVELOPE);
    }

    virtual void SetNoEnvelope()
    {
      Builder.GetChannel().AddCommand(NOENVELOPE);
    }

    virtual void SetNoiseBase(uint_t val)
    {
      Builder.GetChannel().AddCommand(NOISE_BASE, val);
    }

    virtual void SetBreakSample()
    {
      Builder.GetChannel().AddCommand(BREAK_SAMPLE);
    }

    virtual void SetBreakOrnament()
    {
      Builder.GetChannel().AddCommand(BREAK_ORNAMENT);
    }

    virtual void SetNoOrnament()
    {
      Builder.GetChannel().AddCommand(NO_ORNAMENT);
    }

    virtual void SetGliss(uint_t absStep)
    {
      Builder.GetChannel().AddCommand(GLISS, absStep);
    }

    virtual void SetSlide(int_t delta)
    {
      Builder.GetChannel().AddCommand(SLIDE, delta);
    }

    virtual void SetVolumeSlide(uint_t period, int_t delta)
    {
      Builder.GetChannel().AddCommand(VOLUME_SLIDE, period, delta);
    }
  private:
    const ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    PatternsBuilder Builder;
  };

  struct ChannelState
  {
    ChannelState()
      : Note()
      , ToneAccumulator()
      , Volume()
      , Attenuation()
      , NoiseAccumulator()
      , EnvelopeEnabled()
    {
    }

    ObjectLinesIterator<Sample> SampleIterator;
    ObjectLinesIterator<Ornament> OrnamentIterator;
    ToneSlider ToneSlide;
    VolumeSlider VolumeSlide;
    int_t Note;
    int16_t ToneAccumulator;
    uint_t Volume;
    int_t Attenuation;
    uint_t NoiseAccumulator;
    bool EnvelopeEnabled;
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
       : Data(data)
       , EnvelopeTone()
       , NoiseBase()
    {
      Reset();
    }

    virtual void Reset()
    {
      const Sample* const stubSample = Data->Samples.Find(0);
      const Ornament* const stubOrnament = Data->Ornaments.Find(0);
      for (uint_t chan = 0; chan != Devices::AYM::CHANNELS; ++chan)
      {
        ChannelState& dst = PlayerState[chan];
        dst = ChannelState();
        dst.SampleIterator.Set(stubSample);
        dst.SampleIterator.SetBreakLoop(false);
        dst.SampleIterator.Disable();
        dst.OrnamentIterator.Set(stubOrnament);
        dst.OrnamentIterator.SetBreakLoop(false);
        dst.OrnamentIterator.Disable();
      }
      EnvelopeTone = 0;
      NoiseBase = 0;
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
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != Devices::AYM::CHANNELS; ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            GetNewChannelState(*src, PlayerState[chan], track);
          }
        }
      }
      for (uint_t chan = 0; chan != Devices::AYM::CHANNELS; ++chan)
      {
        PlayerState[chan].NoiseAccumulator += NoiseBase;
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      const uint16_t oldTone = static_cast<uint_t>(track.GetFrequency(dst.Note) + dst.ToneAccumulator + dst.ToneSlide.GetSliding());
      if (const bool* enabled = src.GetEnabled())
      {
        if (*enabled)
        {
          dst.SampleIterator.Reset();
          dst.OrnamentIterator.Reset();
          dst.VolumeSlide.Reset();
          dst.ToneSlide.SetSliding(0);
          dst.ToneAccumulator = 0;
          dst.NoiseAccumulator = 0;
          dst.Attenuation = 0;
        }
        else
        {
          dst.SampleIterator.SetBreakLoop(false);
          dst.SampleIterator.Disable();
          dst.OrnamentIterator.SetBreakLoop(false);
          dst.OrnamentIterator.Disable();
        }
        dst.ToneSlide.Reset();
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleIterator.Set(Data->Samples.Find(*sample));
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentIterator.Set(Data->Ornaments.Find(*ornament));
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Volume = *volume;
        dst.Attenuation = 0;
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case BREAK_SAMPLE:
          dst.SampleIterator.SetBreakLoop(true);
          break;
        case BREAK_ORNAMENT:
          dst.OrnamentIterator.SetBreakLoop(true);
          break;
        case NO_ORNAMENT:
          dst.OrnamentIterator.Disable();
          break;
        case ENVELOPE:
          if (it->Param1 || it->Param2)
          {
            track.SetEnvelopeType(it->Param1);
            track.SetEnvelopeTone(EnvelopeTone = it->Param2);
          }
          else
          {
            dst.EnvelopeEnabled = true;
          }
          break;
        case NOENVELOPE:
          dst.EnvelopeEnabled = false;
          break;
        case NOISE_BASE:
          NoiseBase = it->Param1;
          break;
        case GLISS:
          {
            const int_t sliding = oldTone - track.GetFrequency(dst.Note);
            const int_t newGliss = sliding >= 0 ? -it->Param1 : it->Param1;
            const int_t steps = 0 != it->Param1 ? (1 + Math::Absolute(sliding) / it->Param1) : 0;
            dst.ToneSlide.SetSliding(sliding);
            dst.ToneSlide.SetGlissade(newGliss);
            dst.ToneSlide.SetSlidingSteps(steps);
          }
          break;
        case SLIDE:
          dst.ToneSlide.SetGlissade(it->Param1);
          dst.ToneSlide.SetSlidingSteps(0);
          break;
        case VOLUME_SLIDE:
          dst.VolumeSlide.SetParams(it->Param1, it->Param2);
          break;
        default:
          assert(!"Invalid command");
        }
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
      const Sample::Line* const curSampleLine = dst.SampleIterator.GetLine();
      if (!curSampleLine)
      {
        channel.SetLevel(0);
        return;
      }
      dst.SampleIterator.Next();

      //apply tone
      if (const Ornament::Line* curOrnamentLine = dst.OrnamentIterator.GetLine())
      {
        dst.NoiseAccumulator += curOrnamentLine->NoiseAddon;
        dst.Note += curOrnamentLine->NoteAddon;
        if (dst.Note < 0)
        {
          dst.Note += 0x56;
        }
        else if (dst.Note > 0x55)
        {
          dst.Note -= 0x56;
        }
        if (dst.Note > 0x55)
        {
          dst.Note = 0x55;
        }
        dst.OrnamentIterator.Next();
      }
      dst.ToneAccumulator += curSampleLine->Tone;
      const int_t toneOffset = dst.ToneAccumulator + dst.ToneSlide.Update();
      channel.SetTone(dst.Note, toneOffset);
      if (curSampleLine->ToneMask)
      {
        channel.DisableTone();
      }

      //apply level
      dst.Attenuation += curSampleLine->VolumeDelta + dst.VolumeSlide.Update();
      const int_t level = Math::Clamp<int_t>(dst.Attenuation + dst.Volume, 0, 15);
      dst.Attenuation = level - dst.Volume;
      const uint_t vol = 1 + static_cast<uint_t>(level);
      channel.SetLevel(vol * curSampleLine->Level >> 4);

      //apply noise+envelope
      const bool envelope = dst.EnvelopeEnabled && curSampleLine->EnableEnvelope;
      if (envelope)
      {
        channel.EnableEnvelope();
      }
      if (envelope && curSampleLine->NoiseMask)
      {
        EnvelopeTone += curSampleLine->Adding;
        track.SetEnvelopeTone(EnvelopeTone);
      }
      else
      {
        dst.NoiseAccumulator += curSampleLine->Adding;
        if (!curSampleLine->NoiseMask)
        {
          track.SetNoise(dst.NoiseAccumulator);
        }
      }
      if (curSampleLine->NoiseMask)
      {
        channel.DisableNoise();
      }
    }
  private:
    const ModuleData::Ptr Data;
    boost::array<ChannelState, Devices::AYM::CHANNELS> PlayerState;
    uint_t EnvelopeTone;
    uint_t NoiseBase;
  };

  class Chiptune : public AYM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, Devices::AYM::CHANNELS))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual ModuleProperties::Ptr GetProperties() const
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
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };
}

namespace ProSoundCreator
{
  std::auto_ptr<Formats::Chiptune::ProSoundCreator::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
  {
    return std::auto_ptr<Formats::Chiptune::ProSoundCreator::Builder>(new DataBuilder(data, props));
  }

  AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
  {
    return boost::make_shared<Chiptune>(data, properties);
  }
}

namespace PSC
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'P', 'S', 'C', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | SupportedAYMFormatConvertors;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::Decoder::Ptr decoder)
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

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr rawData, std::size_t& usedSize) const
    {
      const ::ProSoundCreator::ModuleData::RWPtr modData = boost::make_shared< ::ProSoundCreator::ModuleData>();
      const std::auto_ptr<Formats::Chiptune::ProSoundCreator::Builder> dataBuilder = ::ProSoundCreator::CreateDataBuilder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ProSoundCreator::Parse(*rawData, *dataBuilder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const AYM::Chiptune::Ptr chiptune = ::ProSoundCreator::CreateChiptune(modData, properties);
        return AYM::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterPSCSupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProSoundCreatorDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<PSC::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(PSC::ID, decoder->GetDescription(), PSC::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
