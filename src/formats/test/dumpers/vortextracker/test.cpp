/**
 *
 * @file
 *
 * @brief  VortexTracker-based tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/protracker3.h"
#include "formats/test/utils.h"

namespace
{
  Formats::Chiptune::ProTracker3::Parser CreateParser(const std::string& type)
  {
    if (type == "txt")
    {
      return Formats::Chiptune::ProTracker3::VortexTracker2::Parse;
    }
    else if (type == "pt3")
    {
      return Formats::Chiptune::ProTracker3::Parse;
    }
    else
    {
      throw std::runtime_error("Invalid type " + type);
    }
  }
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
    const auto builder = Formats::Chiptune::ProTracker3::VortexTracker2::CreateBuilder();
    const std::string type(argv[1]);
    const auto parse = CreateParser(type);
    parse(*data, *builder);
    const auto result = builder->GetResult();
    const char* const start = static_cast<const char*>(result->Start());
    std::cout << std::string(start, start + result->Size());
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
