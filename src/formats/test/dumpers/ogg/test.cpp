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

  class OggBuilder : public OggVorbis::Builder, public MetaBuilder
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
    
    void SetChannels(uint_t channels) override
    {
      std::cout << "Channels: " << channels << std::endl;
    }
    
    void SetFrequency(uint_t frequency) override
    {
      std::cout << "Frequency: " << frequency << std::endl;
    }
    
    void AddFrame(std::size_t offset, uint_t samplesCount) override
    {
      std::cout << Strings::Format("Frame: @%1%(0x%1$08x) %2% samples\n", offset, samplesCount);
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
