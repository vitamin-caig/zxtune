#include <contract.h>
#include <error.h>
#include <progress_callback.h>
#include <binary/container_factories.h>
#include <core/module_detect.h>
#include <io/api.h>
#include <module/attributes.h>
#include <parameters/container.h>

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
      const auto props = module->GetModuleProperties();
      String str;
      Require(props->FindValue(Module::ATTR_TYPE, str));
      ++Types[str];
    }
    
    void Report()
    {
      std::cout << Modules.size() << " modules total\n";
      std::cout << "Types:\n";
      for (const auto& type : Types)
      {
        std::cout << type.first << ": " << type.second << '\n';
      }
      std::cout << std::flush;
    }
  private:
    std::list<Module::Holder::Ptr> Modules;
    std::map<String, uint_t> Types;
  };
  
  class DetectCallbackAdapter : public Module::DetectCallback
  {
  public:
    explicit DetectCallbackAdapter(ModulesList& modules)
      : Modules(modules)
    {
    }
    
    void ProcessModule(ZXTune::DataLocation::Ptr /*location*/, ZXTune::Plugin::Ptr /*decoder*/, Module::Holder::Ptr holder) const override
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
    const auto location = ZXTune::CreateLocation(nocopyData);
    DetectCallbackAdapter cb(modules);
    Module::Detect(*emptyParams, location, cb);
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
