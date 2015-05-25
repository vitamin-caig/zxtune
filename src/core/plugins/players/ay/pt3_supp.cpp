/**
* 
* @file
*
* @brief  ProTracker v3.x support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_base.h"
#include "ts_base.h"
#include "vortex_base.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/plugin.h"
#include "core/plugins/players/simple_orderlist.h"
//common includes
#include <pointers.h>
//library includes
#include <core/plugin_attrs.h>
#include <core/conversion/aym.h>
//text includes
#include <core/text/plugins.h>

namespace Module
{
namespace ProTracker3
{
  typedef Vortex::ModuleData ModuleData;

  class DataBuilder : public Formats::Chiptune::ProTracker3::Builder
  {
  public:
    explicit DataBuilder(PropertiesBuilder& props)
      : Data(boost::make_shared<ModuleData>())
      , Properties(props)
      , PatOffset(Formats::Chiptune::ProTracker3::SINGLE_AY_MODE)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
    {
      Data->Patterns = Patterns.GetResult();
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return Properties;
    }

    virtual void SetVersion(uint_t version)
    {
      Properties.SetVersion(3, Data->Version = version);
    }

    virtual void SetNoteTable(Formats::Chiptune::ProTracker3::NoteTable table)
    {
      const String freqTable = Vortex::GetFreqTable(static_cast<Vortex::NoteTable>(table), Data->Version);
      Properties.SetFreqtable(freqTable);
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
      Data->Samples.Add(index, Vortex::Sample(sample.Loop, sample.Lines.begin(), sample.Lines.end()));
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::ProTracker3::Ornament& ornament)
    {
      Data->Ornaments.Add(index, Vortex::Ornament(ornament.Loop, ornament.Lines.begin(), ornament.Lines.end()));
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      Data->Order = boost::make_shared<SimpleOrderList>(loop, positions.begin(), positions.end());
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
      MutableCell& channel = Patterns.GetChannel();
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
      Patterns.GetChannel().SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Patterns.GetChannel().SetOrnament(ornament);
    }

    virtual void SetVolume(uint_t vol)
    {
      Patterns.GetChannel().SetVolume(vol);
    }

    virtual void SetGlissade(uint_t period, int_t val)
    {
      Patterns.GetChannel().AddCommand(Vortex::GLISS, period, val);
    }

    virtual void SetNoteGliss(uint_t period, int_t val, uint_t /*limit*/)
    {
      //ignore limit
      Patterns.GetChannel().AddCommand(Vortex::GLISS_NOTE, period, val);
    }

    virtual void SetSampleOffset(uint_t offset)
    {
      Patterns.GetChannel().AddCommand(Vortex::SAMPLEOFFSET, offset);
    }

    virtual void SetOrnamentOffset(uint_t offset)
    {
      Patterns.GetChannel().AddCommand(Vortex::ORNAMENTOFFSET, offset);
    }

    virtual void SetVibrate(uint_t ontime, uint_t offtime)
    {
      Patterns.GetChannel().AddCommand(Vortex::VIBRATE, ontime, offtime);
    }

    virtual void SetEnvelopeSlide(uint_t period, int_t val)
    {
      Patterns.GetChannel().AddCommand(Vortex::SLIDEENV, period, val);
    }

    virtual void SetEnvelope(uint_t type, uint_t value)
    {
      Patterns.GetChannel().AddCommand(Vortex::ENVELOPE, type, value);
    }

    virtual void SetNoEnvelope()
    {
      Patterns.GetChannel().AddCommand(Vortex::NOENVELOPE);
    }

    virtual void SetNoiseBase(uint_t val)
    {
      Patterns.GetChannel().AddCommand(Vortex::NOISEBASE, val);
    }

    uint_t GetPatOffset() const
    {
      return PatOffset;
    }

    ModuleData::RWPtr GetResult() const
    {
      return Data;
    }
  private:
    const ModuleData::RWPtr Data;
    PropertiesBuilder& Properties;
    uint_t PatOffset;
    PatternsBuilder Patterns;
  };

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
      return MakeSingletonPointer(instance);
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
      return idx < AYM::TRACK_CHANNELS
        ? First->GetChannel(idx)
        : Second->GetChannel(idx - AYM::TRACK_CHANNELS);
    }

    virtual uint_t CountActiveChannels() const
    {
      return First->CountActiveChannels() + Second->CountActiveChannels();
    }

    virtual uint_t GetTempo() const
    {
      if (const uint_t tempo = Second->GetTempo())
      {
        return tempo;
      }
      return First->GetTempo();
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

  class PT3Chiptune : public AYM::Chiptune
  {
  public:
    PT3Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
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
      const AYM::DataRenderer::Ptr renderer = CreateDataRenderer(Data, 0);
      return AYM::CreateDataIterator(trackParams, iterator, renderer);
    }
  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
    const Information::Ptr Info;
  };


  class TSChiptune : public TurboSound::Chiptune
  {
  public:
    TSChiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(data, TurboSound::TRACK_CHANNELS))
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

    virtual TurboSound::DataIterator::Ptr CreateDataIterator(const TurboSound::TrackParametersArray& trackParams) const
    {
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const TurboSound::DataRenderersArray renderers = {{Vortex::CreateDataRenderer(Data, 0), Vortex::CreateDataRenderer(Data, AYM::TRACK_CHANNELS)}};
      return TurboSound::CreateDataIterator(trackParams, iterator, renderers);
    }
  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
    const Information::Ptr Info;
  };

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::ProTracker3::Decoder::Ptr decoder)
      : Decoder(decoder)
    {
    }

    virtual Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      DataBuilder dataBuilder(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Decoder->Parse(rawData, dataBuilder))
      {
        propBuilder.SetSource(*container);
        const uint_t patOffset = dataBuilder.GetPatOffset();
        const ModuleData::RWPtr modData = dataBuilder.GetResult();
        if (patOffset != Formats::Chiptune::ProTracker3::SINGLE_AY_MODE)
        {
          //TurboSound modules
          propBuilder.SetComment(Text::PT3_TURBOSOUND_MODULE);
          modData->Patterns = CreateTSPatterns(patOffset, modData->Patterns);
          const TurboSound::Chiptune::Ptr chiptune = boost::make_shared<TSChiptune>(modData, propBuilder.GetResult());
          return TurboSound::CreateHolder(chiptune);
        }
        else
        {
          const AYM::Chiptune::Ptr chiptune = boost::make_shared<PT3Chiptune>(modData, propBuilder.GetResult());
          return AYM::CreateHolder(chiptune);
        }
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::ProTracker3::Decoder::Ptr Decoder;
  };
}
}

namespace ZXTune
{
  void RegisterPT3Support(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'P', 'T', '3', 0};
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::AY38910 | Capabilities::Module::Device::TURBOSOUND
      | Module::AYM::GetSupportedFormatConvertors() | Module::Vortex::GetSupportedFormatConvertors();

    const Formats::Chiptune::ProTracker3::Decoder::Ptr decoder = Formats::Chiptune::ProTracker3::CreateDecoder();
    const Module::Factory::Ptr factory = boost::make_shared<Module::ProTracker3::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }

  void RegisterTXTSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'T', 'X', 'T', 0};
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::AY38910
      | Module::AYM::GetSupportedFormatConvertors() | Module::Vortex::GetSupportedFormatConvertors();

    const Formats::Chiptune::ProTracker3::Decoder::Ptr decoder = Formats::Chiptune::ProTracker3::VortexTracker2::CreateDecoder();
    const Module::Factory::Ptr factory = boost::make_shared<Module::ProTracker3::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
