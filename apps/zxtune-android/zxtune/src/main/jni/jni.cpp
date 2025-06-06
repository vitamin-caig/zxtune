/**
 *
 * @file
 *
 * @brief JNI entry points
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-android/zxtune/src/main/jni/defines.h"
#include "apps/zxtune-android/zxtune/src/main/jni/module.h"
#include "apps/zxtune-android/zxtune/src/main/jni/player.h"
#include "apps/zxtune-android/zxtune/src/main/jni/plugin.h"

#include "core/plugin.h"

#include <jni.h>

const jint JNI_VERSION = JNI_VERSION_1_6;

EXPORTED jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
  JNIEnv* env;
  if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION) != JNI_OK)
  {
    return JNI_ERR;
  }

  Module::InitJni(env);
  Player::InitJni(env);
  Plugin::InitJni(env);

  return JNI_VERSION;
}

EXPORTED void JNI_OnUnload(JavaVM* vm, void* /*reserved*/)
{
  JNIEnv* env;
  vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION);

  Plugin::CleanupJni(env);
  Player::CleanupJni(env);
  Module::CleanupJni(env);
}

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniApi_forcedInit(JNIEnv* /*env*/, jobject /*self*/)
{
  class EmptyVisitor : public ZXTune::PluginVisitor
  {
  public:
    void Visit(const ZXTune::Plugin&) override {}
  } stub;
  ZXTune::EnumeratePlugins(stub);
}
