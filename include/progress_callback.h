/**
*
* @file     progress_callback.h
* @brief    Progress callback interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef PROGRESS_CALLBACK_H_DEFINED
#define PROGRESS_CALLBACK_H_DEFINED

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
    typedef std::auto_ptr<ProgressCallback> Ptr;
    virtual ~ProgressCallback() {}

    virtual void OnProgress(uint_t current) = 0;
    virtual void OnProgress(uint_t current, const String& message) = 0;

    static ProgressCallback& Stub();
  };

  ProgressCallback::Ptr CreatePercentProgressCallback(uint_t total, ProgressCallback& delegate);
  ProgressCallback::Ptr CreateNestedPercentProgressCallback(uint_t total, uint_t current, ProgressCallback& delegate);
}

#endif //PROGRESS_CALLBACK_H_DEFINED
