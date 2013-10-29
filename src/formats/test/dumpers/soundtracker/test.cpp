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
#include <formats/chiptune/soundtracker.h>

namespace
{
  using namespace Formats::Chiptune::SoundTracker;

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

  class STDumpBuilder : public Builder
  {
  public:
    virtual void SetProgram(const String& program)
    {
      std::cout << "Program: " << program << std::endl;
    }

    virtual void SetTitle(const String& program)
    {
      std::cout << "Title: " << program << std::endl;
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      std::cout << "Tempo: " << tempo << std::endl;
    }

    virtual void SetSample(uint_t index, const Sample& sample)
    {
      std::cout << "Sample" << index << std::endl;
    }

    virtual void SetOrnament(uint_t index, const Ornament& ornament)
    {
      std::cout << "Ornament" << index << std::endl;
    }

    virtual void SetPositions(const std::vector<PositionEntry>& positions)
    {
      std::cout << "Positions: ";
      for (std::vector<PositionEntry>::const_iterator it = positions.begin(), lim = positions.end(); it != lim; ++it)
      {
        std::cout << it->PatternIndex << "(" << it->Transposition << ") ";
      }
      std::cout << std::endl;
    }

    virtual void StartPattern(uint_t index)
    {
      //nn C-1 soETT
      Line = String(33, ' ');
      std::cout << std::endl << "Pattern" << index << ':';
    }
    virtual void FinishPattern(uint_t size)
    {
      std::cout << Line << std::endl;
      std::cout << size << " lines" << std::endl;
    }

    virtual void StartLine(uint_t index)
    {
      std::cout << Line << std::endl;
      Line[0] = '0' + index / 10;
      Line[1] = '0' + index % 10;
    }

    virtual void StartChannel(uint_t index)
    {
      ChanPtr = &Line[3 + index * 10];
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
      ChanPtr[5] = ToHex(ornament);
    }
    virtual void SetEnvelope(uint_t type, uint_t value)
    {
      ChanPtr[6] = ToHex(type);
      ChanPtr[7] = ToHex(value / 16);
      ChanPtr[8] = ToHex(value % 16);
    }
    virtual void SetNoEnvelope()
    {
      ChanPtr[6] = '0';
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
    std::auto_ptr<Dump> rawData(new Dump());
    Test::OpenFile(argv[2], *rawData);
    const Binary::Container::Ptr data = Binary::CreateContainer(rawData);
    const std::string type(argv[1]);
    STDumpBuilder builder;
    if (type == "st1")
    {
      Formats::Chiptune::SoundTracker::Parse(*data, builder);
    }
    else if (type == "stc")
    {
      Formats::Chiptune::SoundTracker::ParseCompiled(*data, builder);
    }
    else if (type == "st3")
    {
      Formats::Chiptune::SoundTracker::ParseVersion3(*data, builder);
    }
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
