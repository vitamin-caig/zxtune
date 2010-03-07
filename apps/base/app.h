/*
Abstract:
  Application interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef BASE_APP_H_DEFINED
#define BASE_APP_H_DEFINED

#include <char_type.h>

#include <iostream>
#include <memory>

extern const std::string THIS_MODULE;

// standart stream
extern std::basic_ostream<Char>& StdOut;

class Application
{
public:
  virtual ~Application() {}

  virtual int Run(int argc, char* argv[]) = 0;

  static std::auto_ptr<Application> Create();
};

#endif //BASE_APP_H_DEFINED
