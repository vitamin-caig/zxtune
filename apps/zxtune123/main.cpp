/*
Abstract:
  Main program function

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/
#include "app.h"

#ifdef UNICODE
std::basic_ostream<Char>& StdOut = std::wcout;
#else 
std::basic_ostream<Char>& StdOut = std::cout;
#endif

int main(int argc, char* argv[])
{
  std::auto_ptr<Application> app(Application::Create());
  return app->Run(argc, argv);
}
