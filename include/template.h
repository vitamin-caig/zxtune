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
#include <string_helpers.h>
//std includes
#include <memory>

//! @brief %Parameters instantiating modes
enum InstantiateMode
{
  //! All nonexisting parameters' placeholders will be left
  KEEP_NONEXISTING,
  //! All nonexisting parameters' placeholders will be removed
  SKIP_NONEXISTING,
  //! All nonexisting parameters' placeholders will be changed by spaced
  FILL_NONEXISTING
};

//! @brief Substituting template string's placeholders with values
//! @param templ Input string
//! @param properties Values for substitution
//! @param mode See InstantiateMode
//! @param beginMark Placeholders' start marker
//! @param endMark Placeholders' end marker
//! @code
//! StringMap props;
//! props["name1"] = "value1";
//! props["name2"] = "value2";
//! const String& format("name1=[name1] name2=[name2] name3=[name3]");
//! //val1="name1=value1 name2=value2 name3=[name3]");
//! const String& val1 = InstantiateTemplate(format, KEEP_NONEXISTING);
//! //val2="name1=value1 name2=value2 name3=");
//! const String& val2 = InstantiateTemplate(format, SKIP_NONEXISTING);
//! //val3="name1=value1 name2=value2 name3=       ");
//! const String& val3 = InstantiateTemplate(format, FILL_NONEXISTING);
//! @endcode
String InstantiateTemplate(const String& templ, const StringMap& properties,
  InstantiateMode mode = KEEP_NONEXISTING, Char beginMark = '[', Char endMark = ']');

//! @brief Interface optimized for fast template instantiation with different parameters
class StringTemplate
{
public:
  //! @brief Pointer type
  typedef std::auto_ptr<StringTemplate> Ptr;
  //! @brief Virtual destructor
  virtual ~StringTemplate() {}
  //! @brief Performing instantiation
  virtual String Instantiate(const StringMap& properties) const = 0;
  //! @brief Factory
  static Ptr Create(const String& templ, InstantiateMode mode = KEEP_NONEXISTING, Char beginMark = '[', Char endMark = ']');
};

#endif //__TEMPLATE_H_DEFINED__
