/**
 *
 * @file
 *
 * @brief Plugins access interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// platform includes
#include <jni.h>

namespace Plugin
{
  void InitJni(JNIEnv*);
  void CleanupJni(JNIEnv*);
}  // namespace Plugin
