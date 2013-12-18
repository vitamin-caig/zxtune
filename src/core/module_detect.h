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
#include <core/module_holder.h>
#include <parameters/accessor.h>

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
    virtual ~DetectCallback() {}

    //! @brief Returns plugins parameters
    virtual Parameters::Accessor::Ptr GetPluginsParameters() const = 0;
    //! @brief Process module
    virtual void ProcessModule(ZXTune::DataLocation::Ptr location, Module::Holder::Ptr holder) const = 0;
    //! @brief Logging callback
    virtual Log::ProgressCallback* GetProgress() const = 0;
  };

  //! @brief Recursively search all the modules inside location
  //! @param location Start data location
  //! @param callback Detect callback
  //! @return Size in bytes of source data processed
  std::size_t Detect(ZXTune::DataLocation::Ptr location, const DetectCallback& callback);

  //! @brief Opens module directly from location
  //! @param location Start data location
  //! @param callback Detect callback
  //! @throw Error if no module found
  void Open(ZXTune::DataLocation::Ptr location, const DetectCallback& callback);
}
