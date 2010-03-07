/*
Abstract:
  Source data processing helper implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <apps/base/moduleitem.h>

#include <error.h>
#include <core/module_attrs.h>
#include <io/fs_tools.h>
#include <io/provider.h>

#include <boost/bind.hpp>

namespace
{
  Error FormModule(const String& path, const String& subpath, const ZXTune::Module::Holder::Ptr& module,
    const OnItemCallback& callback)
  {
    String uri;
    if (const Error& e = ZXTune::IO::CombineUri(path, subpath, uri))
    {
      return e;
    }
    ModuleItem item;
    item.Id = uri;
    item.Module = module;
    //add additional attributes
    Parameters::Map attrs;
    String tmp;
    attrs.insert(Parameters::Map::value_type(ZXTune::Module::ATTR_FILENAME,
      ZXTune::IO::ExtractLastPathComponent(path, tmp)));
    attrs.insert(Parameters::Map::value_type(ZXTune::Module::ATTR_PATH, path));
    attrs.insert(Parameters::Map::value_type(ZXTune::Module::ATTR_FULLPATH, uri));
    item.Module->ModifyCustomAttributes(attrs, false);
    try
    {
      callback(item);
      return Error();
    }
    catch (const Error& e)
    {
      return e;
    }
  }
}

Error ProcessModuleItems(const StringArray& files, const Parameters::Map& params, const ZXTune::DetectParameters::FilterFunc& filter,
  const ZXTune::DetectParameters::LogFunc& logger, const boost::function<void(const ModuleItem&)>& callback)
{
  //TODO: optimize
  for (StringArray::const_iterator it = files.begin(), lim = files.end(); it != lim; ++it)
  {
    ZXTune::IO::DataContainer::Ptr data;
    String subpath;
    if (const Error& e = ZXTune::IO::OpenData(*it, params, 0, data, subpath))
    {
      return e;
    }
    ZXTune::DetectParameters detectParams;
    detectParams.Filter = filter;
    detectParams.Logger = logger;
    detectParams.Callback = boost::bind(&FormModule, *it, _1, _2, callback);
    if (const Error& e = ZXTune::DetectModules(params, detectParams, data, subpath))
    {
      return e;
    }
  }
  return Error();
}
