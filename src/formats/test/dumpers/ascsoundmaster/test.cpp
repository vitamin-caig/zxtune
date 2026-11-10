/**
 *
 * @file
 *
 * @brief  ASC Sound Master tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/ascsoundmaster.h"
#include "formats/test/utils.h"

#include <iostream>
#include <string>

namespace
{
  using namespace Formats::Chiptune::ASCSoundMaster;

  inline std::string GetNote(uint_t note)
  {
    const uint_t NOTES_PER_OCTAVE = 12;
    static const char TONES[] = "C-C#D-D#E-F-F#G-G#A-A#B-";
    const uint_t octave = note / NOTES_PER_OCTAVE;
    const uint_t halftone = note % NOTES_PER_OCTAVE;
    return std::string(TONES + halftone * 2, TONES + halftone * 2 + 2) + char('1' + octave);
  }

  class ASCDumpBuilder
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

    void SetEnvelopeType(uint_t type) override
    {
      std::cout << "\tEnv: " << type;
    }

    void SetEnvelopeTone(uint_t tone) override
    {
      std::cout << "\tEnvTone: " << tone;
    }

    void SetEnvelope() override
    {
      std::cout << "\tEnvOn";
    }

    void SetNoEnvelope() override
    {
      std::cout << "\tNoEnv";
    }

    void SetNoise(uint_t val) override
    {
      std::cout << "\tNoise: " << val;
    }

    void SetContinueSample() override
    {
      std::cout << "\tContSmp";
    }

    void SetContinueOrnament() override
    {
      std::cout << "\tContOrn";
    }

    void SetGlissade(int_t val) override
    {
      std::cout << "\tGliss: " << val;
    }

    void SetSlide(int_t steps) override
    {
      std::cout << "\tSlide: " << steps;
    }

    void SetNoteSlide(int_t steps, bool useToneSliding) override
    {
      std::cout << "\tPorta: " << steps;
      if (useToneSliding)
      {
        std::cout << "(tone)";
      }
    }

    void SetVolumeSlide(uint_t period, int_t delta) override
    {
      std::cout << "\tVolSlide: " << period << '/' << delta;
    }

    void SetBreakSample() override
    {
      std::cout << "\tBreak";
    }
  };
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
    ASCDumpBuilder builder;
    if (type == "as0")
    {
      Formats::Chiptune::ASCSoundMaster::Ver0::Parse(*data, builder);
    }
    else if (type == "asc")
    {
      Formats::Chiptune::ASCSoundMaster::Ver1::Parse(*data, builder);
    }
    std::cout << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
