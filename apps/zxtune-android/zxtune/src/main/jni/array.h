/**
 *
 * @file
 *
 * @brief Array helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/view.h"

#include "types.h"

#include <jni.h>

#include <cstring>

namespace Jni
{
  template<class StorageType, class ResultType>
  class AutoArray
  {
  public:
    AutoArray(JNIEnv* env, StorageType storage)
      : Env(env)
      , Storage(storage)
      , Length(Env->GetArrayLength(Storage))
      , Content(static_cast<ResultType*>(Env->GetPrimitiveArrayCritical(Storage, nullptr)))
    {}

    ~AutoArray()
    {
      if (Content)
      {
        Env->ReleasePrimitiveArrayCritical(Storage, Content, 0);
      }
    }

    operator bool() const
    {
      return Length != 0 && Content != nullptr;
    }

    ResultType* Data() const
    {
      return Length ? Content : nullptr;
    }

    std::size_t Size() const
    {
      return Length;
    }

  private:
    JNIEnv* const Env;
    const StorageType Storage;
    const jsize Length;
    ResultType* const Content;
  };

  using AutoByteArray = AutoArray<jbyteArray, uint8_t>;
  using AutoShortArray = AutoArray<jshortArray, int16_t>;

  inline jbyteArray MakeByteArray(JNIEnv* env, Binary::View data)
  {
    const auto buf = env->NewByteArray(data.Size());
    const AutoByteArray result(env, buf);
    std::memcpy(result.Data(), data.Start(), result.Size());
    return buf;
  }
}  // namespace Jni
