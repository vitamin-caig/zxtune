/**
*
* @file
*
* @brief  WAV dumper
*
* @author vitamin.caig@gmail.com
*
**/

#include "../../utils.h"
#include <formats/chiptune/music/wav.h>
#include <strings/format.h>

namespace
{
  using namespace Formats::Chiptune;

  class WavBuilder : public Wav::Builder, public MetaBuilder
  {
  public:
    void SetProgram(const String& program) override
    {
      std::cout << "Program: " << program << std::endl;
    }
    
    void SetTitle(const String& title) override
    {
      std::cout << "Title: " << title << std::endl;
    }
    
    void SetAuthor(const String& author) override
    {
      std::cout << "Author: " << author << std::endl;
    }
    
    void SetStrings(const Strings::Array& strings) override
    {
      for (const auto& str : strings)
      {
        std::cout << "Strings: " <<  str << std::endl;
      }
    }

    MetaBuilder& GetMetaBuilder() override
    {
      return *this;
    }

    void SetProperties(uint_t formatCode, uint_t frequency, uint_t channels, uint_t bits, uint_t blocksize) override
    {
      std::cout <<
      "Format: " << FormatToString(formatCode) <<
      "\nFrequency: " << frequency <<
      "\nChannels: " << channels <<
      "\nBits: " << bits <<
      "\nBlocksize: " << blocksize << std::endl;
    }
    
    void SetSamplesData(Binary::Container::Ptr data) override
    {
      std::cout << "Samples data: " << data->Size() << " bytes" << std::endl;
    }
    
    void SetSamplesCountHint(uint_t count) override
    {
      std::cout << "Samples: " << count << std::endl;
    }
  private:
    static const char* FormatToString(uint_t formatCode)
    {
      switch (formatCode)
      {
      case Wav::Format::PCM:
        return "pcm";
      case Wav::Format::IEEE_FLOAT:
        return "float32";
      default:
        return "other";
      }
    }
  };
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      return 0;
    }
    std::unique_ptr<Dump> rawData(new Dump());
    Test::OpenFile(argv[1], *rawData);
    const Binary::Container::Ptr data = Binary::CreateContainer(std::move(rawData));
    WavBuilder builder;
    if (const auto result = Formats::Chiptune::Wav::Parse(*data, builder))
    {
      std::cout << result->Size() << " bytes total" << std::endl;
    }
    else
    {
      std::cout << "Failed to parse" << std::endl;
    }
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
