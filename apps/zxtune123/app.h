/*
Abstract:
  Application interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/
#ifndef ZXTUNE123_APP_H_DEFINED
#define ZXTUNE123_APP_H_DEFINED

#include <char_type.h>

#include <iostream>
#include <memory>

// standart stream
extern std::basic_ostream<Char>& StdOut;

class Application
{
public:
  virtual ~Application() {}

  virtual int Run(int argc, char* argv[]) = 0;

  static std::auto_ptr<Application> Create();
};

#endif //ZXTUNE123_APP_H_DEFINED
