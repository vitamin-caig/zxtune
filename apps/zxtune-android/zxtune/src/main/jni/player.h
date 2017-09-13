/**
* 
* @file
*
* @brief Player access interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "storage.h"
//library includes
#include <module/holder.h>
#include <parameters/container.h>

namespace Player
{
  class Control
  {
  public:
    typedef std::unique_ptr<Control> Ptr;
    virtual ~Control() = default;

    virtual Parameters::Accessor::Ptr GetProperties() const = 0;
    virtual Parameters::Modifier::Ptr GetParameters() const = 0;

    virtual uint_t GetPosition() const = 0;
    virtual uint_t Analyze(uint_t maxEntries, uint32_t* bands, uint32_t* levels) const = 0;
    
    virtual bool Render(uint_t samples, int16_t* buffer) = 0;
    virtual void Seek(uint_t frame) = 0;
    
    virtual uint_t GetPlaybackPerformance() const = 0;
  };

  typedef ObjectsStorage<Control::Ptr> Storage;

  Storage::HandleType Create(Module::Holder::Ptr module);
}
