/*
Abstract:
  Application interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef BASE_APP_H_DEFINED
#define BASE_APP_H_DEFINED

//common includes
#include <types.h>
//std includes
#include <iostream>
#include <memory>

// declaration of module id
extern const std::string THIS_MODULE;

// standart stream
extern std::basic_ostream<Char>& StdOut;

// declaration of app class. Implementation and factory should be implemented
class Application
{
public:
  virtual ~Application() {}

  virtual int Run(int argc, char* argv[]) = 0;

  static std::auto_ptr<Application> Create();
};

#endif //BASE_APP_H_DEFINED
