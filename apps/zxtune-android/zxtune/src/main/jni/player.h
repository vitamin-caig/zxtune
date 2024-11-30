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

#include "apps/zxtune-android/zxtune/src/main/jni/storage.h"

#include "module/holder.h"
#include "parameters/container.h"

#include <jni.h>

namespace Player
{
  using TimeBase = Time::Millisecond;

  class Control
  {
  public:
    using Ptr = std::shared_ptr<Control>;
    virtual ~Control() = default;

    virtual Parameters::Container& GetParameters() const = 0;

    virtual uint_t GetPosition() const = 0;
    virtual uint_t Analyze(uint_t maxEntries, uint8_t* levels) const = 0;

    virtual bool Render(uint_t samples, int16_t* buffer) = 0;
    virtual void Seek(uint_t frame) = 0;

    virtual uint_t GetPlaybackPerformance() const = 0;
    virtual uint_t GetPlaybackProgress() const = 0;
  };

  using Storage = ObjectsStorage<Control::Ptr>;

  jobject Create(JNIEnv* env, const Module::Holder& module, uint_t samplerate);

  void InitJni(JNIEnv*);
  void CleanupJni(JNIEnv*);
}  // namespace Player
