/**
 *
 * @file
 *
 * @brief Binary data functions implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-android/zxtune/src/main/jni/binary.h"

#include "apps/zxtune-android/zxtune/src/main/jni/exception.h"

#include "binary/container_factories.h"

#include "contract.h"
#include "make_ptr.h"

namespace Binary
{
  class ByteBufferData : public Data
  {
  public:
    ByteBufferData(JNIEnv* env, jobject object)
      : Data(GetData(env, object))
      , Length(GetSize(env, object))
      , Vm(GetVm(env))
      , Object(env->NewGlobalRef(object))
    {}

    ~ByteBufferData() override
    {
      auto* env = GetEnv(Vm);
      env->DeleteGlobalRef(Object);
    }

    const void* Start() const override
    {
      return Data;
    }

    std::size_t Size() const override
    {
      return Length;
    }

  private:
    static const uint8_t* GetData(JNIEnv* env, jobject byteBuffer)
    {
      if (const auto* const addr = env->GetDirectBufferAddress(byteBuffer))
      {
        return static_cast<const uint8_t*>(addr);
      }
      throw Jni::NullPointerException("Not a direct buffer!");
    }

    static std::size_t GetSize(JNIEnv* env, jobject byteBuffer)
    {
      const auto capacity = env->GetDirectBufferCapacity(byteBuffer);
      Jni::CheckArgument(capacity > 0, "Empty buffer");
      return capacity;
    }

    static JavaVM* GetVm(JNIEnv* env)
    {
      JavaVM* result = nullptr;
      Require(env->GetJavaVM(&result) == JNI_OK);
      return result;
    }

    static JNIEnv* GetEnv(JavaVM* vm)
    {
      JNIEnv* result = nullptr;
      Require(vm->GetEnv(reinterpret_cast<void**>(&result), JNI_VERSION_1_6) == JNI_OK);
      return result;
    }

  private:
    const uint8_t* const Data;
    const std::size_t Length;
    JavaVM* const Vm;
    const jobject Object;
  };

  Container::Ptr CreateByteBufferContainer(JNIEnv* env, jobject byteBuffer)
  {
    auto data = MakePtr<ByteBufferData>(env, byteBuffer);
    return CreateContainer(std::move(data));
  }
}  // namespace Binary
