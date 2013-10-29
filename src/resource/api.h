/**
*
* @file
*
* @brief  Interface for working with embedded resources
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <binary/container.h>

namespace Resource
{
  Binary::Container::Ptr Load(const String& name);

  class Visitor
  {
  public:
    virtual ~Visitor() {}

    virtual void OnResource(const String& name) = 0;
  };

  void Enumerate(Visitor& visitor);
}
