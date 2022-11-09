/**
 *
 * @file
 *
 * @brief  FLAC dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../../utils.h"
#include <formats/chiptune/music/flac.h>
#include <strings/format.h>

namespace
{
  using namespace Formats::Chiptune;

  class FlacBuilder
    : public Flac::Builder
    , public MetaBuilder
  {
  public:
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

    void SetStrings(const Strings::Array& strings) override
    {
      for (const auto& str : strings)
      {
        std::cout << "Strings: " << str << std::endl;
      }
    }

    MetaBuilder& GetMetaBuilder() override
    {
      return *this;
    }

    void SetBlockSize(uint_t minimal, uint_t maximal) override
    {
      std::cout << "BlockSize: " << minimal << "..." << maximal << std::endl;
    }

    void SetFrameSize(uint_t minimal, uint_t maximal) override
    {
      std::cout << "FrameSize: " << minimal << "..." << maximal << std::endl;
    }

    void SetStreamParameters(uint_t sampleRate, uint_t channels, uint_t bitsPerSample) override
    {
      std::cout << "Frequency: " << sampleRate << std::endl
                << "Channels: " << channels << std::endl
                << "Bits: " << bitsPerSample << std::endl;
      UnpackedSize = channels * bitsPerSample / 8;
    }

    void SetTotalSamples(uint64_t count) override
    {
      std::cout << "TotalSamples: " << count << std::endl;
      UnpackedSize *= count;
    }

    void AddFrame(std::size_t offset) override
    {
      std::cout << Strings::Format("Frame: @{0}(0x{0:08x})\n", offset);
      ++FramesCount;
    }

    uint64_t GetUnpackedSize() const
    {
      return UnpackedSize;
    }

    uint_t GetFramesCount() const
    {
      return FramesCount;
    }

  private:
    uint64_t UnpackedSize = 0;
    uint_t FramesCount = 0;
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
    std::unique_ptr<Binary::Dump> rawData(new Binary::Dump());
    Test::OpenFile(argv[1], *rawData);
    const Binary::Container::Ptr data = Binary::CreateContainer(std::move(rawData));
    FlacBuilder builder;
    if (const auto result = Formats::Chiptune::Flac::Parse(*data, builder))
    {
      std::cout << "Frames: " << builder.GetFramesCount() << std::endl;
      std::cout << result->Size() << " bytes total" << std::endl;
      std::cout << "Compression ratio: " << 100 * result->Size() / builder.GetUnpackedSize() << "%\n";
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
