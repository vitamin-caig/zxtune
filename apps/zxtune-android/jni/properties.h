/*
Abstract:
  Properties access functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef PROPERTIES_H_DEFINED
#define PROPERTIES_H_DEFINED

//common includes
#include <parameters/accessor.h>
#include <parameters/modifier.h>
//platform includes
#include <jni.h>

namespace Jni
{
  class StringHelper
  {
  public:
    StringHelper(JNIEnv* env, jstring js)
      : Env(env)
      , Jstr(js)
    {
    }
    
    StringHelper(JNIEnv* env, const String& str)
      : Env(env)
      , Jstr(0)
      , Cstr(str)
    {
    }
    
    String AsString() const
    {
      if (Cstr.empty())
      {
        if (const std::size_t size = Env->GetStringUTFLength(Jstr))
        {
          const char* const syms = Env->GetStringUTFChars(Jstr, 0);
          Cstr.assign(syms, syms + size);
          Env->ReleaseStringUTFChars(Jstr, syms);
        }
      }
      return Cstr;
    }
    
    jstring AsJstring() const
    {
      if (!Jstr)
      {
        Jstr = Env->NewStringUTF(Cstr.c_str());
      }
      return Jstr;
    }
  private:
    JNIEnv* const Env;
    mutable jstring Jstr;
    mutable String Cstr;
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
      const StringHelper strName(Env, name);
      Parameters::IntType val = defVal;
      Params.FindValue(strName.AsString(), val);
      return val;
    }
    
    jstring Get(jstring name, jstring defVal) const
    {
      const StringHelper strName(Env, name);
      Parameters::StringType val;
      if (Params.FindValue(strName.AsString(), val))
      {
        return StringHelper(Env, val).AsJstring();
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
      const StringHelper strName(Env, name);
      Params.SetValue(strName.AsString(), value);
    }

    virtual void Set(jstring name, jstring value)
    {
      const StringHelper strName(Env, name);
      const StringHelper strValue(Env, value);
      Params.SetValue(strName.AsString(), strValue.AsString());
    }
  private:
    JNIEnv* const Env;
    Parameters::Modifier& Params;
  };
}

#endif //PROPERTIES_H_DEFINED
