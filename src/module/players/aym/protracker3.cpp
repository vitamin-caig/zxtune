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
    explicit Chiptune(ModuleData::Ptr data)
      : Data(std::move(data))
    {}

    Time::Microseconds GetFrameDuration() const override
    {
      return AYM::BASE_FRAME_DURATION;
    }

    FramedInformation GetInformation() const override
    {
      return CreateTrackInfo(*Data);
    }

    AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const override
    {
      return Vortex::CreateDataIterator(Data, std::move(trackParams));
    }

  private:
    const ModuleData::Ptr Data;
  };

  namespace TS
  {
    class Chiptune : public TurboSound::Chiptune
    {
    public:
      explicit Chiptune(ModuleData::Ptr data)
        : Data(std::move(data))
      {}

      Time::Microseconds GetFrameDuration() const override
      {
        return AYM::BASE_FRAME_DURATION;
      }

      FramedInformation GetInformation() const override
      {
        return CreateTrackInfo(*Data);
      }

      TurboSound::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr first,
                                                       AYM::TrackParameters::Ptr /*second*/) const override
      {
        return Vortex::CreateTurboSoundDataIterator(Data, std::move(first));
      }

    private:
      const ModuleData::Ptr Data;
    };
  }  // namespace TS

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::ProTracker3::Parser parse)
      : Parse(std::move(parse))
    {}

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData,
                             Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Parse(rawData, dataBuilder))
      {
        props.SetSource(*container);
        auto modData = dataBuilder.CaptureResult();
        if (modData->TurboPatternsOffset)
        {
          // TurboSound modules
          props.SetChipsCount(Devices::TurboSound::CHIPS);
          props.SetComment(TURBOSOUND_COMMENT);
          auto chiptune = MakePtr<TS::Chiptune>(std::move(modData));
          return TurboSound::CreateHolder(std::move(chiptune), std::move(properties));
        }
        else
        {
          auto chiptune = MakePtr<Chiptune>(std::move(modData));
          return AYM::CreateHolder(std::move(chiptune), std::move(properties));
        }
      }
      return {};
    }

  private:
    const Formats::Chiptune::ProTracker3::Parser Parse;
  };

  Factory::Ptr CreateFactory(Formats::Chiptune::ProTracker3::Parser parse)
  {
    return MakePtr<Factory>(std::move(parse));
  }
}  // namespace Module::ProTracker3
