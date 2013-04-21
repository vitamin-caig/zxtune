/*
Abstract:
  Player interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef PLAYER_H_DEFINED
#define PLAYER_H_DEFINED

//local includes
#include "storage.h"
//library includes
#include <core/module_holder.h>

namespace Player
{
  class Control
  {
  public:
    typedef boost::shared_ptr<Control> Ptr;
    virtual ~Control() {}

    virtual uint_t GetPosition() const = 0;
    virtual uint_t Analyze(uint_t maxEntries, uint32_t* bands, uint32_t* levels) const = 0;
    virtual Parameters::Container::Ptr GetParameters() const = 0;
    
    virtual bool Render(uint_t samples, int16_t* buffer) = 0;
    virtual void Seek(uint_t frame) = 0;
  };

  typedef ObjectsStorage<Control::Ptr> Storage;

  Storage::HandleType Create(ZXTune::Module::Holder::Ptr module);
}

#endif //PLAYER_H_DEFINED
