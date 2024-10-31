/**
 *
 * @file
 *
 * @brief  JNI gate to benchmark library
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/benchmark/core/benchmark.h"

#include <android/log.h>
#include <jni.h>

#include <cmath>
#include <sstream>

namespace
{
  void Debug(const char* msg)
  {
    __android_log_print(ANDROID_LOG_DEBUG, "zxtune.benchmark.so", msg);
  }

  template<class T>
  void Debug(const char* msg, T arg)
  {
    __android_log_print(ANDROID_LOG_DEBUG, "zxtune.benchmark.so", msg, arg);
  }

  template<class T1, class T2>
  void Debug(const char* msg, T1 arg1, T2 arg2)
  {
    __android_log_print(ANDROID_LOG_DEBUG, "zxtune.benchmark.so", msg, arg1, arg2);
  }
}  // namespace

namespace
{
  class JavaVisitorAdapter : public Benchmark::TestsVisitor
  {
  public:
    JavaVisitorAdapter(JNIEnv* env, jobject visitor)
      : Env(env)
      , Visitor(visitor)
      , Check(env->GetMethodID(env->GetObjectClass(visitor), "OnTest", "(Ljava/lang/String;Ljava/lang/String;)Z"))
      , Pass(env->GetMethodID(env->GetObjectClass(visitor), "OnTestResult", "(Ljava/lang/String;Ljava/lang/String;D)V"))
    {
      Debug("JavaVisitorAdapter(Check=%p, Pass=%p)", Check, Pass);
    }

    void OnPerformanceTest(const Benchmark::PerformanceTest& test) override
    {
      const std::string category = test.Category();
      const std::string name = test.Name();
      Debug("OnPerformanceTest(%s, %s) called", category.c_str(), name.c_str());
      const jstring jcategory = Env->NewStringUTF(category.c_str());
      const jstring jname = Env->NewStringUTF(name.c_str());
      const jboolean doCall = Env->CallBooleanMethod(Visitor, Check, jcategory, jname);
      if (doCall == JNI_TRUE)
      {
        const jdouble res = test.Execute();
        Debug("Test executed with result %f", res);
        Env->CallVoidMethod(Visitor, Pass, jcategory, jname, res);
      }
    }

  private:
    JNIEnv* const Env;
    const jobject Visitor;
    const jmethodID Check;
    const jmethodID Pass;
  };
}  // namespace

extern "C" JNIEXPORT void JNICALL Java_app_zxtune_benchmark_Benchmark_ForAllTests(JNIEnv* env, jobject /*self*/,
                                                                                  jclass visitor)
{
  Debug("ForAllTests called");
  JavaVisitorAdapter adapter(env, visitor);
  Benchmark::ForAllTests(adapter);
}
