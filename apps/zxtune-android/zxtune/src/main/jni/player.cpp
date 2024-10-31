/**
 *
 * @file
 *
 * @brief Player access implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-android/zxtune/src/main/jni/player.h"

#include "apps/zxtune-android/zxtune/src/main/jni/array.h"
#include "apps/zxtune-android/zxtune/src/main/jni/debug.h"
#include "apps/zxtune-android/zxtune/src/main/jni/defines.h"
#include "apps/zxtune-android/zxtune/src/main/jni/exception.h"
#include "apps/zxtune-android/zxtune/src/main/jni/global_options.h"
#include "apps/zxtune-android/zxtune/src/main/jni/properties.h"
#include "apps/zxtune-android/zxtune/src/main/jni/storage.h"

#include "module/players/pipeline.h"
#include "sound/impl/fft_analyzer.h"

#include "debug/log.h"
#include "module/holder.h"
#include "module/information.h"
#include "module/renderer.h"
#include "module/state.h"
#include "parameters/container.h"
#include "parameters/merged_accessor.h"
#include "sound/analyzer.h"
#include "sound/chunk.h"
#include "time/duration.h"
#include "time/instant.h"

#include "contract.h"
#include "error.h"
#include "make_ptr.h"
#include "pointers.h"

#include <jni.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <ctime>
#include <deque>
#include <exception>
#include <mutex>
#include <utility>
#include <vector>

namespace
{
  class NativePlayerJni
  {
  public:
    static void Init(JNIEnv* env)
    {
      auto* const tmpClass = env->FindClass("app/zxtune/core/jni/JniPlayer");
      Class = static_cast<jclass>(env->NewGlobalRef(tmpClass));
      Require(Class);
      Constructor = env->GetMethodID(Class, "<init>", "(I)V");
      Require(Constructor);
      Handle = env->GetFieldID(Class, "handle", "I");
      Require(Handle);
    }

    static void Cleanup(JNIEnv* env)
    {
      env->DeleteGlobalRef(Class);
      Class = nullptr;
      Constructor = 0;
      Handle = 0;
    }

    static Player::Storage::HandleType GetHandle(JNIEnv* env, jobject self)
    {
      return env->GetIntField(self, Handle);
    }

    static jobject Create(JNIEnv* env, Player::Storage::HandleType handle)
    {
      return env->NewObject(Class, Constructor, handle);
    }

  private:
    static jclass Class;
    static jmethodID Constructor;
    static jfieldID Handle;
  };

  jclass NativePlayerJni::Class;
  jmethodID NativePlayerJni::Constructor;
  jfieldID NativePlayerJni::Handle;

  static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
  static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
  static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");

  class BufferTarget
  {
  public:
    void Add(Sound::Chunk data)
    {
      TotalSamples += data.size();
      Buffers.emplace_back(std::move(data));
    }

    std::size_t GetSamples(std::size_t count, int16_t* target)
    {
      if (Buffers.empty())
      {
        return 0;
      }
      Buff& cur = Buffers.front();
      const std::size_t samples = count / Sound::Sample::CHANNELS;
      const std::size_t copied = cur.Get(samples, target);
      if (0 == cur.Avail)
      {
        Buffers.pop_front();
      }
      return copied * Sound::Sample::CHANNELS;
    }

    uint64_t GetTotalSamplesDone() const
    {
      return TotalSamples;
    }

  private:
    struct Buff
    {
      explicit Buff(Sound::Chunk data)
        : Data(std::move(data))
        , Avail(Data.size())
      {}

      std::size_t Get(std::size_t count, void* target)
      {
        const std::size_t toCopy = std::min(count, Avail);
        std::memcpy(target, &Data.back() + 1 - Avail, toCopy * sizeof(Data.front()));
        Avail -= toCopy;
        return toCopy;
      }

      Sound::Chunk Data;
      std::size_t Avail;
    };
    std::deque<Buff> Buffers;
    uint64_t TotalSamples = 0;
  };

  class RenderingPerformanceAccountant
  {
  public:
    void StartAccounting()
    {
      LastStart = std::clock();
    }

    void StopAccounting()
    {
      Clocks += std::clock() - LastStart;
      ++Frames;
    }

    uint_t Measure(uint64_t totalSamples, uint_t sampleRate) const
    {
      const uint_t MIN_DURATION_SEC = 1;
      const uint_t minSamples = sampleRate * MIN_DURATION_SEC;
      if (totalSamples >= minSamples)
      {
        if (const uint64_t totalClocks = Clocks + Frames / 2)  // compensate measuring error
        {
          // 100 * (totalSamples / sampleRate) / (totalClocks / CLOCKS_PER_SEC)
          return (totalSamples * CLOCKS_PER_SEC * 100) / (totalClocks * sampleRate);
        }
      }
      return 0;
    }

  private:
    std::clock_t LastStart = 0;
    std::clock_t Clocks = 0;
    uint_t Frames = 0;
  };

  class AnalyzerControl
  {
  public:
    AnalyzerControl()
      : Delegate(Sound::FFTAnalyzer::Create())
    {}

    void FrameStarted()
    {
      const std::scoped_lock guard(Lock);
      Current.Buffer.swap(Next.Buffer);
      Next.Parts = Current.Parts;
      Current.Parts = 0;
    }

    void FrameReady(uint_t count, void* buffer)
    {
      if (count && ++IdleFrames < 10)
      {
        const std::scoped_lock guard(Lock);
        const auto samples = count / Sound::Sample::CHANNELS;
        Next.Buffer.resize(samples);
        std::memcpy(Next.Buffer.data(), buffer, samples * sizeof(Sound::Sample));
      }
    }

    uint_t Analyze(uint_t maxEntries, uint8_t* levels) const
    {
      IdleFrames = 0;
      const std::scoped_lock guard(Lock);
      FeedPart();
      Delegate->GetSpectrum(safe_ptr_cast<Sound::Analyzer::LevelType*>(levels), maxEntries);
      return maxEntries;
    }

  private:
    struct State
    {
      Sound::Chunk Buffer;
      uint_t Parts = 0;
    };

    void FeedPart() const
    {
      const auto done = Current.Parts++;
      const auto total = std::max<uint_t>(Next.Parts, done + 1);
      if (const auto samples = Current.Buffer.size())
      {
        const auto start = samples * done / total;
        const auto end = samples * (done + 1) / total;
        Delegate->FeedSound(Current.Buffer.data() + start, end - start);
      }
      else
      {
        const Sound::Sample EMPTY[16];
        Delegate->FeedSound(EMPTY, 16);
      }
    }

  private:
    const Sound::FFTAnalyzer::Ptr Delegate;
    mutable std::mutex Lock;
    mutable State Current;
    State Next;
    mutable std::atomic<uint_t> IdleFrames = 0;
  };

  class PlayerControl : public Player::Control
  {
  public:
    PlayerControl(const Module::Holder& holder, uint_t samplerate, Parameters::Accessor::Ptr globalParams)
      : Duration(holder.GetModuleInformation()->Duration())
      , Samplerate(samplerate)
      , LocalParameters(Parameters::Container::Create())
      , Renderer(Module::CreatePipelinedRenderer(
            holder, samplerate, Parameters::CreateMergedAccessor(LocalParameters, std::move(globalParams))))
      , State(Renderer->GetState())
    {
      Require(Duration.Get() != 0);
    }

    Parameters::Modifier& GetParameters() const override
    {
      return *LocalParameters;
    }

    uint_t GetPosition() const override
    {
      return State->At().CastTo<Player::TimeBase>().Get();
    }

    uint_t Analyze(uint_t maxEntries, uint8_t* levels) const override
    {
      return Analyzer.Analyze(maxEntries, levels);
    }

    bool Render(uint_t samples, int16_t* buffer) override
    {
      Analyzer.FrameStarted();
      auto rest = samples;
      auto* target = buffer;
      for (;;)
      {
        if (const auto got = Buffer.GetSamples(rest, target))
        {
          target += got;
          rest -= got;
          if (!rest)
          {
            break;
          }
        }
        auto chunk = RenderNextFrame();
        if (chunk.empty())
        {
          break;
        }
        Buffer.Add(std::move(chunk));
      }
      std::fill_n(target, rest, 0);
      Analyzer.FrameReady(samples, buffer);
      return rest == 0;
    }

    void Seek(uint_t pos) override
    {
      Renderer->SetPosition(Time::Instant<Player::TimeBase>(pos));
    }

    uint_t GetPlaybackPerformance() const override
    {
      return RenderingPerformance.Measure(Buffer.GetTotalSamplesDone(), Samplerate);
    }

    // TODO: move to State
    uint_t GetPlaybackProgress() const override
    {
      const auto played = Time::Microseconds::FromRatio(Buffer.GetTotalSamplesDone(), Samplerate);
      return (played * 100).Divide<uint_t>(Duration);
    }

  private:
    Sound::Chunk RenderNextFrame()
    {
      RenderingPerformance.StartAccounting();
      auto chunk = Renderer->Render();
      RenderingPerformance.StopAccounting();
      return chunk;
    }

  private:
    const Time::Milliseconds Duration;
    const uint_t Samplerate;
    const Parameters::Container::Ptr LocalParameters;
    const Module::Renderer::Ptr Renderer;
    const Module::State::Ptr State;
    BufferTarget Buffer;
    RenderingPerformanceAccountant RenderingPerformance;
    AnalyzerControl Analyzer;
  };

  Player::Control::Ptr CreateControl(const Module::Holder& module, uint_t samplerate)
  {
    return MakePtr<PlayerControl>(module, samplerate, MakeSingletonPointer(Parameters::GlobalOptions()));
  }
}  // namespace

namespace Player
{
  jobject Create(JNIEnv* env, const Module::Holder& module, uint_t samplerate)
  {
    auto ctrl = CreateControl(module, samplerate);
    Dbg("Player::Create(module={})={}", static_cast<const void*>(&module), static_cast<const void*>(ctrl.get()));
    const auto handle = Player::Storage::Instance().Add(std::move(ctrl));
    return NativePlayerJni::Create(env, handle);
  }

  void InitJni(JNIEnv* env)
  {
    NativePlayerJni::Init(env);
  }

  void CleanupJni(JNIEnv* env)
  {
    NativePlayerJni::Cleanup(env);
  }
}  // namespace Player

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniPlayer_close(JNIEnv* /*env*/, jclass /*self*/, jint handle)
{
  if (Player::Storage::Instance().Fetch(handle))
  {
    Dbg("Player::Close(handle={})", handle);
  }
}

EXPORTED jboolean JNICALL Java_app_zxtune_core_jni_JniPlayer_render(JNIEnv* env, jobject self, jshortArray buffer)
{
  return Jni::Call(env, [=]() {
    const auto playerHandle = NativePlayerJni::GetHandle(env, self);
    const Jni::AutoShortArray buf(env, buffer);
    Jni::CheckArgument(buf, "Empty render buffer");
    if (const auto player = Player::Storage::Instance().Find(playerHandle))
    {
      return player->Render(buf.Size(), buf.Data());
    }
    else
    {
      return false;
    }
  });
}

EXPORTED jint JNICALL Java_app_zxtune_core_jni_JniPlayer_analyze(JNIEnv* env, jobject self, jbyteArray levels)
{
  return Jni::Call(env, [=]() {
    // Should be before AutoArray calls - else causes 'using JNI after critical get' error
    const auto playerHandle = NativePlayerJni::GetHandle(env, self);
    const auto player = Player::Storage::Instance().Find(playerHandle);
    const Jni::AutoByteArray rawLevels(env, levels);
    if (rawLevels && player)
    {
      return player->Analyze(rawLevels.Size(), rawLevels.Data());
    }
    else
    {
      return uint_t(0);
    }
  });
}

EXPORTED jint JNICALL Java_app_zxtune_core_jni_JniPlayer_getPositionMs(JNIEnv* env, jobject self)
{
  return Jni::Call(env, [=]() {
    const auto playerHandle = NativePlayerJni::GetHandle(env, self);
    if (const auto player = Player::Storage::Instance().Find(playerHandle))
    {
      return player->GetPosition();
    }
    else
    {
      return uint_t(0);
    }
  });
}

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniPlayer_setPositionMs(JNIEnv* env, jobject self, jint position)
{
  return Jni::Call(env, [=]() {
    const auto playerHandle = NativePlayerJni::GetHandle(env, self);
    if (const auto player = Player::Storage::Instance().Find(playerHandle))
    {
      player->Seek(position);
    }
  });
}

EXPORTED jint JNICALL Java_app_zxtune_core_jni_JniPlayer_getPerformance(JNIEnv* env, jobject self)
{
  return Jni::Call(env, [=]() {
    const auto playerHandle = NativePlayerJni::GetHandle(env, self);
    if (const auto player = Player::Storage::Instance().Find(playerHandle))
    {
      return player->GetPlaybackPerformance();
    }
    else
    {
      return uint_t(0);
    }
  });
}

EXPORTED jint JNICALL Java_app_zxtune_core_jni_JniPlayer_getProgress(JNIEnv* env, jobject self)
{
  return Jni::Call(env, [=]() {
    const auto playerHandle = NativePlayerJni::GetHandle(env, self);
    if (const auto player = Player::Storage::Instance().Find(playerHandle))
    {
      return player->GetPlaybackProgress();
    }
    else
    {
      return uint_t(0);
    }
  });
}

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniPlayer_setProperty__Ljava_lang_String_2J(JNIEnv* env, jobject self,
                                                                                           jstring propName,
                                                                                           jlong value)
{
  return Jni::Call(env, [=]() {
    const auto playerHandle = NativePlayerJni::GetHandle(env, self);
    if (const auto player = Player::Storage::Instance().Get(playerHandle))
    {
      auto& params = player->GetParameters();
      Jni::PropertiesWriteHelper helper(env, params);
      helper.Set(propName, value);
    }
  });
}

EXPORTED void JNICALL Java_app_zxtune_core_jni_JniPlayer_setProperty__Ljava_lang_String_2Ljava_lang_String_2(
    JNIEnv* env, jobject self, jstring propName, jstring value)
{
  return Jni::Call(env, [=]() {
    const auto playerHandle = NativePlayerJni::GetHandle(env, self);
    if (const auto player = Player::Storage::Instance().Get(playerHandle))
    {
      auto& params = player->GetParameters();
      Jni::PropertiesWriteHelper helper(env, params);
      helper.Set(propName, value);
    }
  });
}
