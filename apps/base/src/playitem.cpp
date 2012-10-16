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
#include <io/api.h>
//common includes
#include <error_tools.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  class UnresolvedPathPropertiesAccessor : public Parameters::Accessor
  {
  public:
    explicit UnresolvedPathPropertiesAccessor(const String& uri)
      : Uri(uri)
    {
    }

    virtual bool FindValue(const Parameters::NameType& /*name*/, Parameters::IntType& /*val*/) const
    {
      return false;
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      if (name == ZXTune::Module::ATTR_FULLPATH)
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
      visitor.SetValue(ZXTune::Module::ATTR_FULLPATH, Uri);
    }
  private:
    const String Uri;
  };

  class PathPropertiesAccessor : public Parameters::Accessor
  {
  public:
    explicit PathPropertiesAccessor(IO::Identifier::Ptr id)
      : Id(id)
    {
    }

    virtual bool FindValue(const Parameters::NameType& /*name*/, Parameters::IntType& /*val*/) const
    {
      return false;
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      if (name == ZXTune::Module::ATTR_SUBPATH)
      {
        val = Id->Subpath();
        return true;
      }
      else if (name == ZXTune::Module::ATTR_FILENAME)
      {
        val = Id->Filename();
        return true;
      }
      else if (name == ZXTune::Module::ATTR_EXTENSION)
      {
        val = Id->Extension();
        return true;
      }
      else if (name == ZXTune::Module::ATTR_PATH)
      {
        val = Id->Path();
        return true;
      }
      else if (name == ZXTune::Module::ATTR_FULLPATH)
      {
        val = Id->Full();
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
      visitor.SetValue(ZXTune::Module::ATTR_SUBPATH, Id->Subpath());
      visitor.SetValue(ZXTune::Module::ATTR_FILENAME, Id->Filename());
      visitor.SetValue(ZXTune::Module::ATTR_EXTENSION, Id->Extension());
      visitor.SetValue(ZXTune::Module::ATTR_PATH, Id->Path());
      visitor.SetValue(ZXTune::Module::ATTR_FULLPATH, Id->Full());
    }
  private:
    const IO::Identifier::Ptr Id;
  };
}

Parameters::Accessor::Ptr CreatePathProperties(const String& path, const String& subpath)
{
  try
  {
    const IO::Identifier::Ptr id = IO::ResolveUri(path);
    const IO::Identifier::Ptr subId = id->WithSubpath(subpath);
    return CreatePathProperties(subId);
  }
  catch (const Error&)
  {
    //formally impossible situation
    return boost::make_shared<UnresolvedPathPropertiesAccessor>(path);
  }
}

Parameters::Accessor::Ptr CreatePathProperties(const String& fullpath)
{
  try
  {
    const IO::Identifier::Ptr id = IO::ResolveUri(fullpath);
    return CreatePathProperties(id);
  }
  catch (const Error&)
  {
    return boost::make_shared<UnresolvedPathPropertiesAccessor>(fullpath);
  }
}

Parameters::Accessor::Ptr CreatePathProperties(IO::Identifier::Ptr id)
{
  return boost::make_shared<PathPropertiesAccessor>(id);
}
