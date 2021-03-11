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
  typedef ObjectsStorage<Module::Holder::Ptr> Storage;

  void InitJni(JNIEnv*);
  void CleanupJni(JNIEnv*);
}  // namespace Module
