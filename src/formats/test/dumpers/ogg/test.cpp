/**
 *
 * @file
 *
 * @brief  OGG dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../../utils.h"
#include <formats/chiptune/music/oggvorbis.h>
#include <strings/format.h>

namespace
{
  using namespace Formats::Chiptune;

  class OggBuilder
    : public OggVorbis::Builder
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

    void SetComment(StringView comment) override
    {
      std::cout << "Comment: " << comment << std::endl;
    }

    MetaBuilder& GetMetaBuilder() override
    {
      return *this;
    }

    void SetStreamId(uint32_t id) override
    {
      std::cout << "StreamId: " << id << std::endl;
    }

    void AddUnknownPacket(Binary::View data) override
    {
      std::cout << "Unknown data for " << data.Size() << " bytes" << std::endl;
    }

    void SetProperties(uint_t channels, uint_t frequency, uint_t blockSizeLo, uint_t blockSizeHi) override
    {
      std::cout << "Channels: " << channels << std::endl
                << "Frequency: " << frequency << std::endl
                << "BlockSizeLo: " << blockSizeLo << std::endl
                << "BlockSizeHi: " << blockSizeHi << std::endl;
    }

    void SetSetup(Binary::View data) override
    {
      std::cout << "Setup: " << data.Size() << " bytes" << std::endl;
    }

    void AddFrame(std::size_t offset, uint64_t positionInFrames, Binary::View data) override
    {
      std::cout << Strings::Format("Frame: @{0} (0x{0:08x}) at frame {1}, {2} bytes\n", offset, positionInFrames,
                                   data.Size());
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
    OggBuilder builder;
    if (const auto result = Formats::Chiptune::OggVorbis::Parse(*data, builder))
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
