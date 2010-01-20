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

#include <char_type.h>

#include <boost/format.hpp>

//! @brief Formatter type
//! @code
//! const String& txt = (Formatter(FORMAT_STRING) % param1 % param2).str();
//! @endcode
//! <A HREF="http://www.boost.org/doc/libs/1_41_0/libs/format/doc/format.html#syntax">Syntax of format string</A>
typedef boost::basic_format<Char> Formatter;

#endif //__FORMATTER_TYPE_H_DEFINED__
