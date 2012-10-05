/*
Abstract:
  Version api implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <apps/version/api.h>
//library includes
#include <strings/format.h>
//text includes
#include "../text/text.h"

namespace
{
// version definition-related
#ifndef ZXTUNE_VERSION
#define ZXTUNE_VERSION develop
#endif
#ifndef BUILD_PLATFORM
#define BUILD_PLATFORM unknown
#endif
#ifndef BUILD_ARCH
#define BUILD_ARCH unknown
#endif

#define TOSTRING(a) #a
#define STR(a) TOSTRING(a)

  const std::string PROGRAM_VERSION(STR(ZXTUNE_VERSION));
  const std::string PROGRAM_DATE(__DATE__);
  const std::string PROGRAM_PLATFORM(STR(BUILD_PLATFORM));
  const std::string PROGRAM_ARCH(STR(BUILD_ARCH));
}

namespace Text
{
  extern const Char PROGRAM_NAME[];
}

String GetProgramTitle()
{
  return Text::PROGRAM_NAME;
}

String GetProgramVersionString()
{
  return Strings::Format(Text::PROGRAM_VERSION_STRING,
    GetProgramTitle(),
    FromStdString(PROGRAM_VERSION),
    FromStdString(PROGRAM_DATE),
    FromStdString(PROGRAM_PLATFORM),
    FromStdString(PROGRAM_ARCH));
}
