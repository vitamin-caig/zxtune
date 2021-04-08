/**
 *
 * @file
 *
 * @brief  ModInfo utility
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <binary/container_factories.h>
#include <core/module_open.h>
#include <error_tools.h>
#include <io/api.h>
#include <iostream>
#include <module/track_information.h>
#include <parameters/container.h>
#include <parameters/template.h>
#include <progress_callback.h>
#include <time/serialize.h>

namespace
{
  Module::Holder::Ptr OpenModuleByPath(const String& fullPath)
  {
    const Parameters::Container::Ptr emptyParams = Parameters::Container::Create();
    const String filename = fullPath;  // TODO: split if required
    const Binary::Container::Ptr data = IO::OpenData(filename, *emptyParams, Log::ProgressCallback::Stub());
    return Module::Open(*emptyParams, *data, Parameters::Container::Create());
  }

  void ShowModuleInfo(const Module::Information& info)
  {
    if (const auto trackInfo = dynamic_cast<const Module::TrackInformation*>(&info))
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
    void SetValue(const Parameters::NameType& name, Parameters::IntType val) override
    {
      Write(name, Parameters::ConvertToString(val));
    }

    virtual void SetValue(const Parameters::NameType& name, StringView val) override
    {
      Write(name, Parameters::ConvertToString(val));
    }

    virtual void SetValue(const Parameters::NameType& name, Binary::View val) override
    {
      Write(name, Parameters::ConvertToString(val));
    }

  private:
    static void Write(const Parameters::NameType& name, const String& value)
    {
      std::cout << name.Name() << ": " << value << std::endl;
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