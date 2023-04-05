/**
 *
 * @file
 *
 * @brief  VortexTracker-based tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "../../utils.h"
#include <formats/chiptune/aym/protracker3.h>

namespace
{
  Formats::Chiptune::ProTracker3::Decoder::Ptr CreateDecoder(const std::string& type)
  {
    if (type == "txt")
    {
      return Formats::Chiptune::ProTracker3::VortexTracker2::CreateDecoder();
    }
    else if (type == "pt3")
    {
      return Formats::Chiptune::ProTracker3::CreateDecoder();
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
    const auto decoder = CreateDecoder(type);
    decoder->Parse(*data, *builder);
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
