/**
 *
 * @file
 *
 * @brief  Scanning utility
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <binary/format/expression.h>

#include <io/api.h>
#include <parameters/container.h>
#include <tools/progress_callback.h>

#include <error_tools.h>
#include <types.h>

#include <iostream>

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    std::cout << *argv << " <filename> <pattern>" << std::endl;
    return 0;
  }
  try
  {
    const auto format = Binary::FormatDSL::Expression::Parse(argv[2]);
    const std::string filename = argv[1];
    const Parameters::Accessor::Ptr params = Parameters::Container::Create();
    const Binary::Container::Ptr data = IO::OpenData(filename, *params, Log::ProgressCallback::Stub());

    if (format->StartOffset() + format->Predicates().size() > data->Size())
    {
      std::cout << "File too short" << std::endl;
      return 0;
    }
    const auto* rawData = static_cast<const uint8_t*>(data->Start());
    auto inPos = format->StartOffset();
    for (const auto& pred : format->Predicates())
    {
      if (!pred->Match(rawData[inPos]))
      {
        std::cout << "Mismatch at position " << inPos << std::endl;
        return 0;
      }
      ++inPos;
    }
    std::cout << "Matched" << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
  }
  catch (const Error& e)
  {
    std::cout << e.ToString() << std::endl;
  }
  return 0;
}
