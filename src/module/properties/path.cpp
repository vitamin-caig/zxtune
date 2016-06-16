/**
* 
* @file
*
* @brief  Path properties support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "path.h"
//common includes
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <io/api.h>
#include <module/attributes.h>
#include <parameters/visitor.h>

namespace Module
{
  class UnresolvedPathPropertiesAccessor : public Parameters::Accessor
  {
  public:
    explicit UnresolvedPathPropertiesAccessor(const String& uri)
      : Uri(uri)
    {
    }

    virtual uint_t Version() const
    {
      return 1;
    }

    virtual bool FindValue(const Parameters::NameType& /*name*/, Parameters::IntType& /*val*/) const
    {
      return false;
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      if (name == ATTR_FULLPATH)
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
      visitor.SetValue(ATTR_FULLPATH, Uri);
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

    virtual uint_t Version() const
    {
      return 1;
    }

    virtual bool FindValue(const Parameters::NameType& /*name*/, Parameters::IntType& /*val*/) const
    {
      return false;
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      if (name == ATTR_SUBPATH)
      {
        val = Id->Subpath();
        return true;
      }
      else if (name == ATTR_FILENAME)
      {
        val = Id->Filename();
        return true;
      }
      else if (name == ATTR_EXTENSION)
      {
        val = Id->Extension();
        return true;
      }
      else if (name == ATTR_PATH)
      {
        val = Id->Path();
        return true;
      }
      else if (name == ATTR_FULLPATH)
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
      visitor.SetValue(ATTR_SUBPATH, Id->Subpath());
      visitor.SetValue(ATTR_FILENAME, Id->Filename());
      visitor.SetValue(ATTR_EXTENSION, Id->Extension());
      visitor.SetValue(ATTR_PATH, Id->Path());
      visitor.SetValue(ATTR_FULLPATH, Id->Full());
    }
  private:
    const IO::Identifier::Ptr Id;
  };
}

namespace Module
{
  Parameters::Accessor::Ptr CreatePathProperties(const String& fullpath)
  {
    try
    {
      const IO::Identifier::Ptr id = IO::ResolveUri(fullpath);
      return CreatePathProperties(id);
    }
    catch (const Error&)
    {
      return MakePtr<UnresolvedPathPropertiesAccessor>(fullpath);
    }
  }

  Parameters::Accessor::Ptr CreatePathProperties(IO::Identifier::Ptr id)
  {
    return MakePtr<PathPropertiesAccessor>(id);
  }
}
