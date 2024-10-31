/**
 *
 * @file
 *
 * @brief  Core service interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <binary/container.h>
#include <core/module_detect.h>
#include <module/holder.h>
#include <parameters/container.h>

#include <string_view.h>

namespace ZXTune
{
  class Service
  {
  public:
    using Ptr = std::shared_ptr<const Service>;

    virtual ~Service() = default;

    // @throws Error if data cannot be resolved
    virtual Binary::Container::Ptr OpenData(Binary::Container::Ptr data, StringView subpath) const = 0;

    // @throws Error if no module found
    virtual Module::Holder::Ptr OpenModule(Binary::Container::Ptr data, StringView subpath,
                                           Parameters::Container::Ptr initialProperties) const = 0;

    virtual void DetectModules(Binary::Container::Ptr data, Module::DetectCallback& callback) const = 0;
    virtual void OpenModule(Binary::Container::Ptr data, StringView subpath,
                            Module::DetectCallback& callback) const = 0;

    static Ptr Create(Parameters::Accessor::Ptr parameters);
  };
}  // namespace ZXTune
