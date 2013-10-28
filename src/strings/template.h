/**
*
* @file     template.h
* @brief    %String template working
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef STRINGS_TEMPLATE_H_DEFINED
#define STRINGS_TEMPLATE_H_DEFINED

//common includes
#include <types.h>
//std includes
#include <memory>

namespace Strings
{
  //! @brief Interface optimized for fast template instantiation with different parameters
  class Template
  {
  public:
    static const Char FIELD_START;
    static const Char FIELD_END;

    //! @brief Pointer type
    typedef std::auto_ptr<const Template> Ptr;
    //! @brief Virtual destructor
    virtual ~Template() {}
    //! @brief Performing instantiation
    virtual String Instantiate(const class FieldsSource& source) const = 0;

    //! @brief Factory
    static Ptr Create(const String& templ);

    //! @param templ Input string
    //! @param source Fields provider
    //! @param beginMark Placeholders' start marker
    //! @param endMark Placeholders' end marker
    static String Instantiate(const String& templ, const FieldsSource& source);
  };
}

#endif //STRINGS_TEMPLATE_H_DEFINED
