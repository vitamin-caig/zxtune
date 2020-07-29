/**
*
* @file
*
* @brief  S98 dumper
*
* @author vitamin.caig@gmail.com
*
**/

#include "../../utils.h"
#include <formats/chiptune/multidevice/sound98.h>
#include <time/serialize.h>

namespace
{
  using namespace Formats::Chiptune;

  class S98Builder : public Sound98::Builder, public MetaBuilder
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

    void SetTimings(Time::Milliseconds total, Time::Milliseconds loop)
    {
      std::cout << "Duration: " << Time::ToString(total) << "\nLoop: " << Time::ToString(loop) << std::endl;
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
    S98Builder builder;
    if (const auto result = Formats::Chiptune::Sound98::Parse(*data, builder))
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
