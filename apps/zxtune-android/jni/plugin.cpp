/**
* 
* @file
*
* @brief Plugins operating
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "zxtune.h"
//library includes
#include <core/plugin.h>
#include <core/plugin_attrs.h>

namespace
{
  class VisitorAdapter
  {
  public:
    VisitorAdapter(JNIEnv* env, jobject delegate)
      : Env(env)
      , Delegate(delegate)
      , OnPlayerPluginMethod(env->GetMethodID(env->GetObjectClass(Delegate),
        "onPlayerPlugin",
        "(ILjava/lang/String;Ljava/lang/String;)V"))
    {
    }
    
    void OnPlayerPlugin(jint devType, const String& plugId, const String& plugDescr) const
    {
      const jstring id = Env->NewStringUTF(plugId.c_str());
      const jstring descr = Env->NewStringUTF(plugDescr.c_str());
      Env->CallVoidMethod(Delegate, OnPlayerPluginMethod, devType, id, descr);
    }
  private:
    JNIEnv* const Env;
    const jobject Delegate;
    const jmethodID OnPlayerPluginMethod;
  };
}

JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_Plugins_1Enumerate
  (JNIEnv* env, jclass /*self*/, jobject visitor)
{
  const VisitorAdapter adapter(env, visitor);
  for (ZXTune::Plugin::Iterator::Ptr iter = ZXTune::EnumeratePlugins(); iter->IsValid(); iter->Next())
  {
    const ZXTune::Plugin::Ptr plug = iter->Get();
    const uint_t caps = plug->Capabilities();
    const String id = plug->Id();
    const String desc = plug->Description();
    if (0 != (caps & ZXTune::CAP_STOR_MODULE))
    {
      adapter.OnPlayerPlugin(caps & ZXTune::CAP_DEVICE_MASK, id, desc);
    }
  }
}
