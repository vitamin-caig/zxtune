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

// common includes
#include <string_view.h>
#include <types.h>

//! @brief Namespace is used for logging and other informational purposes
namespace Log
{
  //! @brief Progress receiver
  class ProgressCallback
  {
  public:
    virtual ~ProgressCallback() = default;

    virtual void OnProgress(uint_t current) = 0;
    virtual void OnProgress(uint_t current, StringView message) = 0;

    static ProgressCallback& Stub();
  };
}  // namespace Log
