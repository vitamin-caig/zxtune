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
#include "module/players/track_builder.h"

#include "parameters/tracking_helper.h"

#include "make_ptr.h"
#include "pointers.h"

namespace Module::ProTracker3
{
  using ModuleData = Vortex::ModuleData;

  const auto TURBOSOUND_COMMENT = "TurboSound module"sv;

  class DataChannelBuilder : public TrackChannelBuilder<Formats::Chiptune::ProTracker3::ChannelBuilder>
  {
  public:
    void SetGlissade(uint_t period, int_t val) override
    {
      GetCommands().AddOrReplace(Vortex::GLISS, int_t(period), val);
    }
    void SetNoteGliss(uint_t period, int_t val, uint_t /*limit*/) override
    {
      // ignore limit
      GetCommands().AddOrReplace(Vortex::GLISS_NOTE, int_t(period), val);
    }
    void SetSampleOffset(uint_t offset) override
    {
      GetCommands().AddOrReplace(Vortex::SAMPLEOFFSET, offset);
    }
    void SetOrnamentOffset(uint_t offset) override
    {
      GetCommands().AddOrReplace(Vortex::ORNAMENTOFFSET, offset);
    }
    void SetVibrate(uint_t ontime, uint_t offtime) override
    {
      GetCommands().AddOrReplace(Vortex::VIBRATE, ontime, offtime);
    }
    void SetEnvelopeSlide(uint_t period, int_t val) override
    {
      GetCommands().AddOrReplace(Vortex::SLIDEENV, period, val);
    }
    void SetEnvelope(uint_t type, uint_t value) override
    {
      auto& commands = GetCommands();
      commands.Remove(Vortex::ENVELOPE_OFF);
      commands.AddOrReplace(Vortex::ENVELOPE, type, value);
    }
    void SetNoEnvelope() override
    {
      GetCommands().AddOrReplace(Vortex::ENVELOPE_OFF);
    }
    void SetNoiseBase(uint_t val) override
    {
      GetCommands().AddOrReplace(Vortex::NOISEBASE, val);
    }
  };

  using LineBuilder = TrackLineBuilder<Formats::Chiptune::ProTracker3::ChannelBuilder, DataChannelBuilder>;
  using PatternsBuilder = TrackPatternsBuilder<Formats::Chiptune::ProTracker3::ChannelBuilder, DataChannelBuilder>;

  class DataBuilder : public TrackDataBuilder<Formats::Chiptune::ProTracker3::Builder, ModuleData, PatternsBuilder>
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : TrackDataBuilder(props)
      , Properties(props)
    {}

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

  private:
    AYM::PropertiesHelper& Properties;
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
