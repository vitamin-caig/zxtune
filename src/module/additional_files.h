/**
 *
 * @file
 *
 * @brief  Module::Holder extension for additional files operating
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"
#include "strings/array.h"

#include "string_view.h"

namespace Module
{
  class AdditionalFiles
  {
  public:
    virtual Strings::Array Enumerate() const = 0;
    virtual void Resolve(StringView name, Binary::Container::Ptr data) = 0;
  };
}  // namespace Module
