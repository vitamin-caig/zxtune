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

#ifdef UNICODE
std::basic_ostream<Char>& StdOut = std::wcout;
#else 
std::basic_ostream<Char>& StdOut = std::cout;
#endif

//fix for new boost versions
namespace boost
{
  void tss_cleanup_implemented() { }
}

int main(int argc, char* argv[])
{
  std::auto_ptr<Application> app(Application::Create());
  return app->Run(argc, argv);
}
