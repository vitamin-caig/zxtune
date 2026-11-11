/**
 *
 * @file
 *
 * @brief  ProTracker3 tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/protracker3.h"
#include "formats/test/utils.h"

#include <iostream>
#include <string>

namespace
{
  using namespace Formats::Chiptune::ProTracker3;

  inline std::string GetNote(uint_t note)
  {
    const uint_t NOTES_PER_OCTAVE = 12;
    static const char TONES[] = "C-C#D-D#E-F-F#G-G#A-A#B-";
    const uint_t octave = note / NOTES_PER_OCTAVE;
    const uint_t halftone = note % NOTES_PER_OCTAVE;
    return std::string(TONES + halftone * 2, TONES + halftone * 2 + 2) + char('1' + octave);
  }

  class PT3DumpBuilder
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
    void SetVersion(uint_t version) override
    {
      std::cout << "Version: 3." << version << std::endl;
    }

    void SetNoteTable(NoteTable table) override
    {
      std::cout << "NoteTable: ";
      switch (table)
      {
      case PROTRACKER:
        std::cout << "ProTracker";
        break;
      case SOUNDTRACKER:
        std::cout << "SoundTracker";
        break;
      case ASM:
        std::cout << "ASM";
        break;
      case REAL:
        std::cout << "Real";
        break;
      case NATURAL:
        std::cout << "Natural";
        break;
      }
      std::cout << std::endl;
    }

    void SetMode(uint_t mode) override
    {
      std::cout << "Mode: " << mode << std::endl;
    }

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

    void SetGlissade(uint_t period, int_t val) override
    {
      std::cout << "\tGliss: " << period << '/' << val;
    }

    void SetNoteGliss(uint_t period, int_t val, uint_t limit) override
    {
      std::cout << "\tPorta: " << period << '/' << val << '/' << limit;
    }

    void SetSampleOffset(uint_t offset) override
    {
      std::cout << "\tSmpOfs: " << offset;
    }

    void SetOrnamentOffset(uint_t offset) override
    {
      std::cout << "\tOrnOfs: " << offset;
    }

    void SetVibrate(uint_t ontime, uint_t offtime) override
    {
      std::cout << "\tVibrate: " << ontime << '/' << offtime;
    }

    void SetEnvelopeSlide(uint_t period, int_t val) override
    {
      std::cout << "\tEnvSlide: " << period << '/' << val;
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      std::cout << "\tEnv: " << type << '/' << value;
    }

    void SetNoEnvelope() override
    {
      std::cout << "\tNoEnv";
    }

    void SetNoiseBase(uint_t val) override
    {
      std::cout << "\tNoise: " << val;
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
    PT3DumpBuilder builder;
    const std::string type(argv[1]);
    if (type == "txt")
    {
      Formats::Chiptune::ProTracker3::VortexTracker2::Parse(*data, builder);
    }
    else if (type == "pt3")
    {
      Formats::Chiptune::ProTracker3::Parse(*data, builder);
    }
    std::cout << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
