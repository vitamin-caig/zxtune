/**
 *
 * @file
 *
 * @brief  Generic text presentation of track models implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/generic_track_presentation.h"

#include "module/players/track_model.h"

#include "string_type.h"
#include "string_view.h"

#include <algorithm>
#include <array>

namespace Module
{
  namespace
  {
    constexpr auto NOTE_NAMES = std::to_array<StringView>(
        {"C-"sv, "C#"sv, "D-"sv, "D#"sv, "E-"sv, "F-"sv, "F#"sv, "G-"sv, "G#"sv, "A-"sv, "A#"sv, "B-"sv});
    constexpr auto NOTES_COUNT = NOTE_NAMES.size();
    constexpr auto EMPTY_NOTE = "---"sv;
    constexpr auto REST_NOTE = "R--"sv;
    constexpr auto COLUMNS_DELIMITER = " | "sv;
    const uint_t LINE_NUMBER_MIN_WIDTH = 2;
    const uint_t COMMAND_WIDTH = 2;

    uint_t HexDigits(uint_t value)
    {
      uint_t digits = 1;
      while (value >>= 4)
      {
        ++digits;
      }
      return digits;
    }

    char Base36Digit(uint_t value)
    {
      static constexpr auto DIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv;
      return value < DIGITS.size() ? DIGITS[value] : '?';
    }

    class TextBuilder
    {
    public:
      void AddLine(const Pattern& pattern, uint_t row, uint_t channels, uint_t lineNumberWidth)
      {
        if (!Result.empty())
        {
          Result += '\n';
        }
        AddHex(row, lineNumberWidth);
        const auto* line = pattern.GetLine(row);
        for (uint_t channel = 0; channel != channels; ++channel)
        {
          Result += COLUMNS_DELIMITER;
          AddCell(line ? line->GetChannel(channel) : nullptr);
        }
      }

      String Release()
      {
        return std::move(Result);
      }

    private:
      void AddHex(uint_t value, uint_t width)
      {
        static constexpr auto DIGITS = "0123456789ABCDEF"sv;
        String field(width, '0');
        for (uint_t idx = width; idx; --idx)
        {
          field[idx - 1] = DIGITS[value & 0x0f];
          value >>= 4;
        }
        Result += field;
      }

      void AddCell(const Cell* cell)
      {
        // Generic layout from HIGHLIGHTS.md: note, sample/ornament/volume group, command
        AddNote(cell);
        Result += ' ';
        AddField(cell ? cell->GetSample() : nullptr);
        AddField(cell ? cell->GetOrnament() : nullptr);
        AddField(cell ? cell->GetVolume() : nullptr);
        Result += ' ';
        if (cell && !cell->GetCommands().empty())
        {
          AddHex(cell->GetCommands().front().Type, COMMAND_WIDTH);
        }
        else
        {
          AddDots(COMMAND_WIDTH);
        }
      }

      void AddNote(const Cell* cell)
      {
        if (!cell)
        {
          Result += EMPTY_NOTE;
        }
        else if (const auto* enabled = cell->GetEnabled(); enabled && !*enabled)
        {
          Result += REST_NOTE;
        }
        else if (const auto* note = cell->GetNote())
        {
          const auto octave = 1 + *note / NOTES_COUNT;
          if (octave > 9)
          {
            Result += "???"sv;
          }
          else
          {
            Result += NOTE_NAMES[*note % NOTES_COUNT];
            Result += char('0' + octave);
          }
        }
        else
        {
          Result += EMPTY_NOTE;
        }
      }

      void AddField(const uint_t* value)
      {
        if (value)
        {
          Result += Base36Digit(*value);
        }
        else
        {
          Result += '.';
        }
      }

      void AddDots(uint_t width)
      {
        Result.append(width, '.');
      }

    private:
      String Result;
    };
  }  // namespace

  GenericTrackPresentation::GenericTrackPresentation(const TrackModel& model)
    : Model(model)
  {}

  std::string GenericTrackPresentation::GetPattern(std::size_t index) const
  {
    const auto& patterns = Model.GetPatterns();
    if (index >= patterns.GetSize())
    {
      return {};
    }
    const auto* pattern = patterns.Get(static_cast<uint_t>(index));
    if (!pattern || !pattern->GetSize())
    {
      return {};
    }
    const auto channels = Model.GetChannelsCount();
    const auto lines = pattern->GetSize();
    const auto lineNumberWidth = std::max(LINE_NUMBER_MIN_WIDTH, HexDigits(lines - 1));
    TextBuilder builder;
    for (uint_t row = 0; row != lines; ++row)
    {
      builder.AddLine(*pattern, row, channels, lineNumberWidth);
    }
    return builder.Release();
  }
}  // namespace Module
