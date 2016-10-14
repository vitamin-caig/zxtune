/**
*
* @file
*
* @brief  Progress callback interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//std includes
#include <memory>

//! @brief Namespace is used for logging and other informational purposes
namespace Log
{
  //! @brief Progress receiver
  class ProgressCallback
  {
  public:
    typedef std::unique_ptr<ProgressCallback> Ptr;
    virtual ~ProgressCallback() = default;

    virtual void OnProgress(uint_t current) = 0;
    virtual void OnProgress(uint_t current, const String& message) = 0;

    static ProgressCallback& Stub();
  };

  ProgressCallback::Ptr CreatePercentProgressCallback(uint_t total, ProgressCallback& delegate);
  ProgressCallback::Ptr CreateNestedPercentProgressCallback(uint_t total, uint_t current, ProgressCallback& delegate);
}
