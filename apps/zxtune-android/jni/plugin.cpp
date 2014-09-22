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
#include "properties.h"
//library includes
#include <core/plugin.h>
#include <core/plugin_attrs.h>

namespace
{
  struct VisitorTraits
  {
    VisitorTraits()
      : OnPlayerPluginMethod()
      , OnDecoderPluginMethod()
      , OnMultitrackPluginMethod()
    {
    }

    void Init(JNIEnv* env)
    {
      const jclass classType = env->FindClass("app/zxtune/ZXTune$Plugins$Visitor");
      OnPlayerPluginMethod = env->GetMethodID(classType,
        "onPlayerPlugin", "(ILjava/lang/String;Ljava/lang/String;)V");
      OnDecoderPluginMethod = env->GetMethodID(classType,
        "onDecoderPlugin", "(Ljava/lang/String;Ljava/lang/String;)V");
      OnMultitrackPluginMethod = env->GetMethodID(classType,
        "onMultitrackPlugin", "(Ljava/lang/String;Ljava/lang/String;)V");
    }

    jmethodID OnPlayerPluginMethod;
    jmethodID OnDecoderPluginMethod;
    jmethodID OnMultitrackPluginMethod;
  };

  static VisitorTraits VISITOR;

  class VisitorAdapter
  {
  public:
    VisitorAdapter(JNIEnv* env, jobject delegate)
      : Env(env)
      , Delegate(delegate)
    {
    }
    
    void OnPlayerPlugin(jint devType, const String& plugId, const String& plugDescr) const
    {
      const Jni::TempJString id(Env, plugId);
      const Jni::TempJString descr(Env, plugDescr);
      Env->CallVoidMethod(Delegate, VISITOR.OnPlayerPluginMethod, devType, id.Get(), descr.Get());
    }

    void OnDecoderPlugin(const String& plugId, const String& plugDescr) const
    {
      const Jni::TempJString id(Env, plugId);
      const Jni::TempJString descr(Env, plugDescr);
      Env->CallVoidMethod(Delegate, VISITOR.OnDecoderPluginMethod, id.Get(), descr.Get());
    }

    void OnMultitrackPlugin(const String& plugId, const String& plugDescr) const
    {
      const Jni::TempJString id(Env, plugId);
      const Jni::TempJString descr(Env, plugDescr);
      Env->CallVoidMethod(Delegate, VISITOR.OnMultitrackPluginMethod, id.Get(), descr.Get());
    }
  private:
    JNIEnv* const Env;
    const jobject Delegate;
  };
}

JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_00024Plugins_init
  (JNIEnv* env, jclass /*self*/)
{
  VISITOR.Init(env);
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
    else if (0 != (caps & ZXTune::CAP_STOR_CONTAINER))
    {
      adapter.OnDecoderPlugin(id, desc);
    }
    else if (0 != (caps & ZXTune::CAP_STOR_MULTITRACK))
    {
      adapter.OnMultitrackPlugin(id, desc);
    }
  }
}
