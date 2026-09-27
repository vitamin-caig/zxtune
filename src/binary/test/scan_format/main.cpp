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
#include "time/timer.h"
#include "tools/progress_callback.h"

#include "error_tools.h"
#include "types.h"

#include <iostream>

namespace
{
  class ScanSpeed
  {
  public:
    explicit ScanSpeed(std::size_t total)
      : Total(total)
    {}

    void Report(std::size_t pos) const
    {
      const auto elapsed = Start.Elapsed();
      const auto speed = pos * elapsed.PER_SECOND / (elapsed ? elapsed.Get() : 1);
      std::cout << (pos != Total ? "Matched at " : "Finished scanning ") << pos << ". Speed " << double(speed) / 1048576
                << "Mb/s" << std::endl;
    }

  private:
    const std::size_t Total;
    const Time::Timer Start;
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
    const Binary::ScanningFormat::Ptr format = Binary::CreateScanningFormat(argv[2]);
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
