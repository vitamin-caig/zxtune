/**
 *
 * @file
 *
 * @brief  Pro Sound Creator tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/prosoundcreator.h"
#include "formats/test/utils.h"

#include <iostream>
#include <string>

namespace
{
  using namespace Formats::Chiptune::ProSoundCreator;

  inline std::string GetNote(uint_t note)
  {
    const uint_t NOTES_PER_OCTAVE = 12;
    static const char TONES[] = "C-C#D-D#E-F-F#G-G#A-A#B-";
    const uint_t octave = note / NOTES_PER_OCTAVE;
    const uint_t halftone = note % NOTES_PER_OCTAVE;
    return std::string(TONES + halftone * 2, TONES + halftone * 2 + 2) + char('1' + octave);
  }

  class PSCDumpBuilder
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

    void SetEnvelope(uint_t type, uint_t tone) override
    {
      std::cout << "\tEnv: " << type << '/' << tone;
    }

    void SetEnvelope() override
    {
      std::cout << "\tEnvOn";
    }

    void SetNoEnvelope() override
    {
      std::cout << "\tNoEnv";
    }

    void SetNoiseBase(uint_t val) override
    {
      std::cout << "\tNoise: " << val;
    }

    void SetBreakSample() override
    {
      std::cout << "\tBreakSmp";
    }

    void SetBreakOrnament() override
    {
      std::cout << "\tBreakOrn";
    }

    void SetNoOrnament() override
    {
      std::cout << "\tNoOrn";
    }

    void SetGliss(uint_t absStep) override
    {
      std::cout << "\tGliss: " << absStep;
    }

    void SetSlide(int_t delta) override
    {
      std::cout << "\tSlide: " << delta;
    }

    void SetVolumeSlide(uint_t period, int_t delta) override
    {
      std::cout << "\tVolSlide: " << period << '/' << delta;
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
    PSCDumpBuilder builder;
    Formats::Chiptune::ProSoundCreator::Parse(*data, builder);
    std::cout << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
