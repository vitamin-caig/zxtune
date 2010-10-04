/**
*
* @file     template_tools.h
* @brief    %String template working helpers
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __TEMPLATE_TOOLS_H_DEFINED__
#define __TEMPLATE_TOOLS_H_DEFINED__

//common includes
#include <template.h>

// Policies

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

#endif //__TEMPLATE_TOOLS_H_DEFINED__
