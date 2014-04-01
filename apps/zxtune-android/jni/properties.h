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

//common includes
#include <parameters/accessor.h>
#include <parameters/modifier.h>
//platform includes
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
    if (const std::size_t size = env->GetStringUTFLength(str))
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
    {
    }

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
    {
    }
    
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
    {
    }

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
}
