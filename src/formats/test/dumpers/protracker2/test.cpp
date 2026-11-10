/**
 *
 * @file
 *
 * @brief  ProTracker v2.x tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/protracker2.h"
#include "formats/test/utils.h"

#include <iostream>
#include <string>

namespace
{
  using namespace Formats::Chiptune::ProTracker2;

  inline std::string GetNote(uint_t note)
  {
    const uint_t NOTES_PER_OCTAVE = 12;
    static const char TONES[] = "C-C#D-D#E-F-F#G-G#A-A#B-";
    const uint_t octave = note / NOTES_PER_OCTAVE;
    const uint_t halftone = note % NOTES_PER_OCTAVE;
    return std::string(TONES + halftone * 2, TONES + halftone * 2 + 2) + char('1' + octave);
  }

  class PT2DumpBuilder
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

    void SetTitle(StringView title) override
    {
      std::cout << "Title: " << title << std::endl;
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

    // Samples and ornaments data is not dumped, only their usage in patterns
    void SetSample(uint_t /*index*/, Sample /*sample*/) override {}

    void SetOrnament(uint_t /*index*/, Ornament /*ornament*/) override {}

    void SetPositions(Positions positions) override
    {
      std::cout << "Positions: ";
      for (uint_t idx = 0, lim = positions.GetSize(); idx != lim; ++idx)
      {
        std::cout << positions.GetLine(idx);
        if (idx == positions.GetLoop())
        {
          std::cout << 'L';
        }
        std::cout << ' ';
      }
      std::cout << std::endl;
    }

    Formats::Chiptune::PatternBuilder& StartPattern(uint_t index) override
    {
      std::cout << "\nP" << index;
      return *this;
    }

    // PatternBuilder
    void Finish(uint_t /*size*/) override {}

    void StartLine(uint_t index) override
    {
      std::cout << "\n\tL" << index;
    }

    void SetTempo(uint_t tempo) override
    {
      std::cout << "\tTempo: " << tempo;
    }

    // Builder
    void StartChannel(uint_t index) override
    {
      std::cout << "\n\t\tC" << index << ':';
    }

    void SetRest() override
    {
      std::cout << "\tRest";
    }

    void SetNote(uint_t note) override
    {
      std::cout << '\t' << GetNote(note);
    }

    void SetSample(uint_t sample) override
    {
      std::cout << "\tSmp" << sample;
    }

    void SetOrnament(uint_t ornament) override
    {
      std::cout << "\tOrn" << ornament;
    }

    void SetVolume(uint_t vol) override
    {
      std::cout << "\tVol" << vol;
    }

    void SetGlissade(int_t val) override
    {
      std::cout << "\tGliss: " << val;
    }

    void SetNoteGliss(int_t val, uint_t limit) override
    {
      std::cout << "\tPorta: " << val << '/' << limit;
    }

    void SetNoGliss() override
    {
      std::cout << "\tNoGliss";
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      std::cout << "\tEnv: " << type << '/' << value;
    }

    void SetNoEnvelope() override
    {
      std::cout << "\tNoEnv";
    }

    void SetNoiseAddon(int_t val) override
    {
      std::cout << "\tNoiseAdd: " << val;
    }
  };
}  // namespace

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      return 0;
    }
    const auto data = Test::OpenFile(argv[1]);
    PT2DumpBuilder builder;
    Formats::Chiptune::ProTracker2::Parse(*data, builder);
    std::cout << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
