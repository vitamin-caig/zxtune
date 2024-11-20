/**
 *
 * @file
 *
 * @brief  ModInfo utility
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/container_factories.h"
#include "core/service.h"
#include "io/api.h"
#include "module/track_information.h"
#include "parameters/container.h"
#include "parameters/template.h"
#include "time/serialize.h"
#include "tools/progress_callback.h"

#include "error_tools.h"
#include "string_view.h"

#include <iostream>

namespace
{
  Module::Holder::Ptr OpenModuleByPath(StringView fullPath)
  {
    const Parameters::Container::Ptr emptyParams = Parameters::Container::Create();
    const auto filename = fullPath;  // TODO: split if required
    const Binary::Container::Ptr data = IO::OpenData(filename, *emptyParams, Log::ProgressCallback::Stub());
    return ZXTune::Service::Create(emptyParams)->OpenModule(data, {}, Parameters::Container::Create());
  }

  void ShowModuleInfo(const Module::Information& info)
  {
    if (const auto* const trackInfo = dynamic_cast<const Module::TrackInformation*>(&info))
    {
      std::cout << "Positions: " << trackInfo->PositionsCount() << " (" << trackInfo->LoopPosition() << ')'
                << std::endl;
    }
    std::cout << "Duration: " << Time::ToString(info.Duration()) << " (loop " << Time::ToString(info.LoopDuration())
              << ')' << std::endl;
  }

  class PrintValuesVisitor : public Parameters::Visitor
  {
  public:
    void SetValue(Parameters::Identifier name, Parameters::IntType val) override
    {
      Write(name, Parameters::ConvertToString(val));
    }

    void SetValue(Parameters::Identifier name, StringView val) override
    {
      Write(name, Parameters::ConvertToString(val));
    }

    void SetValue(Parameters::Identifier name, Binary::View val) override
    {
      Write(name, Parameters::ConvertToString(val));
    }

  private:
    static void Write(StringView name, StringView value)
    {
      std::cout << name << ": " << value << std::endl;
    }
  };

  void ShowProperties(const Parameters::Accessor& props)
  {
    static PrintValuesVisitor PRINTER;
    props.Process(PRINTER);
  }

  void ShowModuleProperties(const Module::Holder& module)
  {
    ShowProperties(*module.GetModuleProperties());
    ShowModuleInfo(*module.GetModuleInformation());
  }
}  // namespace

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    return 0;
  }
  try
  {
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