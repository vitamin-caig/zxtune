/**
 *
 * @file
 *
 * @brief  Module factory interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <binary/container.h>
#include <module/holder.h>
#include <parameters/container.h>

namespace Module
{
  class Factory
  {
  public:
    typedef std::shared_ptr<const Factory> Ptr;
    virtual ~Factory() = default;

    virtual Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& data,
                                     Parameters::Container::Ptr properties) const = 0;
  };
}  // namespace Module
