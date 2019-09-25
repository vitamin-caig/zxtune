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

//library includes
#include <binary/container.h>
//platform includes
#include <jni.h>

namespace Binary
{
  Container::Ptr CreateByteBufferContainer(JNIEnv* env, jobject byteBuffer);
}
