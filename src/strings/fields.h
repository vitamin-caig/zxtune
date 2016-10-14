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

//common includes
#include <types.h>
//library includes
#include <strings/template.h>

namespace Strings
{
  //! @brief Fields provider interface
  class FieldsSource
  {
  public:
    virtual ~FieldsSource() {}

    virtual String GetFieldValue(const String& fieldName) const = 0;
  };

  //Base implementations

  // Skip all the fields
  class SkipFieldsSource : public FieldsSource
  {
  public:
    String GetFieldValue(const String& /*fieldName*/) const override
    {
      return String();
    }
  };

  // Kepp all the fields
  class KeepFieldsSource : public FieldsSource
  {
  public:
    String GetFieldValue(const String& fieldName) const override
    {
      String res(1, Template::FIELD_START);
      res += fieldName;
      res += Template::FIELD_END;
      return res;
    }
  };

  // Fill all the fields with spaces
  class FillFieldsSource : public FieldsSource
  {
  public:
    String GetFieldValue(const String& fieldName) const override
    {
      return String(fieldName.size() + 2, ' ');
    }
  };
}
