/**
 *
 * @file
 *
 * @brief  Path properties support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/properties/path.h"

#include "binary/data.h"
#include "io/api.h"
#include "module/attributes.h"
#include "parameters/identifier.h"
#include "parameters/types.h"
#include "parameters/visitor.h"

#include "error.h"
#include "make_ptr.h"
#include "string_type.h"
#include "string_view.h"
#include "types.h"

#include <memory>
#include <optional>
#include <utility>

namespace Module
{
  class UnresolvedPathPropertiesAccessor : public Parameters::Accessor
  {
  public:
    explicit UnresolvedPathPropertiesAccessor(StringView uri)
      : Uri(uri)
    {}

    uint_t Version() const override
    {
      return 1;
    }

    std::optional<Parameters::IntType> FindInteger(Parameters::Identifier /*name*/) const override
    {
      return std::nullopt;
    }

    std::optional<Parameters::StringType> FindString(Parameters::Identifier name) const override
    {
      if (name == ATTR_FULLPATH)
      {
        return Uri;
      }
      return std::nullopt;
    }

    Binary::Data::Ptr FindData(Parameters::Identifier /*name*/) const override
    {
      return {};
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

    std::optional<Parameters::IntType> FindInteger(Parameters::Identifier /*name*/) const override
    {
      return std::nullopt;
    }

    std::optional<Parameters::StringType> FindString(Parameters::Identifier name) const override
    {
      if (name == ATTR_SUBPATH)
      {
        return Id->Subpath();
      }
      else if (name == ATTR_FILENAME)
      {
        return Id->Filename();
      }
      else if (name == ATTR_EXTENSION)
      {
        return Id->Extension();
      }
      else if (name == ATTR_PATH)
      {
        return Id->Path();
      }
      else if (name == ATTR_FULLPATH)
      {
        return Id->Full();
      }
      return std::nullopt;
    }

    Binary::Data::Ptr FindData(Parameters::Identifier /*name*/) const override
    {
      return {};
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
  Parameters::Accessor::Ptr CreatePathProperties(StringView fullpath)
  {
    try
    {
      auto id = IO::ResolveUri(fullpath);
      return CreatePathProperties(std::move(id));
    }
    catch (const Error&)
    {
      return MakePtr<UnresolvedPathPropertiesAccessor>(fullpath);
    }
  }

  Parameters::Accessor::Ptr CreatePathProperties(IO::Identifier::Ptr id)
  {
    return MakePtr<PathPropertiesAccessor>(std::move(id));
  }
}  // namespace Module
