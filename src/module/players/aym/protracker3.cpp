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
#include "module/players/generic_track_presentation.h"
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

    const TrackModel& GetTrackModel() const override
    {
      return *Data;
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

  namespace Tracking
  {
    constexpr auto COLUMNS_DELIMITER = "|"sv;
    constexpr uint_t LINE_NUMBER_WIDTH = 2;
    constexpr uint_t ENVELOPE_BASE_WIDTH = 4;
    constexpr uint_t NOISE_BASE_WIDTH = 2;
    constexpr uint_t COMMAND_WIDTH = 4;
    constexpr uint_t TEMPO_COMMAND = 11;

    class PatternTextBuilder : public TextBuilder
    {
    public:
      void AddLine(uint_t row, const Line* line)
      {
        if (!Result.empty())
        {
          Result += '\n';
        }
        AddHex(row, LINE_NUMBER_WIDTH);
        AddLine(line);
      }

      void AddLine(const Line* line)
      {
        Result += COLUMNS_DELIMITER;
        AddRowPrefix(line);
        auto tempo = line ? line->GetTempo() : 0;
        for (uint_t channel = 0; channel != AYM::TRACK_CHANNELS; ++channel)
        {
          Result += COLUMNS_DELIMITER;
          AddCell(line ? line->GetChannel(channel) : nullptr, tempo);
        }
      }

    private:
      static const Command* FindCommand(const Cell* cell, uint_t type)
      {
        if (cell)
        {
          for (const auto& cmd : cell->GetCommands())
          {
            if (cmd.Type == type)
            {
              return &cmd;
            }
          }
        }
        return nullptr;
      }

      static const Command* FindRightmostCommand(const Line* line, uint_t type)
      {
        if (line)
        {
          // If several channels have the same global command, the rightmost one defines the row state
          for (uint_t channel = AYM::TRACK_CHANNELS; channel--;)
          {
            if (const auto* cmd = FindCommand(line->GetChannel(channel), type))
            {
              return cmd;
            }
          }
        }
        return nullptr;
      }

      static const Command* FindPatternCommand(const Cell* cell)
      {
        if (cell)
        {
          for (const auto& cmd : cell->GetCommands())
          {
            switch (cmd.Type)
            {
            case Vortex::ENVELOPE:
            case Vortex::NOENVELOPE:
            case Vortex::NOISEBASE:
              // These are represented by the envelope/noise fields instead
              break;
            default:
              return &cmd;
            }
          }
        }
        return nullptr;
      }

      void AddCell(const Cell* cell, uint_t& tempo)
      {
        AddCellNote(cell);
        AddFields(cell);
        AddCommand(cell, tempo);
      }

      void AddRowPrefix(const Line* line)
      {
        if (const auto* envelope = FindRightmostCommand(line, Vortex::ENVELOPE); envelope && envelope->Param2)
        {
          AddHex(static_cast<uint_t>(envelope->Param2), ENVELOPE_BASE_WIDTH);
        }
        else
        {
          AddDots(ENVELOPE_BASE_WIDTH);
        }
        Result += COLUMNS_DELIMITER;
        if (const auto* noise = FindRightmostCommand(line, Vortex::NOISEBASE); noise && noise->Param1)
        {
          AddHex(static_cast<uint_t>(noise->Param1), NOISE_BASE_WIDTH);
        }
        else
        {
          AddDots(NOISE_BASE_WIDTH);
        }
      }

      void AddCellNote(const Cell* cell)
      {
        const auto* enabled = cell ? cell->GetEnabled() : nullptr;
        if (enabled && !*enabled)
        {
          AddNote(cell);
          return;
        }
        if (const auto* gliss = FindCommand(cell, Vortex::GLISS_NOTE))
        {
          // The note is the glissade target rather than a separate cell field
          AddNote(static_cast<uint_t>(gliss->Param3));
          return;
        }
        AddNote(cell);
      }

      void AddFields(const Cell* cell)
      {
        Result += ' ';
        if (const auto* sample = cell ? cell->GetSample() : nullptr)
        {
          Result += Base36Digit(*sample);
        }
        else
        {
          Result += '.';
        }
        if (const auto* cmd = FindCommand(cell, Vortex::ENVELOPE))
        {
          Result += HexDigit(cmd->Param1);
        }
        else if (FindCommand(cell, Vortex::NOENVELOPE))
        {
          Result += 'F';
        }
        else
        {
          Result += '.';
        }
        if (const auto* ornament = cell ? cell->GetOrnament() : nullptr)
        {
          Result += HexDigit(*ornament);
        }
        else
        {
          Result += '.';
        }
        if (const auto* volume = cell ? cell->GetVolume() : nullptr)
        {
          Result += HexDigit(*volume);
        }
        else
        {
          Result += '.';
        }
      }

      void AddCommand(const Cell* cell, uint_t& tempo)
      {
        Result += ' ';
        const auto* cmd = FindPatternCommand(cell);
        if (!cmd)
        {
          if (tempo)
          {
            Result += HexDigit(TEMPO_COMMAND);
            Result += '.';
            AddHex(tempo, 2);
            tempo = 0;
          }
          else
          {
            AddDots(COMMAND_WIDTH);
          }
          return;
        }
        uint_t code = cmd->Type;
        std::optional<uint_t> period;
        uint_t param = static_cast<uint_t>(cmd->Param2);
        switch (cmd->Type)
        {
        case Vortex::GLISS:
          code = cmd->Param2 < 0 ? 2 : 1;
          period = static_cast<uint_t>(cmd->Param1);
          param = static_cast<uint_t>(cmd->Param2 < 0 ? -cmd->Param2 : cmd->Param2);
          break;
        case Vortex::GLISS_NOTE:
          code = 3;
          period = static_cast<uint_t>(cmd->Param1);
          param = static_cast<uint_t>(cmd->Param2 < 0 ? -cmd->Param2 : cmd->Param2);
          break;
        case Vortex::SAMPLEOFFSET:
          code = 4;
          param = static_cast<uint_t>(cmd->Param1);
          break;
        case Vortex::ORNAMENTOFFSET:
          code = 5;
          param = static_cast<uint_t>(cmd->Param1);
          break;
        case Vortex::VIBRATE:
          code = 6;
          param = static_cast<uint_t>(cmd->Param1 * 16 + cmd->Param2);
          break;
        case Vortex::SLIDEENV:
          code = cmd->Param2 < 0 ? 10 : 9;
          period = static_cast<uint_t>(cmd->Param1);
          param = static_cast<uint_t>(cmd->Param2 < 0 ? -cmd->Param2 : cmd->Param2);
          break;
        default:
          period = static_cast<uint_t>(cmd->Param1);
          break;
        }
        Result += HexDigit(code);
        if (period && *period < 16)
        {
          Result += HexDigit(*period);
        }
        else
        {
          Result += '.';
        }
        AddHex(param, 2);
      }
    };

    class TrackTextPresentation : public Module::TrackPresentation
    {
    public:
      explicit TrackTextPresentation(ModuleData::Ptr data)
        : Data(std::move(data))
      {}

      std::string GetPattern(std::size_t index) const override
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
        PatternTextBuilder builder;
        for (uint_t row = 0; row != lines; ++row)
        {
          const auto* line = pattern->GetLine(row);
          builder.AddLine(row, line);
          if (turboBase)
          {
            const auto* mirroredLine = mirrored ? mirrored->GetLine(row) : nullptr;
            builder.AddLine(mirroredLine);
          }
        }
        return builder.Release();
      }

    private:
      const ModuleData::Ptr Data;
    };

    std::unique_ptr<const TrackPresentation> CreateTrackPresentation(ModuleData::Ptr data)
    {
      return std::make_unique<TrackTextPresentation>(std::move(data));
    }
  }  // namespace Tracking

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
        auto presentation = Tracking::CreateTrackPresentation(modData);
        if (modData->TurboPatternsOffset)
        {
          // TurboSound modules
          props.SetChipsCount(Devices::TurboSound::CHIPS);
          props.SetComment(TURBOSOUND_COMMENT);
          auto chiptune = MakePtr<TS::Chiptune>(std::move(modData));
          return TurboSound::CreateHolder(std::move(chiptune), std::move(properties), std::move(presentation));
        }
        else
        {
          auto chiptune = MakePtr<Chiptune>(std::move(modData));
          return AYM::CreateHolder(std::move(chiptune), std::move(properties), std::move(presentation));
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
