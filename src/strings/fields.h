/**
 *
 * @file
 *
 * @brief  Fields source and some basic adapters
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
#include <types.h>
// library includes
#include <strings/template.h>

namespace Strings
{
  //! @brief Fields provider interface
  class FieldsSource
  {
  public:
    virtual ~FieldsSource() = default;

    virtual String GetFieldValue(StringView fieldName) const = 0;
  };

  // Base implementations

  // Skip all the fields
  class SkipFieldsSource : public FieldsSource
  {
  public:
    String GetFieldValue(StringView /*fieldName*/) const override
    {
      return {};
    }
  };

  // Kepp all the fields
  class KeepFieldsSource : public FieldsSource
  {
  public:
    String GetFieldValue(StringView fieldName) const override
    {
      String res;
      res.reserve(fieldName.size() + 2);
      res += Template::FIELD_START;
      res.append(fieldName);
      res += Template::FIELD_END;
      return res;
    }
  };

  // Fill all the fields with spaces
  class FillFieldsSource : public FieldsSource
  {
  public:
    String GetFieldValue(StringView fieldName) const override
    {
      return String(fieldName.size() + 2, ' ');
    }
  };
}  // namespace Strings
