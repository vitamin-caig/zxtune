/**
 *
 * @file
 *
 * @brief Properties access helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "array.h"

#include <parameters/accessor.h>
#include <parameters/modifier.h>

#include <string_view.h>

#include <jni.h>

namespace Jni
{
  class JstringView : public StringView
  {
  public:
    JstringView(JNIEnv* env, jstring str)
      : StringView(MakeView(env, str))
      , Env(env)
      , Str(str)
    {}

    ~JstringView()
    {
      if (!empty())
      {
        Env->ReleaseStringUTFChars(Str, data());
      }
    }

  private:
    // Since android-4.0.1-r1 GetStringUTFLength returns 0 on null string
    // So, emulate that behaviour for all platforms
    static StringView MakeView(JNIEnv* env, jstring str)
    {
      if (const std::size_t size = str ? env->GetStringUTFLength(str) : 0)
      {
        return {env->GetStringUTFChars(str, 0), size};
      }
      else
      {
        return {};
      }
    }

  private:
    JNIEnv* const Env;
    const jstring Str;
  };

  inline jstring MakeJstring(JNIEnv* env, const String& str)
  {
    return env->NewStringUTF(str.c_str());
  }

  class TempJString
  {
  public:
    TempJString(JNIEnv* env, const String& str)
      : Env(env)
      , Jstr(MakeJstring(env, str))
    {}

    ~TempJString()
    {
      Env->DeleteLocalRef(Jstr);
    }

    jstring Get() const
    {
      return Jstr;
    }

  private:
    JNIEnv* const Env;
    const jstring Jstr;
  };

  class PropertiesReadHelper
  {
  public:
    PropertiesReadHelper(JNIEnv* env, const Parameters::Accessor& params)
      : Env(env)
      , Params(params)
    {}

    jlong Get(jstring name, jlong defVal) const
    {
      return Params.FindInteger(JstringView(Env, name)).value_or(defVal);
    }

    jstring Get(jstring name, jstring defVal) const
    {
      if (const auto val = Params.FindString(JstringView(Env, name)))
      {
        return MakeJstring(Env, *val);
      }
      return defVal;
    }

    jbyteArray Get(jstring name, jbyteArray defVal) const
    {
      if (const auto val = Params.FindData(JstringView(Env, name)))
      {
        return Jni::MakeByteArray(Env, *val);
      }
      return defVal;
    }

  private:
    JNIEnv* const Env;
    const Parameters::Accessor& Params;
  };

  class PropertiesWriteHelper
  {
  public:
    PropertiesWriteHelper(JNIEnv* env, Parameters::Modifier& params)
      : Env(env)
      , Params(params)
    {}

    virtual void Set(jstring name, jlong value)
    {
      Params.SetValue(JstringView(Env, name), value);
    }

    virtual void Set(jstring name, jstring value)
    {
      Params.SetValue(JstringView(Env, name), JstringView(Env, value));
    }

  private:
    JNIEnv* const Env;
    Parameters::Modifier& Params;
  };
}  // namespace Jni
