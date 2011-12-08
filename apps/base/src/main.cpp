/*
Abstract:
  Main program function

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <apps/base/app.h>
//common includes
#include <format.h>
//std includes
#include <locale>
//text includes
#include "../text/base_text.h"

#ifdef UNICODE
std::basic_ostream<Char>& StdOut = std::wcout;
#else 
std::basic_ostream<Char>& StdOut = std::cout;
#endif

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

//fix for new boost versions
#ifdef BOOST_THREAD_USE_LIB
namespace boost
{
  void tss_cleanup_implemented() { }
}
#endif

int main(int argc, char* argv[])
{
  std::locale::global(std::locale(""));
  std::auto_ptr<Application> app(Application::Create());
  return app->Run(argc, argv);
}
