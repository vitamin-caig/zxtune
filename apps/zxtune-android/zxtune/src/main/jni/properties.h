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

// library includes
#include <parameters/accessor.h>
#include <parameters/modifier.h>
// platform includes
#include <jni.h>

namespace Jni
{
  inline jstring MakeJstring(JNIEnv* env, const String& str)
  {
    return env->NewStringUTF(str.c_str());
  }

  inline String MakeString(JNIEnv* env, jstring str)
  {
    String res;
    // Since android-4.0.1-r1 GetStringUTFLength returns 0 on null string
    // So, emulate that behaviour for all platforms
    if (const std::size_t size = str ? env->GetStringUTFLength(str) : 0)
    {
      const char* const syms = env->GetStringUTFChars(str, 0);
      res.assign(syms, syms + size);
      env->ReleaseStringUTFChars(str, syms);
    }
    return res;
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
      Parameters::IntType val = defVal;
      Params.FindValue(MakeString(Env, name), val);
      return val;
    }

    jstring Get(jstring name, jstring defVal) const
    {
      Parameters::StringType val;
      if (Params.FindValue(MakeString(Env, name), val))
      {
        return MakeJstring(Env, val);
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
      Params.SetValue(MakeString(Env, name), value);
    }

    virtual void Set(jstring name, jstring value)
    {
      Params.SetValue(MakeString(Env, name), MakeString(Env, value));
    }

  private:
    JNIEnv* const Env;
    Parameters::Modifier& Params;
  };
}  // namespace Jni
