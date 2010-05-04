/*
Abstract:
  Source data processing helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef BASE_MODULEITEM_H_DEFINED
#define BASE_MODULEITEM_H_DEFINED

//library includes
#include <core/module_detect.h>
//std includes
#include <memory>

//detected module item
struct ModuleItem
{
  String Id;
  ZXTune::Module::Holder::Ptr Module;
  Parameters::Map Params;
};
typedef std::vector<ModuleItem> ModuleItemsArray;

typedef boost::function<bool(const ModuleItem&)> OnItemCallback;

//TODO: make class with cached context
Error ProcessModuleItems(const StringArray& files, const Parameters::Map& params, 
  const ZXTune::DetectParameters::FilterFunc& filter, const ZXTune::DetectParameters::LogFunc& logger,
  const OnItemCallback& callback);

#endif //BASE_MODULEITEM_H_DEFINED
