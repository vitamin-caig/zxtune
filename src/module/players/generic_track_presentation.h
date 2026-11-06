/**
 *
 * @file
 *
 * @brief  Generic text presentation of track models
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/track_model.h"

#include "module/track_presentation.h"

#include "string_type.h"
#include "string_view.h"

#include <array>
#include <utility>

namespace Module
{
  //! @brief Shared primitives for tracker-like text rendering
  class TextBuilder
  {
  public:
    String Release()
    {
      return std::move(Result);
    }

  protected:
    static uint_t HexDigits(uint_t value)
    {
      uint_t digits = 1;
      while (value >>= 4)
      {
        ++digits;
      }
      return digits;
    }

    static char HexDigit(uint_t value)
    {
      static constexpr auto DIGITS = "0123456789ABCDEF"sv;
      return value < DIGITS.size() ? DIGITS[value] : '?';
    }

    static char Base36Digit(uint_t value)
    {
      static constexpr auto DIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv;
      return value < DIGITS.size() ? DIGITS[value] : '?';
    }

    void AddHex(uint_t value, uint_t width)
    {
      const auto offset = Result.size();
      Result.resize(offset + width, '0');
      for (uint_t idx = width; idx; --idx)
      {
        Result[offset + idx - 1] = HexDigit(value & 0x0f);
        value >>= 4;
      }
    }

    void AddDots(uint_t width)
    {
      Result.append(width, '.');
    }

    void AddNote(uint_t note)
    {
      const auto octave = 1 + note / NOTES_COUNT;
      if (octave > 9)
      {
        Result += "???"sv;
      }
      else
      {
        Result += NOTE_NAMES[note % NOTES_COUNT];
        Result += char('0' + octave);
      }
    }

    void AddNote(const Cell* cell)
    {
      if (!cell)
      {
        Result += EMPTY_NOTE;
        return;
      }
      if (const auto* enabled = cell->GetEnabled(); enabled && !*enabled)
      {
        Result += REST_NOTE;
        return;
      }
      if (const auto* note = cell->GetNote())
      {
        AddNote(*note);
      }
      else
      {
        Result += EMPTY_NOTE;
      }
    }

    String Result;

  private:
    static constexpr auto NOTE_NAMES = std::to_array<StringView>(
        {"C-"sv, "C#"sv, "D-"sv, "D#"sv, "E-"sv, "F-"sv, "F#"sv, "G-"sv, "G#"sv, "A-"sv, "A#"sv, "B-"sv});
    static constexpr auto NOTES_COUNT = NOTE_NAMES.size();
    static constexpr auto EMPTY_NOTE = "---"sv;
    static constexpr auto REST_NOTE = "R--"sv;
  };

  //! @brief Tracker-like text rendering of a TrackModel pattern
  class GenericTrackPresentation : public TrackPresentation
  {
  public:
    explicit GenericTrackPresentation(const TrackModel& model);

    std::string GetPattern(std::size_t index) const override;

  private:
    const TrackModel& Model;
  };
}  // namespace Module
