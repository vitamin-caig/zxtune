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
#include <formats/chiptune/builder_meta.h>
#include <formats/chiptune/builder_pattern.h>
#include <formats/chiptune/aym/ascsoundmaster.h>

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

  class ASCDumpBuilder : public Builder, public Formats::Chiptune::MetaBuilder, public Formats::Chiptune::PatternBuilder
  {
  public:
    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return *this;
    }

    virtual void SetProgram(const String& program)
    {
      std::cout << "Program: " << program << std::endl;
    }

    virtual void SetTitle(const String& program)
    {
      std::cout << "Title: " << program << std::endl;
    }

    virtual void SetAuthor(const String& author)
    {
      std::cout << "Author: " << author << std::endl;
    }
    
    virtual void SetStrings(const Strings::Array& strings)
    {
      std::cout << "Strings: [some]" << std::endl;
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      std::cout << "Tempo: " << tempo << std::endl;
    }

    virtual void SetSample(uint_t index, const Sample& sample)
    {
      std::cout << "[Sample" << ToHex(index) << "]\n"
      "Loop: " << sample.Loop << ".." << sample.LoopLimit << '\n';
      for (std::vector<Sample::Line>::const_iterator it = sample.Lines.begin(), lim = sample.Lines.end(); it != lim; ++it)
      {
        std::cout << "V=" << ToHex(it->Level) << " T=" << it->ToneDeviation << ' ' << 
          (it->ToneMask ? 'T' : 't') << (it->NoiseMask ? 'N' : 'n') << (it->EnableEnvelope ? 'E' : ' ') << 
          "A=" << it->Adding << " dV=" << it->VolSlide <<
          '\n';
      }
      std::cout << std::endl;
    }

    virtual void SetOrnament(uint_t index, const Ornament& ornament)
    {
      std::cout << "[Ornament" << ToHex(index) << "]\n"
      "Loop: " << ornament.Loop << ".." << ornament.LoopLimit << '\n';
      for (std::vector<Ornament::Line>::const_iterator it = ornament.Lines.begin(), lim = ornament.Lines.end(); it != lim; ++it)
      {
        std::cout << it->NoteAddon << ' ' << it->NoiseAddon << '\n';
      }
      std::cout << std::endl;
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      std::cout << "Positions: ";
      for (uint_t idx = 0, lim = positions.size(); idx != lim; ++idx)
      {
        std::cout << positions[idx];
        if (idx == loop)
        {
          std::cout << 'L';
        }
        std::cout << ' ';
      }
      std::cout << std::endl;
    }

    virtual Formats::Chiptune::PatternBuilder& StartPattern(uint_t index)
    {
      std::cout << std::endl << "[Pattern" << index << ']' << std::endl;
      return *this;
    }

    virtual void Finish(uint_t size)
    {
      std::cout << Line << std::endl;
      std::cout << size << " lines" << std::endl;
      Line.clear();
    }

    virtual void StartLine(uint_t index)
    {
      if (!Line.empty())
      {
        std::cout << Line << std::endl;
      }
      //nn C-1 SEOVee
      Line = String(36, ' ');
      Line[0] = '0' + index / 10;
      Line[1] = '0' + index % 10;
    }

    virtual void SetTempo(uint_t tempo)
    {
      //TODO
    }

    virtual void StartChannel(uint_t index)
    {
      ChanPtr = &Line[3 + index * 11];
      ChanPtr[0] = ChanPtr[1] = ChanPtr[2] = '-';
      ChanPtr[4] = ChanPtr[5] = ChanPtr[6] = ChanPtr[7] = ChanPtr[8] = ChanPtr[9] = '.';
    }

    virtual void SetRest()
    {
      ChanPtr[0] = 'R';
      ChanPtr[1] = '-';
      ChanPtr[2] = '-';
    }
    virtual void SetNote(uint_t note)
    {
      const std::string str = GetNote(note);
      ChanPtr[0] = str[0];
      ChanPtr[1] = str[1];
      ChanPtr[2] = str[2];
    }
    virtual void SetSample(uint_t sample)
    {
      ChanPtr[4] = ToHex(sample);
    }
    virtual void SetOrnament(uint_t ornament)
    {
      ChanPtr[6] = ToHex(ornament);
    }
    virtual void SetVolume(uint_t vol)
    {
      ChanPtr[7] = ToHex(vol);
    }

    virtual void SetEnvelopeType(uint_t type)
    {
      ChanPtr[5] = ToHex(type);
    }

    virtual void SetEnvelopeTone(uint_t value)
    {
      ChanPtr[8] = ToHex(value / 16);
      ChanPtr[9] = ToHex(value % 16);
    }

    virtual void SetEnvelope()
    {
      //TODO
    }

    virtual void SetNoEnvelope()
    {
      ChanPtr[5] = '0';
    }

    virtual void SetNoise(uint_t val)
    {
      //TODO
    }

    virtual void SetContinueSample()
    {
      //TODO
    }

    virtual void SetContinueOrnament()
    {
      //TODO
    }

    virtual void SetGlissade(int_t val)
    {
      //TODO
    }

    virtual void SetSlide(int_t steps, bool useToneSliding)
    {
      //TODO
    }

    virtual void SetVolumeSlide(uint_t period, int_t delta)
    {
      //TODO
    }

    virtual void SetBreakSample()
    {
      //TODO
    }

  private:
    String Line;
    Char* ChanPtr;
  };
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 3)
    {
      return 0;
    }
    std::unique_ptr<Dump> rawData(new Dump());
    Test::OpenFile(argv[2], *rawData);
    const Binary::Container::Ptr data = Binary::CreateContainer(rawData);
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
