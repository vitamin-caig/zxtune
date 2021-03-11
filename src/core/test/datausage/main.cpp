#include <binary/container_factories.h>
#include <contract.h>
#include <core/module_detect.h>
#include <error.h>
#include <io/api.h>
#include <module/attributes.h>
#include <parameters/container.h>
#include <progress_callback.h>

#include <iostream>
#include <list>
#include <map>

namespace
{
  class ModulesList
  {
  public:
    void Add(Module::Holder::Ptr module)
    {
      Modules.push_back(module);
    }

    void Report()
    {
      std::cout << Modules.size() << " modules total\n";
      std::map<String, uint_t> types;
      for (const auto& module : Modules)
      {
        try
        {
          const auto props = module->GetModuleProperties();
          String str;
          Require(props->FindValue(Module::ATTR_TYPE, str));
          ++types[str];
        }
        catch (const Error& err)
        {
          ++types[err.GetText()];
        }
      }
      std::cout << "Types:\n";
      for (const auto& type : types)
      {
        std::cout << type.first << ": " << type.second << '\n';
      }
      std::cout << std::flush;
    }

  private:
    std::list<Module::Holder::Ptr> Modules;
  };

  class DetectCallbackAdapter : public Module::DetectCallback
  {
  public:
    explicit DetectCallbackAdapter(ModulesList& modules)
      : Modules(modules)
    {}

    void ProcessModule(ZXTune::DataLocation::Ptr /*location*/, ZXTune::Plugin::Ptr /*decoder*/,
                       Module::Holder::Ptr holder) const override
    {
      Modules.Add(holder);
    }

    virtual Log::ProgressCallback* GetProgress() const override
    {
      return nullptr;
    }

  private:
    ModulesList& Modules;
  };

  void Analyze(const String& path, ModulesList& modules)
  {
    const auto emptyParams = Parameters::Container::Create();
    const auto data = IO::OpenData(path, *emptyParams, Log::ProgressCallback::Stub());
    const auto nocopyData = Binary::CreateNonCopyContainer(data->Start(), data->Size());
    DetectCallbackAdapter cb(modules);
    Module::Detect(*emptyParams, nocopyData, cb);
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
    ModulesList modules;
    {
      for (int arg = 1; arg < argc; ++arg)
      {
        Analyze(argv[arg], modules);
      }
    }
    modules.Report();
    return 0;
  }
  catch (const Error& e)
  {
    std::cout << e.ToString() << std::endl;
    return -1;
  }
}
