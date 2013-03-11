/*
Abstract:
  PT3 modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "ts_base.h"
#include "vortex_base.h"
#include "core/plugins/utils.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
//common includes
#include <contract.h>
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
#include <formats/chiptune/protracker3.h>
#include <sound/mixer_factory.h>
//text includes
#include <core/text/plugins.h>

#define FILE_TAG 3CBC0BBC

namespace ProTracker3
{
  typedef ZXTune::Module::Vortex::Track Track;

  std::auto_ptr<Formats::Chiptune::ProTracker3::Builder> CreateDataBuilder(Track::ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props, uint_t& patOffset);
}

namespace ProTracker3
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  void ConvertSample(const Formats::Chiptune::ProTracker3::Sample& src, Track::Sample& dst)
  {
    dst = Track::Sample(src.Loop, src.Lines.begin(), src.Lines.end());
  }

  void ConvertOrnament(const Formats::Chiptune::ProTracker3::Ornament& src, Track::Ornament& dst)
  {
    dst = Track::Ornament(src.Loop, src.Lines.begin(), src.Lines.end());
  }

  class DataBuilder : public Formats::Chiptune::ProTracker3::Builder
  {
  public:
    DataBuilder(Track::ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props, uint_t& patOffset)
      : Data(data)
      , Properties(props)
      , PatOffset(patOffset)
      , Builder(PatternsBuilder::Create<Track::CHANNELS>())
    {
      Data->Patterns = Builder.GetPatterns();
    }

    virtual void SetProgram(const String& program)
    {
      Properties->SetProgram(OptimizeString(program));
    }

    virtual void SetTitle(const String& title)
    {
      Properties->SetTitle(OptimizeString(title));
    }

    virtual void SetAuthor(const String& author)
    {
      Properties->SetAuthor(OptimizeString(author));
    }

    virtual void SetVersion(uint_t version)
    {
      Properties->SetVersion(3, version);
    }

    virtual void SetNoteTable(Formats::Chiptune::ProTracker3::NoteTable table)
    {
      const String freqTable = Vortex::GetFreqTable(static_cast<Vortex::NoteTable>(table), Vortex::ExtractVersion(*Properties));
      Properties->SetFreqtable(freqTable);
    }

    virtual void SetMode(uint_t mode)
    {
      PatOffset = mode;
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::ProTracker3::Sample& sample)
    {
      //TODO: use common types
      Data->Samples.resize(index + 1);
      ConvertSample(sample, Data->Samples[index]);
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::ProTracker3::Ornament& ornament)
    {
      Data->Ornaments.resize(index + 1);
      ConvertOrnament(ornament, Data->Ornaments[index]);
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      Data->Order = boost::make_shared<SimpleOrderList>(positions.begin(), positions.end(), loop);
    }

    virtual void StartPattern(uint_t index)
    {
      Builder.SetPattern(index);
    }

    virtual void FinishPattern(uint_t size)
    {
      Builder.FinishPattern(size);
    }

    virtual void StartLine(uint_t index)
    {
      Builder.SetLine(index);
    }

    virtual void SetTempo(uint_t tempo)
    {
      Builder.GetLine().SetTempo(tempo);
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
      if (Command* cmd = channel.FindCommand(Vortex::GLISS_NOTE))
      {
        cmd->Param3 = int_t(note);
      }
      else
      {
        channel.SetNote(note);
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

    virtual void SetGlissade(uint_t period, int_t val)
    {
      Builder.GetChannel().AddCommand(Vortex::GLISS, period, val);
    }

    virtual void SetNoteGliss(uint_t period, int_t val, uint_t /*limit*/)
    {
      //ignore limit
      Builder.GetChannel().AddCommand(Vortex::GLISS_NOTE, period, val);
    }

    virtual void SetSampleOffset(uint_t offset)
    {
      Builder.GetChannel().AddCommand(Vortex::SAMPLEOFFSET, offset);
    }

    virtual void SetOrnamentOffset(uint_t offset)
    {
      Builder.GetChannel().AddCommand(Vortex::ORNAMENTOFFSET, offset);
    }

    virtual void SetVibrate(uint_t ontime, uint_t offtime)
    {
      Builder.GetChannel().AddCommand(Vortex::VIBRATE, ontime, offtime);
    }

    virtual void SetEnvelopeSlide(uint_t period, int_t val)
    {
      Builder.GetChannel().AddCommand(Vortex::SLIDEENV, period, val);
    }

    virtual void SetEnvelope(uint_t type, uint_t value)
    {
      Builder.GetChannel().AddCommand(Vortex::ENVELOPE, type, value);
    }

    virtual void SetNoEnvelope()
    {
      Builder.GetChannel().AddCommand(Vortex::NOENVELOPE);
    }

    virtual void SetNoiseBase(uint_t val)
    {
      Builder.GetChannel().AddCommand(Vortex::NOISEBASE, val);
    }
  private:
    const Track::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    uint_t& PatOffset;
    PatternsBuilder Builder;
  };
}

namespace ProTracker3
{
  std::auto_ptr<Formats::Chiptune::ProTracker3::Builder> CreateDataBuilder(Track::ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props, uint_t& patOffset)
  {
    return std::auto_ptr<Formats::Chiptune::ProTracker3::Builder>(new DataBuilder(data, props, patOffset));
  }
}

namespace PT3
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class StubLine : public Line
  {
    StubLine()
    {
    }
  public:
    virtual Cell::Ptr GetChannel(uint_t /*idx*/) const
    {
      return Cell::Ptr();
    }

    virtual uint_t CountActiveChannels() const
    {
      return 0;
    }

    virtual uint_t GetTempo() const
    {
      return 0;
    }

    static Ptr Create()
    {
      static StubLine instance;
      return Ptr(&instance, NullDeleter<Line>());
    }
  };

  class TSLine : public Line
  {
  public:
    TSLine(Line::Ptr first, Line::Ptr second)
      : First(first ? first : StubLine::Create())
      , Second(second ? second : StubLine::Create())
    {
    }

    virtual Cell::Ptr GetChannel(uint_t idx) const
    {
      return idx < Devices::AYM::CHANNELS
        ? First->GetChannel(idx)
        : Second->GetChannel(idx - Devices::AYM::CHANNELS);
    }

    virtual uint_t CountActiveChannels() const
    {
      return First->CountActiveChannels() + Second->CountActiveChannels();
    }

    virtual uint_t GetTempo() const
    {
      if (const uint_t tempo = First->GetTempo())
      {
        return tempo;
      }
      return Second->GetTempo();
    }

    static Line::Ptr Create(Line::Ptr first, Line::Ptr second)
    {
      if (first || second)
      {
        return boost::make_shared<TSLine>(first, second);
      }
      else
      {
        return Line::Ptr();
      }
    }
  private:
    const Line::Ptr First;
    const Line::Ptr Second;
  };

  class TSPattern : public Pattern
  {
  public:
    TSPattern(Pattern::Ptr first, Pattern::Ptr second)
      : First(first)
      , Second(second)
    {
    }

    virtual Line::Ptr GetLine(uint_t row) const
    {
      const Line::Ptr first = First->GetLine(row);
      const Line::Ptr second = Second->GetLine(row);
      return TSLine::Create(first, second);
    }

    virtual uint_t GetSize() const
    {
      return std::min(First->GetSize(), Second->GetSize());
    }
  private:
    const Pattern::Ptr First;
    const Pattern::Ptr Second;
  };

  class TSPatternsSet : public PatternsSet
  {
  public:
    TSPatternsSet(uint_t base, PatternsSet::Ptr delegate)
      : Base(base)
      , Delegate(delegate)
    {
    }

    virtual Pattern::Ptr Get(uint_t idx) const
    {
      const Pattern::Ptr first = Delegate->Get(idx);
      const Pattern::Ptr second = Delegate->Get(Base - 1 - idx);
      return boost::make_shared<TSPattern>(first, second);
    }

    virtual uint_t GetSize() const
    {
      return Delegate->GetSize();
    }
  private:
    const uint_t Base;
    const PatternsSet::Ptr Delegate;
  };

  PatternsSet::Ptr CreateTSPatterns(uint_t patOffset, PatternsSet::Ptr pats)
  {
    return boost::make_shared<TSPatternsSet>(patOffset, pats);
  }

  class TSHolder : public Holder
  {
  public:
    TSHolder(Vortex::Track::ModuleData::Ptr data, ModuleProperties::Ptr properties)
      : Properties(properties)
      , Data(data)
      , Info(CreateTrackInfo(data, 2 * Vortex::Track::CHANNELS, Vortex::Track::CHANNELS))
    {
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Properties;
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      const Sound::ThreeChannelsMixer::Ptr mixer = Sound::CreateThreeChannelsMixer(params);
      mixer->SetTarget(target);
      const Devices::AYM::Receiver::Ptr receiver = AYM::CreateReceiver(mixer);
      const boost::array<Devices::AYM::Receiver::Ptr, 2> tsMixer = CreateTSAYMixer(receiver);
      const Devices::AYM::ChipParameters::Ptr chipParams = AYM::CreateChipParameters(params);
      const Devices::AYM::Chip::Ptr chip1 = Devices::AYM::CreateChip(chipParams, tsMixer[0]);
      const Devices::AYM::Chip::Ptr chip2 = Devices::AYM::CreateChip(chipParams, tsMixer[1]);

      const uint_t version = Vortex::ExtractVersion(*Properties);
      const Renderer::Ptr renderer1 = Vortex::CreateRenderer(params, Data, version, chip1, 0);
      const Renderer::Ptr renderer2 = Vortex::CreateRenderer(params, Data, version, chip2, Vortex::Track::CHANNELS);
      return CreateTSRenderer(renderer1, renderer2, renderer1->GetTrackState());
    }
  private:
    const ModuleProperties::Ptr Properties;
    const Vortex::Track::ModuleData::Ptr Data;
    const Information::Ptr Info;
  };
}

namespace PT3
{
  //plugin attributes
  const Char ID[] = {'P', 'T', '3', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | SupportedAYMFormatConvertors | SupportedVortexFormatConvertors;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::Decoder::Ptr decoder)
      : Decoder(decoder)
    {
    }

    virtual bool Check(const Binary::Container& inputData) const
    {
      return Decoder->Check(inputData);
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr rawData, std::size_t& usedSize) const
    {
      const ::ProTracker3::Track::ModuleData::RWPtr modData = ::ProTracker3::Track::ModuleData::Create();
      uint_t patOffset = 0;
      const std::auto_ptr<Formats::Chiptune::ProTracker3::Builder> dataBuilder = ::ProTracker3::CreateDataBuilder(modData, properties, patOffset);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ProTracker3::ParseCompiled(*rawData, *dataBuilder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        if (patOffset != Formats::Chiptune::ProTracker3::SINGLE_AY_MODE)
        {
          //TurboSound modules
          properties->SetComment(Text::PT3_TURBOSOUND_MODULE);
          modData->Patterns = CreateTSPatterns(patOffset, modData->Patterns);
          return boost::make_shared<TSHolder>(modData, properties);
        }
        else
        {
          const AYM::Chiptune::Ptr chiptune = Vortex::CreateChiptune(modData, properties);
          return AYM::CreateHolder(chiptune);
        }
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace Vortex2
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Char ID[] = {'T', 'X', 'T', 0};
  const uint_t CAPS = PT3::CAPS;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::Decoder::Ptr decoder)
      : Decoder(decoder)
    {
    }

    virtual bool Check(const Binary::Container& inputData) const
    {
      return Decoder->Check(inputData);
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr rawData, std::size_t& usedSize) const
    {
      const ::ProTracker3::Track::ModuleData::RWPtr modData = ::ProTracker3::Track::ModuleData::Create();
      uint_t patOffset = 0;
      const std::auto_ptr<Formats::Chiptune::ProTracker3::Builder> dataBuilder = ::ProTracker3::CreateDataBuilder(modData, properties, patOffset);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ProTracker3::ParseVortexTracker2(*rawData, *dataBuilder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const AYM::Chiptune::Ptr chiptune = Vortex::CreateChiptune(modData, properties);
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
  void RegisterPT3Support(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProTracker3Decoder();
    const ModulesFactory::Ptr factory = boost::make_shared<PT3::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(PT3::ID, decoder->GetDescription(), PT3::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }

  void RegisterTXTSupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateVortexTracker2Decoder();
    const ModulesFactory::Ptr factory = boost::make_shared<Vortex2::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(Vortex2::ID, decoder->GetDescription(), Vortex2::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
