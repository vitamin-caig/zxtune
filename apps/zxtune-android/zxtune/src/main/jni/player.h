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
//platform includes
#include <jni.h>

namespace Player
{
  class Control
  {
  public:
    typedef std::shared_ptr<Control> Ptr;
    virtual ~Control() = default;

    virtual const Parameters::Accessor& GetProperties() const = 0;
    virtual Parameters::Modifier& GetParameters() const = 0;

    virtual uint_t GetPosition() const = 0;
    virtual uint_t Analyze(uint_t maxEntries, uint8_t* levels) const = 0;
    
    virtual bool Render(uint_t samples, int16_t* buffer) = 0;
    virtual void Seek(uint_t frame) = 0;
    
    virtual uint_t GetPlaybackPerformance() const = 0;
  };

  typedef ObjectsStorage<Control::Ptr> Storage;

  jobject Create(JNIEnv* env, Module::Holder::Ptr module);

  void InitJni(JNIEnv*);
  void CleanupJni(JNIEnv*);
}
