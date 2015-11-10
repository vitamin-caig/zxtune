/**
* 
* @file
*
* @brief  ModInfo utility
*
* @author vitamin.caig@gmail.com
*
**/

#include <error_tools.h>
#include <progress_callback.h>
#include <binary/container_factories.h>
#include <core/module_open.h>
#include <io/api.h>
#include <iostream>
#include <parameters/container.h>
#include <parameters/template.h>

namespace
{
  using namespace ZXTune;

  Module::Holder::Ptr OpenModuleByPath(const String& fullPath)
  {
    const Parameters::Container::Ptr emptyParams = Parameters::Container::Create();
    const String filename = fullPath;//TODO: split if required
    const Binary::Container::Ptr data = IO::OpenData(filename, *emptyParams, Log::ProgressCallback::Stub());
    const DataLocation::Ptr dataLocation = CreateLocation(data);
    return Module::Open(*emptyParams, dataLocation);
  }

  void ShowModuleInfo(const Module::Information& info)
  {
    std::cout <<
      "Positions: " << info.PositionsCount() << " (" << info.LoopPosition() << ')' << std::endl <<
      "Patterns: " << info.PatternsCount() << std::endl <<
      "Frames: " << info.FramesCount() << " (" << info.LoopFrame() << ')' << std::endl <<
      "Channels: " << info.ChannelsCount() << std::endl <<
      "Initial tempo: " << info.Tempo() << std::endl;
  }

  void ShowProperties(const Parameters::Accessor& props)
  {
    static const char TEMPLATE[] =
      "Title: [Title]\n"
      "Author: [Author]\n"
      "Program: [Program]\n"
      "Type: [Type]\n"
      "Size: [Size]\n"
      "[Comment]\n"
    ;
   
    const Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource> fields(props);
    std::cout << Strings::Template::Instantiate(TEMPLATE, fields) << std::endl;
  }

  void ShowModuleProperties(const Module::Holder& module)
  {
    ShowProperties(*module.GetModuleProperties());
    ShowModuleInfo(*module.GetModuleInformation());
  }
}

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    return 0;
  }
  try
  {
    using namespace ZXTune;

    const Module::Holder::Ptr module = OpenModuleByPath(argv[1]);
    ShowModuleProperties(*module);
    return 0;
  }
  catch (const Error& e)
  {
    std::cout << e.ToString() << std::endl;
    return -1;
  }
}