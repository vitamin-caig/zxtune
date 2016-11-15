/**
* 
* @file
*
* @brief  ProTracker v3.x chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "protracker3.h"
#include "aym_base.h"
#include "aym_base_track.h"
#include "aym_properties_helper.h"
#include "turbosound.h"
#include "vortex.h"
//common includes
#include <pointers.h>
//library includes
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>
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
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , PatOffset(Formats::Chiptune::ProTracker3::SINGLE_AY_MODE)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
      , Data(MakeRWPtr<ModuleData>())
    {
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetVersion(uint_t version) override
    {
      Properties.SetVersion(3, Data->Version = version);
    }

    void SetNoteTable(Formats::Chiptune::ProTracker3::NoteTable table) override
    {
      const String freqTable = Vortex::GetFreqTable(static_cast<Vortex::NoteTable>(table), Data->Version);
      Properties.SetFrequencyTable(freqTable);
    }

    void SetMode(uint_t mode) override
    {
      PatOffset = mode;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, Formats::Chiptune::ProTracker3::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::ProTracker3::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(std::vector<uint_t> positions, uint_t loop) override
    {
      Data->Order = MakePtr<SimpleOrderList>(loop, std::move(positions));
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

    void SetSample(uint_t sample) override
    {
      Patterns.GetChannel().SetSample(sample);
    }

    void SetOrnament(uint_t ornament) override
    {
      Patterns.GetChannel().SetOrnament(ornament);
    }

    void SetVolume(uint_t vol) override
    {
      Patterns.GetChannel().SetVolume(vol);
    }

    void SetGlissade(uint_t period, int_t val) override
    {
      Patterns.GetChannel().AddCommand(Vortex::GLISS, period, val);
    }

    void SetNoteGliss(uint_t period, int_t val, uint_t /*limit*/) override
    {
      //ignore limit
      Patterns.GetChannel().AddCommand(Vortex::GLISS_NOTE, period, val);
    }

    void SetSampleOffset(uint_t offset) override
    {
      Patterns.GetChannel().AddCommand(Vortex::SAMPLEOFFSET, offset);
    }

    void SetOrnamentOffset(uint_t offset) override
    {
      Patterns.GetChannel().AddCommand(Vortex::ORNAMENTOFFSET, offset);
    }

    void SetVibrate(uint_t ontime, uint_t offtime) override
    {
      Patterns.GetChannel().AddCommand(Vortex::VIBRATE, ontime, offtime);
    }

    void SetEnvelopeSlide(uint_t period, int_t val) override
    {
      Patterns.GetChannel().AddCommand(Vortex::SLIDEENV, period, val);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Patterns.GetChannel().AddCommand(Vortex::ENVELOPE, type, value);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().AddCommand(Vortex::NOENVELOPE);
    }

    void SetNoiseBase(uint_t val) override
    {
      Patterns.GetChannel().AddCommand(Vortex::NOISEBASE, val);
    }

    uint_t GetPatOffset() const
    {
      return PatOffset;
    }

    ModuleData::RWPtr CaptureResult()
    {
      Data->Patterns = Patterns.CaptureResult();
      return std::move(Data);
    }
  private:
    AYM::PropertiesHelper& Properties;
    MetaProperties Meta;
    uint_t PatOffset;
    PatternsBuilder Patterns;
    ModuleData::RWPtr Data;
  };

  class StubLine : public Line
  {
    StubLine()
    {
    }
  public:
    const Cell* GetChannel(uint_t /*idx*/) const override
    {
      return nullptr;
    }

    uint_t CountActiveChannels() const override
    {
      return 0;
    }

    uint_t GetTempo() const override
    {
      return 0;
    }

    static const Line* Create()
    {
      static const StubLine instance;
      return &instance;
    }
  };

  class TSLine : public Line
  {
  public:
    typedef std::unique_ptr<TSLine> Ptr;
    
    TSLine(const Line* first, const Line* second)
      : First(first ? first : StubLine::Create())
      , Second(second ? second : StubLine::Create())
    {
    }

    const Cell* GetChannel(uint_t idx) const override
    {
      return idx < AYM::TRACK_CHANNELS
        ? First->GetChannel(idx)
        : Second->GetChannel(idx - AYM::TRACK_CHANNELS);
    }

    uint_t CountActiveChannels() const override
    {
      return First->CountActiveChannels() + Second->CountActiveChannels();
    }

    uint_t GetTempo() const override
    {
      if (const uint_t tempo = Second->GetTempo())
      {
        return tempo;
      }
      return First->GetTempo();
    }
  private:
    const Line* const First;
    const Line* const Second;
  };

  class TSPattern : public Pattern
  {
  public:
    typedef std::unique_ptr<TSPattern> Ptr;

    TSPattern(const Pattern& first, const Pattern& second)
      : First(first)
      , Second(second)
    {
    }

    const Line* GetLine(uint_t row) const override
    {
      if (const auto cached = Lines.Get(row).get())
      {
        return cached;
      }
      else
      {
        const auto first = First.GetLine(row);
        const auto second = Second.GetLine(row);
        return Lines.Add(row, MakePtr<TSLine>(first, second)).get();
      }
    }

    uint_t GetSize() const override
    {
      return std::min(First.GetSize(), Second.GetSize());
    }
  private:
    const Pattern& First;
    const Pattern& Second;
    mutable SparsedObjectsStorage<TSLine::Ptr> Lines;
  };

  class TSPatternsSet : public PatternsSet
  {
  public:
    TSPatternsSet(uint_t base, PatternsSet::Ptr delegate)
      : Base(base)
      , Delegate(std::move(delegate))
    {
    }

    const Pattern* Get(uint_t idx) const override
    {
      if (const auto cached = Patterns.Get(idx).get())
      {
        return cached;
      }
      else
      {
        const auto first = Delegate->Get(idx);
        const auto second = Delegate->Get(Base - 1 - idx);
        return Patterns.Add(idx, MakePtr<TSPattern>(*first, *second)).get();
      }
    }

    uint_t GetSize() const override
    {
      return Delegate->GetSize();
    }
  private:
    const uint_t Base;
    const PatternsSet::Ptr Delegate;
    mutable SparsedObjectsStorage<TSPattern::Ptr> Patterns;
  };

  PatternsSet::Ptr CreateTSPatterns(uint_t patOffset, PatternsSet::Ptr pats)
  {
    return MakePtr<TSPatternsSet>(patOffset, std::move(pats));
  }

  class PT3Chiptune : public AYM::Chiptune
  {
  public:
    PT3Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
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
      : Data(std::move(data))
      , Properties(std::move(properties))
      , Info(CreateTrackInfo(Data, TurboSound::TRACK_CHANNELS))
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

    TurboSound::DataIterator::Ptr CreateDataIterator(const TurboSound::TrackParametersArray& trackParams) const override
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
      : Decoder(std::move(decoder))
    {
    }

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const Formats::Chiptune::Container::Ptr container = Decoder->Parse(rawData, dataBuilder))
      {
        props.SetSource(*container);
        const uint_t patOffset = dataBuilder.GetPatOffset();
        auto modData = dataBuilder.CaptureResult();
        if (patOffset != Formats::Chiptune::ProTracker3::SINGLE_AY_MODE)
        {
          //TurboSound modules
          props.SetComment(Text::PT3_TURBOSOUND_MODULE);
          modData->Patterns = CreateTSPatterns(patOffset, std::move(modData->Patterns));
          const TurboSound::Chiptune::Ptr chiptune = MakePtr<TSChiptune>(std::move(modData), properties);
          return TurboSound::CreateHolder(chiptune);
        }
        else
        {
          const AYM::Chiptune::Ptr chiptune = MakePtr<PT3Chiptune>(std::move(modData), properties);
          return AYM::CreateHolder(chiptune);
        }
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::ProTracker3::Decoder::Ptr Decoder;
  };

  Factory::Ptr CreateFactory(Formats::Chiptune::ProTracker3::Decoder::Ptr decoder)
  {
    return MakePtr<Factory>(decoder);
  }
}
}
