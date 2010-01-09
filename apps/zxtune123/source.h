#ifndef ZXTUNE123_SOURCE_H_DEFINED
#define ZXTUNE123_SOURCE_H_DEFINED

#include <core/player.h>

#include <memory>

struct ModuleItem
{
  String Id;
  ZXTune::Module::Holder::Ptr Module;
  Parameters::Map Params;
};
typedef std::vector<ModuleItem> ModuleItemsArray;

Error GetModuleItems(const StringArray& files, const Parameters::Map& params, const ZXTune::DetectParameters::FilterFunc& filter,
                    ModuleItemsArray& result);

namespace boost
{
  namespace program_options
  {
    class options_description;
  }
}

class SourceComponent
{
public:
  // commandline-related part
  virtual const boost::program_options::options_description& GetOptionsDescription() const = 0;
  // throw
  virtual void Initialize() = 0;
  // functional part
  virtual void GetItems(ModuleItemsArray& result) = 0;
  
  static std::auto_ptr<SourceComponent> Create(Parameters::Map& globalParams);
};

#endif //ZXTUNE123_SOURCE_H_DEFINED
