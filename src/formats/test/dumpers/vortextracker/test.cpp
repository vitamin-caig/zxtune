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
  using namespace Formats::Chiptune::ProTracker3;

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
    const Formats::Chiptune::ProTracker3::ChiptuneBuilder::Ptr builder = Formats::Chiptune::ProTracker3::CreateVortexTracker2Builder();
    const std::string type(argv[1]);
    if (type == "txt")
    {
      Formats::Chiptune::ProTracker3::ParseVortexTracker2(*data, *builder);
    }
    else if (type == "pt3")
    {
      Formats::Chiptune::ProTracker3::ParseCompiled(*data, *builder);
    }
    const Binary::Data::Ptr result = builder->GetResult();
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
