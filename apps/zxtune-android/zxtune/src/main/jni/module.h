/**
 *
 * @file
 *
 * @brief Module access interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-android/zxtune/src/main/jni/storage.h"

#include "module/holder.h"

#include <jni.h>

namespace Module
{
  using Storage = ObjectsStorage<Module::Holder::Ptr>;

  void InitJni(JNIEnv*);
  void CleanupJni(JNIEnv*);
}  // namespace Module
