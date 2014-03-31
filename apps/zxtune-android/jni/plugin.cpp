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
  struct VisitorTraits
  {
    VisitorTraits()
      : ClassType()
      , OnPlayerPluginMethod()
      , OnDecoderPluginMethod()
      , OnMultitrackPluginMethod()
    {
    }

    void Init(JNIEnv* env)
    {
      ClassType = env->FindClass("app/zxtune/ZXTune$Plugins$Visitor");
      OnPlayerPluginMethod = env->GetMethodID(ClassType,
        "onPlayerPlugin", "(ILjava/lang/String;Ljava/lang/String;)V");
      OnDecoderPluginMethod = env->GetMethodID(ClassType,
        "onDecoderPlugin", "(Ljava/lang/String;Ljava/lang/String;)V");
      OnMultitrackPluginMethod = env->GetMethodID(ClassType,
        "onMultitrackPlugin", "(Ljava/lang/String;Ljava/lang/String;)V");
    }

    jclass ClassType;
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
      const jstring id = Env->NewStringUTF(plugId.c_str());
      const jstring descr = Env->NewStringUTF(plugDescr.c_str());
      Env->CallVoidMethod(Delegate, VISITOR.OnPlayerPluginMethod, devType, id, descr);
    }

    void OnDecoderPlugin(const String& plugId, const String& plugDescr) const
    {
      const jstring id = Env->NewStringUTF(plugId.c_str());
      const jstring descr = Env->NewStringUTF(plugDescr.c_str());
      Env->CallVoidMethod(Delegate, VISITOR.OnDecoderPluginMethod, id, descr);
    }

    void OnMultitrackPlugin(const String& plugId, const String& plugDescr) const
    {
      const jstring id = Env->NewStringUTF(plugId.c_str());
      const jstring descr = Env->NewStringUTF(plugDescr.c_str());
      Env->CallVoidMethod(Delegate, VISITOR.OnMultitrackPluginMethod, id, descr);
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
