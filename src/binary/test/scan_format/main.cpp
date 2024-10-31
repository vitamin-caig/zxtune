/**
 *
 * @file
 *
 * @brief  Scanning utility
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/format_factories.h"
#include "io/api.h"
#include "parameters/container.h"
#include "tools/progress_callback.h"

#include "error_tools.h"
#include "types.h"

#include <ctime>
#include <iostream>

namespace
{
  class ScanSpeed
  {
  public:
    explicit ScanSpeed(std::size_t total)
      : Start(std::clock())
      , Total(total)
    {}

    void Report(std::size_t pos) const
    {
      const std::clock_t elapsed = std::clock() - Start;
      const std::size_t speed = pos * CLOCKS_PER_SEC / (elapsed ? elapsed : 1);
      std::cout << (pos != Total ? "Matched at " : "Finished scanning ") << pos << ". Speed " << double(speed) / 1048576
                << "Mb/s" << std::endl;
    }

  private:
    const std::clock_t Start;
    const std::size_t Total;
  };
}  // namespace

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    std::cout << *argv << " <filename> <pattern>" << std::endl;
    return 0;
  }
  try
  {
    const Binary::Format::Ptr format = Binary::CreateFormat(argv[2]);
    const std::string filename = argv[1];
    const Parameters::Accessor::Ptr params = Parameters::Container::Create();
    const Binary::Container::Ptr data = IO::OpenData(filename, *params, Log::ProgressCallback::Stub());

    if (format->Match(*data))
    {
      std::cout << "Matched" << std::endl;
    }
    else
    {
      const ScanSpeed speed(data->Size());
      std::size_t cursor = 0;
      while (const Binary::Data::Ptr subdata = data->GetSubcontainer(cursor, data->Size() - cursor))
      {
        const std::size_t offset = format->NextMatchOffset(*subdata);
        if (offset != subdata->Size())
        {
          cursor += offset;
          speed.Report(cursor);
        }
        else
        {
          speed.Report(cursor + offset);
          break;
        }
      }
    }
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
  }
  catch (const Error& e)
  {
    std::cout << e.ToString() << std::endl;
  }
}
