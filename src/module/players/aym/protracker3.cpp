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
#include "module/players/generic_presentation.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"

#include "parameters/tracking_helper.h"

#include "pointers.h"

#include <memory>
#include <optional>

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
      auto& channel = Patterns.GetChannel();
      channel.SetEnabled(true);
      channel.SetNote(note);
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
      Patterns.GetChannel().GetCommands().AddOrReplace(Vortex::GLISS, int_t(period), val);
    }

    void SetNoteGliss(uint_t period, int_t val, uint_t /*limit*/) override
    {
      // ignore limit
      Patterns.GetChannel().GetCommands().AddOrReplace(Vortex::GLISS_NOTE, int_t(period), val);
    }

    void SetSampleOffset(uint_t offset) override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(Vortex::SAMPLEOFFSET, offset);
    }

    void SetOrnamentOffset(uint_t offset) override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(Vortex::ORNAMENTOFFSET, offset);
    }

    void SetVibrate(uint_t ontime, uint_t offtime) override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(Vortex::VIBRATE, ontime, offtime);
    }

    void SetEnvelopeSlide(uint_t period, int_t val) override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(Vortex::SLIDEENV, period, val);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      auto& commands = Patterns.GetChannel().GetCommands();
      commands.Remove(Vortex::ENVELOPE_OFF);
      commands.AddOrReplace(Vortex::ENVELOPE, type, value);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(Vortex::ENVELOPE_OFF);
    }

    void SetNoiseBase(uint_t val) override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(Vortex::NOISEBASE, val);
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

  namespace Presentation
  {
    std::unique_ptr<const TrackPresentation> CreatePresentation(ModuleData::Ptr data);
  }  // namespace Presentation

  class Chiptune : public AYM::TrackingChiptune
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

    std::unique_ptr<const TrackPresentation> CreatePresentation() const override
    {
      return Presentation::CreatePresentation(Data);
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

  namespace Presentation
  {
    constexpr uint_t TEMPO_COMMAND = 11;

    using Delimiter = Module::Presentation::Delimiter;
    using Layout = Module::Presentation::Layout<'.'>;

    struct RowPrefix
    {
      Layout::Hex<4> Envelope;
      Delimiter Separator;
      Layout::Hex<2> Noise;
    };

    struct Command
    {
      Layout::Hex<1> Code;
      Layout::Hex<1> Period;
      Layout::Hex<2> Param;
    };

    struct Channel
    {
      Delimiter Separator;
      Module::Presentation::NoteAndOctave Note;
      char Space = ' ';
      Layout::Base36 Sample;
      Layout::Hex<1> Envelope;
      Layout::Hex<1> Ornament;
      Layout::Hex<1> Volume;
      char Space2 = ' ';
      Command Cmd;
    };

    struct Row
    {
      Delimiter Separator;
      RowPrefix Prefix;
      Channel Channels[AYM::TRACK_CHANNELS];
    };

    struct LineLayout
    {
      Module::Presentation::Dec<2> Number;
      Row Body;
    };

    struct LineContext
    {
      uint_t Tempo = 0;
      RowPrefix& Prefix;
    };

    template<uint_t Width>
    void SetBaseHex(Layout::Hex<Width>& field, int_t value)
    {
      if (value)
      {
        field = static_cast<uint_t>(value);
      }
      else
      {
        field.Clear();
      }
    }

    void FillCommand(const Module::Command& cmd, Command& txt)
    {
      uint_t code = cmd.Type;
      std::optional<uint_t> period;
      int_t param = cmd.Param2;
      switch (cmd.Type)
      {
      case Vortex::GLISS:
        code = param < 0 ? 2 : 1;
        period = cmd.Param1;
        param = param < 0 ? -param : param;
        break;
      case Vortex::GLISS_NOTE:
        code = 3;
        period = cmd.Param1;
        param = param < 0 ? -param : param;
        break;
      case Vortex::SAMPLEOFFSET:
        code = 4;
        param = cmd.Param1;
        break;
      case Vortex::ORNAMENTOFFSET:
        code = 5;
        param = cmd.Param1;
        break;
      case Vortex::VIBRATE:
        code = 6;
        param = cmd.Param1 * 16 + param;
        break;
      case Vortex::SLIDEENV:
        code = param < 0 ? 10 : 9;
        period = cmd.Param1;
        param = param < 0 ? -param : param;
        break;
      default:
        period = cmd.Param1;
        break;
      }
      txt.Code = code;
      txt.Period = period && *period < 16 ? *period : 0;
      txt.Param = param;
    }

    void FillCommand(uint_t tempo, Command& txt)
    {
      txt.Code = TEMPO_COMMAND;
      txt.Period = 0;
      txt.Param = tempo;
    }

    void FillCell(const Cell& cell, Channel& txt, LineContext& context)
    {
      txt.Note.Set(&cell);
      if (const auto* sample = cell.GetSample())
      {
        txt.Sample = *sample;
      }
      bool hasEnvelope = false;
      bool hasEnvelopeOff = false;
      uint_t envelopeType = 0;
      const Module::Command* patternCommand = nullptr;
      for (const auto& cmd : cell.GetCommands())
      {
        switch (cmd.Type)
        {
        case Vortex::ENVELOPE:
          hasEnvelope = true;
          envelopeType = cmd.Param1;
          SetBaseHex(context.Prefix.Envelope, cmd.Param2);
          break;
        case Vortex::ENVELOPE_OFF:
          hasEnvelopeOff = true;
          break;
        case Vortex::NOISEBASE:
          SetBaseHex(context.Prefix.Noise, cmd.Param1);
          break;
        default:
          if (!patternCommand)
          {
            patternCommand = &cmd;
          }
          break;
        }
      }
      if (hasEnvelope)
      {
        txt.Envelope = envelopeType;
      }
      else if (hasEnvelopeOff)
      {
        txt.Envelope = 0x0f;
      }
      if (const auto* ornament = cell.GetOrnament())
      {
        txt.Ornament = *ornament;
      }
      if (const auto* volume = cell.GetVolume())
      {
        txt.Volume = *volume;
      }
      if (patternCommand)
      {
        FillCommand(*patternCommand, txt.Cmd);
      }
      else if (context.Tempo)
      {
        FillCommand(context.Tempo, txt.Cmd);
        context.Tempo = 0;
      }
    }

    void FillLine(const Line& line, Row& row)
    {
      LineContext context{line.GetTempo(), row.Prefix};
      for (uint_t channel = 0; channel != AYM::TRACK_CHANNELS; ++channel)
      {
        if (const auto* cell = line.GetChannel(channel))
        {
          FillCell(*cell, row.Channels[channel], context);
        }
      }
    }

    class TrackTextPresentation : public Module::TrackPresentation
    {
    public:
      explicit TrackTextPresentation(ModuleData::Ptr data)
        : Data(std::move(data))
      {}

      String GetPattern(std::size_t index) const override
      {
        const auto& patterns = Data->GetPatterns();
        if (index >= patterns.GetSize())
        {
          return {};
        }
        const auto* pattern = patterns.Get(static_cast<uint_t>(index));
        if (!pattern || !pattern->GetSize())
        {
          return {};
        }
        const auto turboBase = Data->TurboPatternsOffset;
        const auto mirroredIndex = turboBase && index < *turboBase
                                       ? std::optional<uint_t>(*turboBase - 1 - static_cast<uint_t>(index))
                                       : std::nullopt;
        const auto* mirrored = mirroredIndex ? patterns.Get(*mirroredIndex) : nullptr;
        const auto lines = pattern->GetSize();
        String result;
        result.reserve(lines * (sizeof(LineLayout) + (turboBase ? sizeof(Row) : 0) + 1));
        for (uint_t row = 0; row != lines; ++row)
        {
          auto& dst = Module::Presentation::AppendLine<LineLayout>(result);
          dst.Number = row;
          if (const auto* line = pattern->GetLine(row))
          {
            FillLine(*line, dst.Body);
          }
          if (turboBase)
          {
            auto& mirroredDst = Module::Presentation::AppendTo<Row>(result);
            if (const auto* mirroredLine = mirrored ? mirrored->GetLine(row) : nullptr)
            {
              FillLine(*mirroredLine, mirroredDst);
            }
          }
        }
        return result;
      }

    private:
      const ModuleData::Ptr Data;
    };

    std::unique_ptr<const TrackPresentation> CreatePresentation(ModuleData::Ptr data)
    {
      return std::make_unique<TrackTextPresentation>(std::move(data));
    }
  }  // namespace Presentation

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
          auto chiptune = MakePtr<TS::Chiptune>(modData);
          auto presentation = Presentation::CreatePresentation(std::move(modData));
          return TurboSound::CreateHolder(std::move(chiptune), std::move(properties), std::move(presentation));
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
