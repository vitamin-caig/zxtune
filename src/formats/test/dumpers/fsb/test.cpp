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
#include <iomanip>
#include <strings/format.h>

namespace
{
  using namespace Formats::Archived::Fmod;

  void Write(uint_t level, StringView msg)
  {
    if (level)
    {
      std::cout << std::setw(level) << ' ';
    }
    std::cout << msg << std::endl;
  }

  template<class... P>
  constexpr void Write(uint_t level, Strings::FormatString<P...> msg, P&&... params)
  {
    Write(level, Strings::Format(msg, std::forward<P>(params)...));
  }

  char ToHex(uint_t nib)
  {
    return nib > 9 ? 'a' + (nib - 10) : '0' + nib;
  }

  char ToSym(uint_t val)
  {
    return val >= ' ' && val < 0x7f ? val : '.';
  }

  void DumpHex(uint_t level, Binary::View data)
  {
    const std::size_t LINE_SIZE = 16;
    const auto DUMP_SIZE = std::min<std::size_t>(data.Size(), 256);
    for (std::size_t offset = 0; offset < DUMP_SIZE;)
    {
      const auto* const in = data.As<uint8_t>() + offset;
      const auto toPrint = std::min(data.Size() - offset, LINE_SIZE);
      std::string msg(5 + 2 + LINE_SIZE * 3 + 2 + LINE_SIZE, ' ');
      msg[0] = ToHex((offset >> 12) & 15);
      msg[1] = ToHex((offset >> 8) & 15);
      msg[2] = ToHex((offset >> 4) & 15);
      msg[3] = ToHex((offset >> 0) & 15);
      msg[5] = '|';
      msg[7 + LINE_SIZE * 3] = '|';
      for (std::size_t idx = 0; idx < toPrint; ++idx)
      {
        msg[7 + idx * 3 + 0] = ToHex(in[idx] >> 4);
        msg[7 + idx * 3 + 1] = ToHex(in[idx] & 15);
        msg[7 + LINE_SIZE * 3 + 2 + idx] = ToSym(in[idx]);
      }
      Write(level, msg.c_str());
      offset += toPrint;
    }
    if (DUMP_SIZE != data.Size())
    {
      Write(level, "<truncated>");
    }
  }

  class FsbBuilder : public Builder
  {
  public:
    void Setup(uint_t samplesCount, uint_t format) override
    {
      Write(0, "{} samples type {} ({})", samplesCount, format, FormatString(format));
    }

    void StartSample(uint_t idx) override
    {
      Write(1, "Sample {}", idx);
    }

    void SetFrequency(uint_t frequency) override
    {
      Write(2, "Frequency: {}Hz", frequency);
    }

    void SetChannels(uint_t channels) override
    {
      Write(2, "Channels: {}", channels);
    }

    void SetName(StringView name) override
    {
      Write(2, "Name: {}", name);
    }

    void AddMetaChunk(uint_t type, Binary::View chunk) override
    {
      Write(2, "Meta chunk {} ({}) {} bytes", type, ChunkTypeString(type), chunk.Size());
      DumpHex(3, chunk);
    }

    void SetData(uint_t samplesCount, Binary::Container::Ptr blob) override
    {
      Write(2, "Data: {} samples in {} bytes", samplesCount, blob ? blob->Size() : std::size_t(0));
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
        return "atrac9";
      case Format::XWMA:
        return "xwma";
      case Format::VORBIS:
        return "vorbis";
      case Format::FADPCM:
        return "fadpcm";
      default:
        return "unknown";
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
      case ChunkType::ATRAC9DATA:
        return "atrac9data";
      case ChunkType::VORBISDATA:
        return "vorbisdata";
      default:
        return "unknown";
      }
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
