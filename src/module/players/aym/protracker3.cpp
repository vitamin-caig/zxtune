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
      Data->TurboPatternsOffset = mode;
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

    ModuleData::RWPtr CaptureResult()
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
      auto renderer = CreateDataRenderer(Data);
      return AYM::CreateDataIterator(std::move(trackParams), std::move(iterator), std::move(renderer));
    }

  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
  };

  namespace TS
  {
    class DataIterator : public TurboSound::DataIterator
    {
    public:
      DataIterator(uint_t base, AYM::TrackParameters::Ptr trackParams, Iterator::Ptr iterator,
                   AYM::DataRenderer::Ptr first, AYM::DataRenderer::Ptr second)
        : Base(base)
        , Params(std::move(trackParams))
        , Delegate(std::move(iterator))
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

      Module::State GetState() const override
      {
        return Delegate->GetState();
      }

      Devices::TurboSound::Registers GetData() const override
      {
        SynchronizeParameters();
        auto state = Delegate->GetState();
        auto firstReg = RenderFrom(*state.Track, *First);
        state.Track->Pattern = Base - 1 - state.Track->Pattern;
        return {{std::move(firstReg), RenderFrom(*state.Track, *Second)}};
      }

    private:
      Devices::AYM::Registers RenderFrom(const TrackState& state, AYM::DataRenderer& renderer) const
      {
        AYM::TrackBuilder builder(Table);
        renderer.SynthesizeData(state, builder);
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
      const uint_t Base;
      Parameters::TrackingHelper<AYM::TrackParameters> Params;
      const Iterator::Ptr Delegate;
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
        return MakePtr<DataIterator>(*Data->TurboPatternsOffset, std::move(first), std::move(iterator),
                                     Vortex::CreateDataRenderer(Data), Vortex::CreateDataRenderer(Data));
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
        auto modData = dataBuilder.CaptureResult();
        if (modData->TurboPatternsOffset)
        {
          // TurboSound modules
          props.SetComment(TURBOSOUND_COMMENT);
          props.SetChannels(TurboSound::MakeChannelsNames());
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
