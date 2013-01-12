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
//common includes
#include <parameters.h>

namespace Player
{
  class Control
  {
  public:
    typedef boost::shared_ptr<Control> Ptr;
    virtual ~Control() {}

    virtual uint_t GetPosition() const = 0;
    virtual Parameters::Container::Ptr GetParameters() const = 0;
    
    virtual bool Render(std::size_t samples, int16_t* buffer) = 0;
    virtual void Seek(uint_t frame) = 0;
  };

  typedef ObjectsStorage<Control::Ptr> Storage;
}

#endif //PLAYER_H_DEFINED
