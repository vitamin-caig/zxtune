#include <error_tools.h>
#include <template_parameters.h>
#include <core/module_detect.h>
#include <core/module_holder.h>
#include <io/provider.h>
#include <iostream>

namespace
{
  using namespace ZXTune;

  Module::Holder::Ptr OpenModuleByPath(const String& fullPath)
  {
    const Parameters::Accessor::Ptr emptyParams = Parameters::Container::Create();
    const String filename = fullPath;//TODO: split if required
    const String subpath = String();
    Binary::Container::Ptr data;
    ThrowIfError(IO::OpenData(filename, *emptyParams, Log::ProgressCallback::Stub(), data));
    Module::Holder::Ptr module;
    ThrowIfError(OpenModule(emptyParams, data, subpath, module));
    return module;
  }

  void ShowModuleInfo(const Module::Information& info)
  {
    std::cout <<
      "Positions: " << info.PositionsCount() << " (" << info.LoopPosition() << ')' << std::endl <<
      "Patterns: " << info.PatternsCount() << std::endl <<
      "Frames: " << info.FramesCount() << " (" << info.LoopFrame() << ')' << std::endl <<
      "Channels: " << info.LogicalChannels() << " (" << info.PhysicalChannels() << ')' << std::endl <<
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
   
    const Parameters::FieldsSourceAdapter<SkipFieldsSource> fields(props);
    std::cout << InstantiateTemplate(TEMPLATE, fields) << std::endl;
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
    return e.GetCode();
  }
}