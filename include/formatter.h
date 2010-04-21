/**
*
* @file     formatter.h
* @brief    Formatter type definition
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __FORMATTER_TYPE_H_DEFINED__
#define __FORMATTER_TYPE_H_DEFINED__

//common includes
#include <types.h>
//boost includes
#include <boost/format.hpp>

//! @brief Formatter type
//! @code
//! const String& txt = (Formatter(FORMAT_STRING) % param1 % param2).str();
//! @endcode
//! <A HREF="http://www.boost.org/doc/libs/1_41_0/libs/format/doc/format.html#syntax">Syntax of format string</A>
typedef boost::basic_format<Char> Formatter;

//! @brief Safe formatter type - does not throw exceptions if format string does not correspond to parameters count
class SafeFormatter : public Formatter
{
public:
  //use template ctor for common purposes- const Char* or const String& input types
  template<class T>
  SafeFormatter(const T str)
    : Formatter(str)
  {
    using namespace boost::io;
    exceptions(all_error_bits ^ (too_many_args_bit | too_few_args_bit));
  }
};

//! @brief Format time
//! @param timeInFrames frames quantity
//! @param frameDurationMicrosec frame duration in microseconds
//! @return Formatted time
String FormatTime(uint_t timeInFrames, uint_t frameDurationMicrosec);

#endif //__FORMATTER_TYPE_H_DEFINED__
