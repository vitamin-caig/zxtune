/**
 *
 * @file
 *
 * @brief JNI entry points
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "defines.h"
#include "module.h"
#include "player.h"
#include "plugin.h"
// library includes
#include <core/plugin.h>
// platform includes
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
