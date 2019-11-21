/**
*
* @file
*
* @brief  FSB dumper
*
* @author vitamin.caig@gmail.com
*
**/

#include "../../utils.h"
#include <formats/archived/fmod.h>
#include <strings/format.h>

namespace
{
  using namespace Formats::Archived::Fmod;

  class FsbBuilder : public Builder
  {
  public:
    void Setup(uint_t samplesCount, uint_t format) override
    {
      std::cout << samplesCount << " samples " << FormatString(format) << std::endl;
    }
    
    void StartSample(uint_t idx) override
    {
      std::cout << " Sample " << idx << std::endl;
    }

    void SetFrequency(uint_t frequency) override
    {
      std::cout << "  Frequency: " << frequency << std::endl;
    }
    
    void SetChannels(uint_t channels) override
    {
      std::cout << "  Channels: " << channels << std::endl;
    }
    
    void SetName(String name) override
    {
      std::cout << "  Name: " << name << std::endl;
    }
    
    void AddMetaChunk(uint_t type, const Binary::Data& chunk) override
    {
      std::cout << "  Meta chunk " << type << " (" << ChunkTypeString(type) << ") " << chunk.Size() << " bytes" << std::endl;
    }
    
    void SetData(uint_t samplesCount, Binary::Container::Ptr blob) override
    {
      std::cout << "  Data: " << samplesCount << " samples in ";
      if (blob)
      {
        std::cout << blob->Size() << " bytes";
      }
      else
      {
        std::cout << " absent data";
      }
      std::cout << std::endl;
    }
  private:
    static const char* FormatString(uint_t format)
    {
      switch (format)
      {
      case Format::UNKNOWN:
        return "unknown";
      case Format::PCM8:
        return "pcm8";
      case Format::PCM16:
        return "pcm16";
      case Format::PCM24:
        return "pcm24";
      case Format::PCM32:
        return "pcm32";
      case Format::PCMFLOAT:
        return "pcmfloat";
      case Format::GCADPCM:
        return "gcadpcm";
      case Format::IMAADPCM:
        return "imaadpcm";
      case Format::VAG:
        return "vag";
      case Format::HEVAG:
        return "hevag";
      case Format::XMA:
        return "xma";
      case Format::MPEG:
        return "mpeg";
      case Format::CELT:
        return "celt";
      case Format::AT9:
        return "at9";
      case Format::XWMA:
        return "xwma";
      case Format::VORBIS:
        return "vorbis";
      default:
        return "invalid";
      }
    }
    
    static const char* ChunkTypeString(uint_t type)
    {
      switch (type)
      {
      case ChunkType::CHANNELS:
        return "channels";
      case ChunkType::FREQUENCY:
        return "frequency";
      case ChunkType::LOOP:
        return "loop";
      case ChunkType::XMASEEK:
        return "xmaseek";
      case ChunkType::DSPCOEFF:
        return "dspcoeff";
      case ChunkType::XWMADATA:
        return "xwmadata";
      case ChunkType::VORBISDATA:
        return "vorbisdata";
      default:
        return "unknown";
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
    FsbBuilder builder;
    if (const auto result = Formats::Archived::Fmod::Parse(*data, builder))
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
