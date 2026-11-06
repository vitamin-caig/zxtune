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
#include <memory>
#include <type_traits>
#include <utility>

namespace Module::Presentation
{
  using NoteText = std::array<char, 2>;

  inline constexpr auto NOTES = std::array<NoteText, 12>{{
      {'C', '-'},
      {'C', '#'},
      {'D', '-'},
      {'D', '#'},
      {'E', '-'},
      {'F', '-'},
      {'F', '#'},
      {'G', '-'},
      {'G', '#'},
      {'A', '-'},
      {'A', '#'},
      {'B', '-'},
  }};

  struct NoteAndOctave
  {
    NoteText Note = {'-', '-'};
    char Octave = '-';

    void Set(uint_t note)
    {
      const auto octave = 1 + note / NOTES.size();
      if (octave > 9)
      {
        Note = {'?', '?'};
        Octave = '?';
        return;
      }
      Note = NOTES[note % NOTES.size()];
      Octave = char('0' + octave);
    }

    void Set(const Cell* cell)
    {
      if (!cell)
      {
        return;
      }
      if (const auto* enabled = cell->GetEnabled(); enabled && !*enabled)
      {
        Note = {'R', '-'};
        return;
      }
      if (const auto* note = cell->GetNote())
      {
        Set(*note);
      }
    }
  };

  struct Delimiter : std::array<char, 3>
  {
    constexpr Delimiter()
      : std::array<char, 3>{' ', '|', ' '}
    {}
  };

  template<uint_t Width, char Filler = '0'>
  struct Dec : std::array<char, Width>
  {
    constexpr Dec()
    {
      this->fill(Filler);
    }

    Dec& operator=(uint_t value)
    {
      for (uint_t idx = Width; idx; --idx)
      {
        (*this)[idx - 1] = char('0' + value % 10);
        value /= 10;
      }
      return *this;
    }
  };

  template<uint_t Width>
  using Decimal = Dec<Width>;

  template<char Filler>
  struct Layout
  {
    template<uint_t Width>
    struct Hex : std::array<char, Width>
    {
      static char Digit(uint_t value)
      {
        static constexpr auto DIGITS = "0123456789ABCDEF"sv;
        return value < DIGITS.size() ? DIGITS[value] : '?';
      }

      constexpr Hex()
      {
        this->fill(Filler);
      }

      Hex& operator=(uint_t value)
      {
        if (!value)
        {
          Clear();
          return *this;
        }
        uint_t idx = Width;
        do
        {
          (*this)[--idx] = Digit(value & 0x0f);
          value >>= 4;
        } while (value && idx);
        return *this;
      }

      void Clear()
      {
        this->fill(Filler);
      }
    };

    template<uint_t Width>
    struct Text : std::array<char, Width>
    {
      constexpr Text()
      {
        this->fill(Filler);
      }

      Text& operator=(StringView value)
      {
        this->fill(Filler);
        const auto size = value.size() < Width ? value.size() : Width;
        for (uint_t idx = 0; idx != size; ++idx)
        {
          (*this)[idx] = value[idx];
        }
        return *this;
      }
    };

    struct Base36
    {
      static char Digit(uint_t value)
      {
        static constexpr auto DIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv;
        return value < DIGITS.size() ? DIGITS[value] : '?';
      }

      char Value = Filler;

      Base36& operator=(uint_t value)
      {
        Value = Digit(value);
        return *this;
      }

      void Clear()
      {
        Value = Filler;
      }
    };
  };

  template<class T>
  T& AppendTo(String& out)
  {
    static_assert(alignof(T) == 1, "Layout must be byte-aligned");
    static_assert(std::is_trivially_copyable_v<T>, "Layout must be trivially copyable");
    static_assert(std::is_trivially_destructible_v<T>, "Layout must be trivially destructible");
    const auto offset = out.size();
    out.resize(offset + sizeof(T));
    return *std::construct_at(reinterpret_cast<T*>(out.data() + offset));
  }

  template<class T>
  T& AppendLine(String& out)
  {
    if (!out.empty())
    {
      out += '\n';
    }
    return AppendTo<T>(out);
  }

  //! @brief Tracker-like text rendering of a TrackModel pattern
  class Generic : public TrackPresentation
  {
  public:
    using PresentPatternFunction = String (*)(const Pattern&, uint_t channels);

    explicit Generic(const TrackModel& model, PresentPatternFunction presentPattern = nullptr);

    String GetPattern(std::size_t index) const override;

  private:
    const TrackModel& Model;
    const PresentPatternFunction PresentPattern;
  };
}  // namespace Module::Presentation
