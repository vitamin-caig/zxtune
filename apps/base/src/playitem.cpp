/*
Abstract:
  Playitem support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "apps/base/playitem.h"
//library includes
#include <core/module_attrs.h>
#include <io/fs_tools.h>
#include <io/provider.h>
//common includes
#include <error_tools.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;

  class PathPropertiesAccessor : public Parameters::Accessor
  {
  public:
    PathPropertiesAccessor(const String& uri, const String& path, const String& subpath)
      : Uri(uri)
      , Path(path)
      , Filename(ZXTune::IO::ExtractLastPathComponent(Path, Dir))
      , Subpath(subpath)
    {
    }

    virtual bool FindValue(const Parameters::NameType& /*name*/, Parameters::IntType& /*val*/) const
    {
      return false;
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      if (name == Module::ATTR_SUBPATH)
      {
        val = Subpath;
        return true;
      }
      else if (name == Module::ATTR_FILENAME)
      {
        val = Filename;
        return true;
      }
      else if (name == Module::ATTR_PATH)
      {
        val = Path;
        return true;
      }
      else if (name == Module::ATTR_FULLPATH)
      {
        val = Uri;
        return true;
      }
      return false;
    }

    virtual bool FindValue(const Parameters::NameType& /*name*/, Parameters::DataType& /*val*/) const
    {
      return false;
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      visitor.SetValue(Module::ATTR_SUBPATH, Subpath);
      visitor.SetValue(Module::ATTR_FILENAME, Filename);
      visitor.SetValue(Module::ATTR_PATH, Path);
      visitor.SetValue(Module::ATTR_FULLPATH, Uri);
    }
  private:
    const String Uri;
    const String Path;
    String Dir;//before filename
    const String Filename;
    const String Subpath;
  };
}

Parameters::Accessor::Ptr CreatePathProperties(const String& path, const String& subpath)
{
  String fullPath;
  if (subpath.empty() || ZXTune::IO::CombineUri(path, subpath, fullPath))
  {
    return boost::make_shared<PathPropertiesAccessor>(path, path, String());
  }
  else
  {
    return boost::make_shared<PathPropertiesAccessor>(fullPath, path, subpath);
  }
}

Parameters::Accessor::Ptr CreatePathProperties(const String& fullpath)
{
  String path, subpath;
  if (ZXTune::IO::SplitUri(fullpath, path, subpath))
  {
    return boost::make_shared<PathPropertiesAccessor>(fullpath, String(), String());
  }
  else
  {
    return boost::make_shared<PathPropertiesAccessor>(fullpath, path, subpath);
  }
}
