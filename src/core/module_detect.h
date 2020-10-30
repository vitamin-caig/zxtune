/**
*
* @file
*
* @brief  Modules detecting functionality
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/container.h>
#include <core/data_location.h>
#include <core/plugin.h>
#include <module/holder.h>
#include <parameters/accessor.h>
#include <parameters/container.h>

//forward declarations
namespace Log
{
  class ProgressCallback;
}

//! @brief Global library namespace
namespace Module
{
  class DetectCallback
  {
  public:
    virtual ~DetectCallback() = default;

    virtual Parameters::Container::Ptr CreateInitialProperties(const String& subpath) const = 0;
    //! @brief Process module
    virtual void ProcessModule(const ZXTune::DataLocation& location, const ZXTune::Plugin& decoder, Module::Holder::Ptr holder) = 0;
    //! @brief Logging callback
    virtual Log::ProgressCallback* GetProgress() const = 0;
  };

  //! @brief Recursively search all the modules inside location
  //! @param params Parameters for plugins
  //! @param data Data to scan
  //! @param callback Detect callback
  void Detect(const Parameters::Accessor& params, Binary::Container::Ptr data, DetectCallback& callback);

  void Open(const Parameters::Accessor& params, Binary::Container::Ptr data, const String& subpath, DetectCallback& callback);
}
