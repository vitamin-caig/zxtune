/**
 *
 * @file
 *
 * @brief  SoundTracker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/soundtracker.h"
#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
#include "formats/test/utils.h"

#include "strings/array.h"

#include "string_view.h"
#include "types.h"

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
  using namespace Formats::Chiptune::SoundTracker;

  auto ToHex(uint_t val)
  {
    return val >= 10 ? 'A' + (val - 10) : '0' + val;
  }

  inline std::string GetNote(uint_t note)
  {
    const uint_t NOTES_PER_OCTAVE = 12;
    static const char TONES[] = "C-C#D-D#E-F-F#G-G#A-A#B-";
    const uint_t octave = note / NOTES_PER_OCTAVE;
    const uint_t halftone = note % NOTES_PER_OCTAVE;
    return std::string(TONES + halftone * 2, TONES + halftone * 2 + 2) + char('1' + octave);
  }

  class STDumpBuilder
    : public Builder
    , public Formats::Chiptune::MetaBuilder
    , public Formats::Chiptune::PatternBuilder
  {
  public:
    // Builder
    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return *this;
    }

    // MetaBuilder
    void SetProgram(StringView program) override
    {
      std::cout << "Program: " << program << std::endl;
    }

    void SetTitle(StringView program) override
    {
      std::cout << "Title: " << program << std::endl;
    }

    void SetAuthor(StringView author) override
    {
      std::cout << "Author: " << author << std::endl;
    }

    void SetStrings(const Strings::Array& /*strings*/) override {}

    void SetComment(StringView comment) override
    {
      std::cout << "Comment: " << comment << std::endl;
    }

    // Builder
    void SetInitialTempo(uint_t tempo) override
    {
      std::cout << "Tempo: " << tempo << std::endl;
    }

    void SetSample(uint_t index, Sample /*sample*/) override
    {
      std::cout << "Sample" << index << std::endl;
    }

    void SetOrnament(uint_t index, Ornament /*ornament*/) override
    {
      std::cout << "Ornament" << index << std::endl;
    }

    void SetPositions(Positions positions) override
    {
      std::cout << "Positions: ";
      for (auto position : positions.Lines)
      {
        std::cout << position.PatternIndex << "(" << position.Transposition << ") ";
      }
      std::cout << std::endl;
    }

    Formats::Chiptune::PatternBuilder& StartPattern(uint_t index) override
    {
      // nn C-1 soETT
      Line = String(33, ' ');
      std::cout << std::endl << "Pattern" << index << ':';
      return *this;
    }

    // PatternBuilder
    void Finish(uint_t size) override
    {
      std::cout << Line << std::endl;
      std::cout << size << " lines" << std::endl;
    }

    void StartLine(uint_t index) override
    {
      std::cout << Line << std::endl;
      Line[0] = '0' + index / 10;
      Line[1] = '0' + index % 10;
    }

    void SetTempo(uint_t /*tempo*/) override {}

    // Builder
    void StartChannel(uint_t index) override
    {
      ChanPtr = &Line[3 + index * 10];
    }

    void SetRest() override
    {
      ChanPtr[0] = 'R';
      ChanPtr[1] = '-';
      ChanPtr[2] = '-';
    }
    void SetNote(uint_t note) override
    {
      const std::string str = GetNote(note);
      ChanPtr[0] = str[0];
      ChanPtr[1] = str[1];
      ChanPtr[2] = str[2];
    }
    void SetSample(uint_t sample) override
    {
      ChanPtr[4] = ToHex(sample);
    }
    void SetOrnament(uint_t ornament) override
    {
      ChanPtr[5] = ToHex(ornament);
    }
    void SetEnvelope(uint_t type, uint_t value) override
    {
      ChanPtr[6] = ToHex(type);
      ChanPtr[7] = ToHex(value / 16);
      ChanPtr[8] = ToHex(value % 16);
    }
    void SetNoEnvelope() override
    {
      ChanPtr[6] = '0';
    }

  private:
    String Line;
    char* ChanPtr;
  };

  Formats::Chiptune::SoundTracker::Decoder::Ptr CreateDecoder(const std::string& type)
  {
    if (type == "st1")
    {
      return Formats::Chiptune::SoundTracker::Ver1::CreateUncompiledDecoder();
    }
    else if (type == "stc")
    {
      return Formats::Chiptune::SoundTracker::Ver1::CreateCompiledDecoder();
    }
    else if (type == "st3")
    {
      return Formats::Chiptune::SoundTracker::Ver3::CreateDecoder();
    }
    else
    {
      throw std::runtime_error("Unknown type " + type);
    }
  }
}  // namespace

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 3)
    {
      return 0;
    }
    const auto data = Test::OpenFile(argv[2]);
    const std::string type(argv[1]);
    STDumpBuilder builder;
    const auto decoder = CreateDecoder(type);
    decoder->Parse(*data, builder);
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
