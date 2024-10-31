/**
 *
 * @file
 *
 * @brief  ProTracker v3.x chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/protracker3.h"

#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/aym/turbosound.h"
#include "module/players/aym/vortex.h"
#include "module/players/platforms.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"

#include "parameters/tracking_helper.h"

#include "pointers.h"

namespace Module::ProTracker3
{
  using ModuleData = Vortex::ModuleData;

  const auto TURBOSOUND_COMMENT = "TurboSound module"sv;

  class DataBuilder : public Formats::Chiptune::ProTracker3::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , PatOffset(Formats::Chiptune::ProTracker3::SINGLE_AY_MODE)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
      , Data(MakeRWPtr<ModuleData>())
    {}

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
      const auto freqTable = Vortex::GetFreqTable(static_cast<Vortex::NoteTable>(table), Data->Version);
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

    void SetPositions(Formats::Chiptune::ProTracker3::Positions positions) override
    {
      Data->Order = MakePtr<SimpleOrderList>(positions.Loop, std::move(positions.Lines));
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
      // ignore limit
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
    StubLine() = default;

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

  class Chiptune : public AYM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(std::move(data))
      , Properties(std::move(properties))
    {}

    Time::Microseconds GetFrameDuration() const override
    {
      return AYM::BASE_FRAME_DURATION;
    }

    TrackModel::Ptr FindTrackModel() const override
    {
      return Data;
    }

    Module::StreamModel::Ptr FindStreamModel() const override
    {
      return {};
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const override
    {
      auto iterator = CreateTrackStateIterator(GetFrameDuration(), Data);
      auto renderer = CreateDataRenderer(Data, 0);
      return AYM::CreateDataIterator(std::move(trackParams), std::move(iterator), std::move(renderer));
    }

  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
  };

  namespace TS
  {
    class Line : public Module::Line
    {
    public:
      using Ptr = std::unique_ptr<Line>;

      Line(const Module::Line* first, const Module::Line* second)
        : First(first ? first : StubLine::Create())
        , Second(second ? second : StubLine::Create())
      {}

      const Cell* GetChannel(uint_t idx) const override
      {
        return idx < AYM::TRACK_CHANNELS ? First->GetChannel(idx) : Second->GetChannel(idx - AYM::TRACK_CHANNELS);
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
      const Module::Line* const First;
      const Module::Line* const Second;
    };

    class Pattern : public Module::Pattern
    {
    public:
      using Ptr = std::unique_ptr<Pattern>;

      Pattern(const Module::Pattern& first, const Module::Pattern& second)
        : First(first)
        , Second(second)
      {}

      const Line* GetLine(uint_t row) const override
      {
        if (auto* const cached = Lines.Get(row).get())
        {
          return cached;
        }
        else
        {
          const auto* const first = First.GetLine(row);
          const auto* const second = Second.GetLine(row);
          return Lines.Add(row, MakePtr<Line>(first, second)).get();
        }
      }

      uint_t GetSize() const override
      {
        return std::min(First.GetSize(), Second.GetSize());
      }

    private:
      const Module::Pattern& First;
      const Module::Pattern& Second;
      mutable SparsedObjectsStorage<Line::Ptr> Lines;
    };

    class PatternsSet : public Module::PatternsSet
    {
    public:
      PatternsSet(uint_t base, Module::PatternsSet::Ptr delegate)
        : Base(base)
        , Delegate(std::move(delegate))
      {}

      const Pattern* Get(uint_t idx) const override
      {
        if (auto* const cached = Patterns.Get(idx).get())
        {
          return cached;
        }
        else
        {
          const auto* const first = Delegate->Get(idx);
          const auto* const second = Delegate->Get(Base - 1 - idx);
          return Patterns.Add(idx, MakePtr<Pattern>(*first, *second)).get();
        }
      }

      uint_t GetSize() const override
      {
        return Delegate->GetSize();
      }

    private:
      const uint_t Base;
      const PatternsSet::Ptr Delegate;
      mutable SparsedObjectsStorage<Pattern::Ptr> Patterns;
    };

    PatternsSet::Ptr CreatePatterns(uint_t patOffset, PatternsSet::Ptr pats)
    {
      return MakePtr<PatternsSet>(patOffset, std::move(pats));
    }

    class DataIterator : public TurboSound::DataIterator
    {
    public:
      DataIterator(AYM::TrackParameters::Ptr trackParams, TrackStateIterator::Ptr iterator,
                   AYM::DataRenderer::Ptr first, AYM::DataRenderer::Ptr second)
        : Params(std::move(trackParams))
        , Delegate(std::move(iterator))
        , State(Delegate->GetStateObserver())
        , First(std::move(first))
        , Second(std::move(second))
      {}

      void Reset() override
      {
        Params.Reset();
        Delegate->Reset();
        First->Reset();
        Second->Reset();
      }

      void NextFrame() override
      {
        Delegate->NextFrame();
      }

      Module::State::Ptr GetStateObserver() const override
      {
        return State;
      }

      Devices::TurboSound::Registers GetData() const override
      {
        SynchronizeParameters();
        return {{RenderFrom(*First), RenderFrom(*Second)}};
      }

    private:
      Devices::AYM::Registers RenderFrom(AYM::DataRenderer& renderer) const
      {
        AYM::TrackBuilder builder(Table);
        renderer.SynthesizeData(*State, builder);
        return builder.GetResult();
      }

      void SynchronizeParameters() const
      {
        if (Params.IsChanged())
        {
          Params->FreqTable(Table);
        }
      }

    private:
      Parameters::TrackingHelper<AYM::TrackParameters> Params;
      const TrackStateIterator::Ptr Delegate;
      const TrackModelState::Ptr State;
      const AYM::DataRenderer::Ptr First;
      const AYM::DataRenderer::Ptr Second;
      mutable FrequencyTable Table;
    };

    class Chiptune : public TurboSound::Chiptune
    {
    public:
      Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
        : Data(std::move(data))
        , Properties(std::move(properties))
      {}

      Time::Microseconds GetFrameDuration() const override
      {
        return AYM::BASE_FRAME_DURATION;
      }

      TrackModel::Ptr FindTrackModel() const override
      {
        return Data;
      }

      Module::StreamModel::Ptr FindStreamModel() const override
      {
        return {};
      }

      Parameters::Accessor::Ptr GetProperties() const override
      {
        return Properties;
      }

      TurboSound::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr first,
                                                       AYM::TrackParameters::Ptr /*second*/) const override
      {
        auto iterator = CreateTrackStateIterator(GetFrameDuration(), Data);
        return MakePtr<DataIterator>(std::move(first), std::move(iterator), Vortex::CreateDataRenderer(Data, 0),
                                     Vortex::CreateDataRenderer(Data, AYM::TRACK_CHANNELS));
      }

    private:
      const ModuleData::Ptr Data;
      const Parameters::Accessor::Ptr Properties;
    };
  }  // namespace TS

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::ProTracker3::Decoder::Ptr decoder)
      : Decoder(std::move(decoder))
    {}

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData,
                             Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Decoder->Parse(rawData, dataBuilder))
      {
        props.SetSource(*container);
        props.SetPlatform(Platforms::ZX_SPECTRUM);
        const uint_t patOffset = dataBuilder.GetPatOffset();
        auto modData = dataBuilder.CaptureResult();
        if (patOffset != Formats::Chiptune::ProTracker3::SINGLE_AY_MODE)
        {
          // TurboSound modules
          props.SetComment(TURBOSOUND_COMMENT);
          modData->Patterns = TS::CreatePatterns(patOffset, std::move(modData->Patterns));
          auto chiptune = MakePtr<TS::Chiptune>(std::move(modData), std::move(properties));
          return TurboSound::CreateHolder(std::move(chiptune));
        }
        else
        {
          auto chiptune = MakePtr<Chiptune>(std::move(modData), std::move(properties));
          return AYM::CreateHolder(std::move(chiptune));
        }
      }
      return {};
    }

  private:
    const Formats::Chiptune::ProTracker3::Decoder::Ptr Decoder;
  };

  Factory::Ptr CreateFactory(Formats::Chiptune::ProTracker3::Decoder::Ptr decoder)
  {
    return MakePtr<Factory>(std::move(decoder));
  }
}  // namespace Module::ProTracker3
