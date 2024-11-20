/**
 *
 * @file
 *
 * @brief Binary data functions interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"

#include <jni.h>

namespace Binary
{
  Container::Ptr CreateByteBufferContainer(JNIEnv* env, jobject byteBuffer);
}
