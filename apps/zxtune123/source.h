/*
Abstract:
  Source data provider declaration and related functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/
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

typedef boost::function<void(const ModuleItem&)> OnItemCallback;

Error ProcessModuleItems(const StringArray& files, const Parameters::Map& params, const ZXTune::DetectParameters::FilterFunc& filter,
                    const OnItemCallback& callback);
                    
                    
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
  virtual ~SourceComponent() {}
  // commandline-related part
  virtual const boost::program_options::options_description& GetOptionsDescription() const = 0;
  // throw
  virtual void Initialize() = 0;
  virtual void ProcessItems(const OnItemCallback& callback) = 0;
  
  static std::auto_ptr<SourceComponent> Create(Parameters::Map& globalParams);
};

#endif //ZXTUNE123_SOURCE_H_DEFINED
