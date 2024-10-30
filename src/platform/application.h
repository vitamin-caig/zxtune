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

// common includes
#include <types.h>
// library includes
#include <strings/array.h>
// std includes
#include <iostream>
#include <memory>

// standart stream
extern std::ostream& StdOut;

namespace Platform
{
  // declaration of app class. Implementation and factory should be implemented
  class Application
  {
  public:
    virtual ~Application() = default;

    virtual int Run(Strings::Array args) = 0;

    static std::unique_ptr<Application> Create();
  };
}  // namespace Platform
