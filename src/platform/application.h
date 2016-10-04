/**
* 
* @file
*
* @brief  Application interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//std includes
#include <iostream>
#include <memory>

// standart stream
extern std::basic_ostream<Char>& StdOut;

namespace Platform
{
  // declaration of app class. Implementation and factory should be implemented
  class Application
  {
  public:
    virtual ~Application() {}

    virtual int Run(int argc, const char* argv[]) = 0;

    static std::unique_ptr<Application> Create();
  };
}
