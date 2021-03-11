/**
 *
 * @file
 *
 * @brief  Path properties support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "path.h"
// common includes
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <io/api.h>
#include <module/attributes.h>
#include <parameters/visitor.h>

namespace Module
{
  class UnresolvedPathPropertiesAccessor : public Parameters::Accessor
  {
  public:
    explicit UnresolvedPathPropertiesAccessor(String uri)
      : Uri(std::move(uri))
    {}

    uint_t Version() const override
    {
      return 1;
    }

    bool FindValue(const Parameters::NameType& /*name*/, Parameters::IntType& /*val*/) const override
    {
      return false;
    }

    bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const override
    {
      if (name == ATTR_FULLPATH)
      {
        val = Uri;
        return true;
      }
      return false;
    }

    bool FindValue(const Parameters::NameType& /*name*/, Parameters::DataType& /*val*/) const override
    {
      return false;
    }

    void Process(Parameters::Visitor& visitor) const override
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
      : Id(std::move(id))
    {}

    uint_t Version() const override
    {
      return 1;
    }

    bool FindValue(const Parameters::NameType& /*name*/, Parameters::IntType& /*val*/) const override
    {
      return false;
    }

    bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const override
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

    bool FindValue(const Parameters::NameType& /*name*/, Parameters::DataType& /*val*/) const override
    {
      return false;
    }

    void Process(Parameters::Visitor& visitor) const override
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
}  // namespace Module

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
}  // namespace Module
