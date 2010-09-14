/**
*
* @file     template.h
* @brief    %String template working
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __TEMPLATE_H_DEFINED__
#define __TEMPLATE_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <memory>

//! @brief Fields provider interface
class FieldsSource
{
public:
  virtual ~FieldsSource() {}

  virtual String GetFieldValue(const String& fieldName) const = 0;
};

//! @param templ Input string
//! @param source Fields provider
//! @param beginMark Placeholders' start marker
//! @param endMark Placeholders' end marker
String InstantiateTemplate(const String& templ, const FieldsSource& source, Char beginMark = '[', Char endMark = ']');

//! @brief Interface optimized for fast template instantiation with different parameters
class StringTemplate
{
public:
  //! @brief Pointer type
  typedef std::auto_ptr<StringTemplate> Ptr;
  //! @brief Virtual destructor
  virtual ~StringTemplate() {}
  //! @brief Performing instantiation
  virtual String Instantiate(const FieldsSource& source) const = 0;
  //! @brief Factory
  static Ptr Create(const String& templ, Char beginMark = '[', Char endMark = ']');
};

#endif //__TEMPLATE_H_DEFINED__
