/**
 *
 * @file
 *
 * @brief  SoundTracker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../../utils.h"
#include <formats/chiptune/aym/ascsoundmaster.h>
#include <formats/chiptune/builder_meta.h>
#include <formats/chiptune/builder_pattern.h>
#include <string_view.h>

namespace
{
  using namespace Formats::Chiptune::ASCSoundMaster;

  Char ToHex(uint_t val)
  {
    return val >= 10 ? (val - 10 + 'A') : val + '0';
  }

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
    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return *this;
    }

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

    void SetStrings(const Strings::Array& /*strings*/) override
    {
      std::cout << "Strings: [some]" << std::endl;
    }

    void SetComment(StringView comment) override
    {
      std::cout << "Comment: " << comment << std::endl;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      std::cout << "Tempo: " << tempo << std::endl;
    }

    void SetSample(uint_t index, Sample sample) override
    {
      std::cout << "[Sample" << ToHex(index)
                << "]\n"
                   "Loop: "
                << sample.Loop << ".." << sample.LoopLimit << '\n';
      for (const auto& it : sample.Lines)
      {
        std::cout << "V=" << ToHex(it.Level) << " T=" << it.ToneDeviation << ' ' << (it.ToneMask ? 'T' : 't')
                  << (it.NoiseMask ? 'N' : 'n') << (it.EnableEnvelope ? 'E' : ' ') << "A=" << it.Adding
                  << " dV=" << it.VolSlide << '\n';
      }
      std::cout << std::endl;
    }

    void SetOrnament(uint_t index, Ornament ornament) override
    {
      std::cout << "[Ornament" << ToHex(index)
                << "]\n"
                   "Loop: "
                << ornament.Loop << ".." << ornament.LoopLimit << '\n';
      for (auto it : ornament.Lines)
      {
        std::cout << it.NoteAddon << ' ' << it.NoiseAddon << '\n';
      }
      std::cout << std::endl;
    }

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
      std::cout << std::endl << "[Pattern" << index << ']' << std::endl;
      return *this;
    }

    void Finish(uint_t size) override
    {
      std::cout << Line << std::endl;
      std::cout << size << " lines" << std::endl;
      Line.clear();
    }

    void StartLine(uint_t index) override
    {
      if (!Line.empty())
      {
        std::cout << Line << std::endl;
      }
      // nn C-1 SEOVee
      Line = String(36, ' ');
      Line[0] = '0' + index / 10;
      Line[1] = '0' + index % 10;
    }

    void SetTempo(uint_t tempo) override
    {
      // TODO
    }

    void StartChannel(uint_t index) override
    {
      ChanPtr = &Line[3 + index * 11];
      ChanPtr[0] = ChanPtr[1] = ChanPtr[2] = '-';
      ChanPtr[4] = ChanPtr[5] = ChanPtr[6] = ChanPtr[7] = ChanPtr[8] = ChanPtr[9] = '.';
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
      ChanPtr[6] = ToHex(ornament);
    }
    void SetVolume(uint_t vol) override
    {
      ChanPtr[7] = ToHex(vol);
    }

    void SetEnvelopeType(uint_t type) override
    {
      ChanPtr[5] = ToHex(type);
    }

    void SetEnvelopeTone(uint_t value) override
    {
      ChanPtr[8] = ToHex(value / 16);
      ChanPtr[9] = ToHex(value % 16);
    }

    void SetEnvelope() override
    {
      // TODO
    }

    void SetNoEnvelope() override
    {
      ChanPtr[5] = '0';
    }

    void SetNoise(uint_t val) override
    {
      // TODO
    }

    void SetContinueSample() override
    {
      // TODO
    }

    void SetContinueOrnament() override
    {
      // TODO
    }

    void SetGlissade(int_t val) override
    {
      // TODO
    }

    void SetSlide(int_t steps, bool useToneSliding) override
    {
      // TODO
    }

    void SetVolumeSlide(uint_t period, int_t delta) override
    {
      // TODO
    }

    void SetBreakSample() override
    {
      // TODO
    }

  private:
    String Line;
    Char* ChanPtr;
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
      Formats::Chiptune::ASCSoundMaster::Ver0::CreateDecoder()->Parse(*data, builder);
    }
    else if (type == "asc")
    {
      Formats::Chiptune::ASCSoundMaster::Ver1::CreateDecoder()->Parse(*data, builder);
    }
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
