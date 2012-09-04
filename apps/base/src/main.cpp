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
#include <error.h>
//library includes
#include <l10n/api.h>
#include <l10n/control.h>
//std includes
#include <locale>
//text includes
#include "../text/base_text.h"

#ifdef UNICODE
std::basic_ostream<Char>& StdOut = std::wcout;
#else 
std::basic_ostream<Char>& StdOut = std::cout;
#endif

int main(int argc, char* argv[])
{
  try
  {
    std::locale::global(std::locale(""));
    std::auto_ptr<Application> app(Application::Create());
    return app->Run(argc, argv);
  }
  catch (const Error& e)
  {
    StdOut << Error::ToString(e) << std::endl;
    return 1;
  }
}
