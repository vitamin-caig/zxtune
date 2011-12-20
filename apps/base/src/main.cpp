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
//std includes
#include <locale>
//text includes
#include "../text/base_text.h"

#ifdef UNICODE
std::basic_ostream<Char>& StdOut = std::wcout;
#else 
std::basic_ostream<Char>& StdOut = std::cout;
#endif

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
