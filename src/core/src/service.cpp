/**
 *
 * @file
 *
 * @brief  Core service implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/src/location.h"
// common includes
#include <make_ptr.h>
// library includes
#include <core/module_detect.h>
#include <core/module_open.h>
#include <core/service.h>

namespace ZXTune
{
  class ServiceImpl : public Service
  {
  public:
    explicit ServiceImpl(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {}

    Binary::Container::Ptr OpenData(Binary::Container::Ptr data, const String& subpath) const override
    {
      return OpenLocation(std::move(data), subpath)->GetData();
    }

    Module::Holder::Ptr OpenModule(Binary::Container::Ptr data, const String& subpath,
                                   Parameters::Container::Ptr initialProperties) const override
    {
      return subpath.empty() ? Module::Open(*Params, *data, std::move(initialProperties))
                             : Module::Open(*Params, std::move(data), subpath, std::move(initialProperties));
    }

    void DetectModules(Binary::Container::Ptr data, Module::DetectCallback& callback) const override
    {
      return Module::Detect(*Params, std::move(data), callback);
    }

    void OpenModule(Binary::Container::Ptr data, const String& subpath, Module::DetectCallback& callback) const override
    {
      return Module::Open(*Params, std::move(data), subpath, callback);
    }

  private:
    DataLocation::Ptr OpenLocation(Binary::Container::Ptr data, const String& subpath) const
    {
      return ZXTune::OpenLocation(*Params, std::move(data), subpath);
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };

  Service::Ptr Service::Create(Parameters::Accessor::Ptr parameters)
  {
    return MakePtr<ServiceImpl>(std::move(parameters));
  }
}  // namespace ZXTune
