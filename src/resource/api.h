/**
*
* @file      resource/api.h
* @brief     Interface for working with embedded resources
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef RESOURCE_API_H_DEFINED
#define RESOURCE_API_H_DEFINED

//common includes
#include <types.h>
//library includes
#include <binary/container.h>

namespace Resource
{
  Binary::Container::Ptr Load(const String& name);

  class Visitor
  {
  public:
    virtual ~Visitor() {}

    virtual void OnResource(const String& name) = 0;
  };

  void Enumerate(Visitor& visitor);
}

#endif //RESOURCE_API_H_DEFINED
