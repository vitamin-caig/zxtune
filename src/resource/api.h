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

#include <binary/container.h>

#include <string_view.h>
#include <types.h>

namespace Resource
{
  Binary::Container::Ptr Load(StringView name);

  class Visitor
  {
  public:
    virtual ~Visitor() = default;

    virtual void OnResource(StringView name) = 0;
  };

  void Enumerate(Visitor& visitor);
}  // namespace Resource
