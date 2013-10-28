/*
Abstract:
  Sound backends storage interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef SOUND_BACKENDS_STORAGE_H_DEFINED
#define SOUND_BACKENDS_STORAGE_H_DEFINED

//local includes
#include "backend_impl.h"

namespace Sound
{
  class BackendsStorage
  {
  public:
    virtual ~BackendsStorage() {}

    //Functional
    virtual void Register(const String& id, const char* description, uint_t caps, BackendWorkerFactory::Ptr factory) = 0;
    //Disabled due to error
    virtual void Register(const String& id, const char* description, uint_t caps, const Error& status) = 0;
    //Disabled due to configuration
    virtual void Register(const String& id, const char* description, uint_t caps) = 0;
  };
}

#endif //SOUND_BACKENDS_STORAGE_H_DEFINED
