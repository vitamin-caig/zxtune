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

// local includes
#include "storage.h"
// library includes
#include <module/holder.h>
// platform includes
#include <jni.h>

namespace Module
{
  using Storage = ObjectsStorage<Module::Holder::Ptr>;

  void InitJni(JNIEnv*);
  void CleanupJni(JNIEnv*);
}  // namespace Module
