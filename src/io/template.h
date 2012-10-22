/**
*
* @file      io/template.h
* @brief     IO objects identifiers template support
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef IO_TEMPLATE_H_DEFINED
#define IO_TEMPLATE_H_DEFINED

//library includes
#include <strings/template.h>

namespace IO
{
  Strings::Template::Ptr CreateFilenameTemplate(const String& notation);
}

#endif //IO_TEMPLATE_H_DEFINED
