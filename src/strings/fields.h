/**
*
* @file
* @brief    Fields source and some basic adapters
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef STRINGS_FIELDS_H_DEFINED
#define STRINGS_FIELDS_H_DEFINED

//common includes
#include <types.h>

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
    virtual String GetFieldValue(const String& /*fieldName*/) const
    {
      return String();
    }
  };

  // Kepp all the fields
  template<Char MarkBegin = '[', Char MarkEnd = ']'>
  class KeepFieldsSource : public FieldsSource
  {
  public:
    virtual String GetFieldValue(const String& fieldName) const
    {
      String res(1, MarkBegin);
      res += fieldName;
      res += MarkEnd;
      return res;
    }
  };

  // Fill all the fields with spaces
  class FillFieldsSource : public FieldsSource
  {
  public:
    virtual String GetFieldValue(const String& fieldName) const
    {
      return String(fieldName.size() + 2, ' ');
    }
  };
}

#endif //STRINGS_FIELDS_H_DEFINED
