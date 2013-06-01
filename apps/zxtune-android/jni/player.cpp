/*
Abstract:
  Player implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "debug.h"
#include "module.h"
#include "player.h"
#include "properties.h"
#include "zxtune.h"
//common includes
#include <cycle_buffer.h>
//library includes
#include <sound/mixer_factory.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/type_traits/is_signed.hpp>

namespace
{
  BOOST_STATIC_ASSERT(Sound::Sample::CHANNELS == 2);
  BOOST_STATIC_ASSERT(Sound::Sample::BITS == 16);
  BOOST_STATIC_ASSERT(boost::is_signed<Sound::Sample::Type>::value);

  class BufferTarget : public Sound::Receiver
  {
  public:
    typedef boost::shared_ptr<BufferTarget> Ptr;

    BufferTarget()
      : Buffer(32768)
    {
    }
    
    virtual void ApplyData(const Sound::Sample& data)
    {
      Buffer.Put(&data, 1);
    }

    virtual void Flush()
    {
    }

    std::size_t GetSamples(std::size_t count, int16_t* target)
    {
      const Sound::Sample* part1 = 0;
      std::size_t part1Size = 0;
      const Sound::Sample* part2 = 0;
      std::size_t part2Size = 0;
      if (const std::size_t got = Buffer.Peek(count / Sound::Sample::CHANNELS, part1, part1Size, part2, part2Size))
      {
        std::memcpy(target, part1, part1Size * sizeof(*part1));
        if (part2)
        {
          std::memcpy(target + part1Size * Sound::Sample::CHANNELS, part2, part2Size * sizeof(*part2));
        }
        return Buffer.Consume(got) * Sound::Sample::CHANNELS;
      }
      return 0;
    }
  private:
    CycleBuffer<Sound::Sample> Buffer;
  };

  class PlayerControl : public Player::Control
  {
  public:
    PlayerControl(Parameters::Container::Ptr params, ZXTune::Module::Renderer::Ptr render, BufferTarget::Ptr buffer)
      : Params(params)
      , Renderer(render)
      , Buffer(buffer)
      , TrackState(Renderer->GetTrackState())
      , Analyser(Renderer->GetAnalyzer())
    {
    }
    
    virtual uint_t GetPosition() const
    {
      return TrackState->Frame();
    }

    virtual uint_t Analyze(uint_t maxEntries, uint32_t* bands, uint32_t* levels) const
    {
      typedef std::vector<ZXTune::Module::Analyzer::ChannelState> ChannelsState;
      ChannelsState result;
      Analyser->GetState(result);
      uint_t doneEntries = 0;
      for (ChannelsState::const_iterator it = result.begin(), lim = result.end(); it != lim && doneEntries != maxEntries; ++it)
      {
        if (it->Enabled)
        {
          bands[doneEntries] = it->Band;
          levels[doneEntries] = it->Level;
          ++doneEntries;
        }
      }
      return doneEntries;
    }

    virtual Parameters::Container::Ptr GetParameters() const
    {
      return Params;
    }
    
    virtual bool Render(uint_t samples, int16_t* buffer)
    {
      for (;;)
      {
        if (const std::size_t got = Buffer->GetSamples(samples, buffer))
        {
          buffer += got;
          samples -= got;
          if (!samples)
          {
            break;
          }
        }
        if (!Renderer->RenderFrame())
        {
          break;
        }
      }
      std::fill_n(buffer, samples, 0);
      return samples == 0;
    }

    virtual void Seek(uint_t frame)
    {
      Renderer->SetPosition(frame);
    }
  private:
    const Parameters::Container::Ptr Params;
    const ZXTune::Module::Renderer::Ptr Renderer;
    const BufferTarget::Ptr Buffer;
    const ZXTune::Module::TrackState::Ptr TrackState;
    const ZXTune::Module::Analyzer::Ptr Analyser;
  };

  Player::Control::Ptr CreateControl(const ZXTune::Module::Holder::Ptr module)
  {
    const Parameters::Container::Ptr params = Parameters::Container::Create();
    const Parameters::Accessor::Ptr props = module->GetModuleProperties();
    const Parameters::Accessor::Ptr allProps = Parameters::CreateMergedAccessor(params, props);
    const BufferTarget::Ptr buffer = boost::make_shared<BufferTarget>();
    const ZXTune::Module::Renderer::Ptr renderer = module->CreateRenderer(allProps, buffer);
    return boost::make_shared<PlayerControl>(params, renderer, buffer);
  }

  template<class StorageType, class ResultType>
  class AutoArray
  {
  public:
    AutoArray(JNIEnv* env, StorageType storage)
    : Env(env)
      , Storage(storage)
      , Length(Env->GetArrayLength(Storage))
      , Content(static_cast<ResultType*>(Env->GetPrimitiveArrayCritical(Storage, 0)))
    {
    }

    ~AutoArray()
    {
      if (Content)
      {
        Env->ReleasePrimitiveArrayCritical(Storage, Content, 0);
      }
    }

    operator bool () const
    {
      return Length != 0 && Content != 0;
    }

    ResultType* Data() const
    {
      return Length ? Content : 0;
    }

    std::size_t Size() const
    {
      return Length;
    }
  private:
    JNIEnv* const Env;
    const StorageType Storage;
    const jsize Length;
    ResultType* const Content;
  };
}

namespace Player
{
  int Create(ZXTune::Module::Holder::Ptr module)
  {
    const Player::Control::Ptr ctrl = CreateControl(module);
    Dbg("Player::Create(module=%p)=%p", module.get(), ctrl.get());
    return Player::Storage::Instance().Add(ctrl);
  }
}

JNIEXPORT jboolean JNICALL Java_app_zxtune_ZXTune_Player_1Render
  (JNIEnv* env, jclass /*self*/, jint playerHandle, jshortArray buffer)
{
  if (const Player::Control::Ptr player = Player::Storage::Instance().Get(playerHandle))
  {
    typedef AutoArray<jshortArray, int16_t> ArrayType;
    if (ArrayType buf = ArrayType(env, buffer))
    {
      return player->Render(buf.Size(), buf.Data());
    }
  }
  return false;
}

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Player_1Analyze
  (JNIEnv* env, jclass /*self*/, jint playerHandle, jintArray bands, jintArray levels)
{
  if (const Player::Control::Ptr player = Player::Storage::Instance().Get(playerHandle))
  {
    typedef AutoArray<jintArray, uint32_t> ArrayType;
    ArrayType rawBands(env, bands);
    ArrayType rawLevels(env, levels);
    if (rawBands && rawLevels)
    {
      return player->Analyze(std::min(rawBands.Size(), rawLevels.Size()), rawBands.Data(), rawLevels.Data());
    }
  }
  return 0;
}

JNIEXPORT jint JNICALL Java_app_zxtune_ZXTune_Player_1GetPosition
  (JNIEnv* /*env*/, jclass /*self*/, jint playerHandle)
{
  if (const Player::Control::Ptr player = Player::Storage::Instance().Get(playerHandle))
  {
    return player->GetPosition();
  }
  return -1;
}

JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_Player_1SetPosition
  (JNIEnv* /*env*/, jclass /*self*/, jint playerHandle, jint position)
{
  if (const Player::Control::Ptr player = Player::Storage::Instance().Get(playerHandle))
  {
    player->Seek(position);
  }
}

JNIEXPORT jlong JNICALL Java_app_zxtune_ZXTune_Player_1GetProperty__ILjava_lang_String_2J
  (JNIEnv* env, jclass /*self*/, jint playerHandle, jstring propName, jlong defVal)
{
  if (const Player::Control::Ptr player = Player::Storage::Instance().Get(playerHandle))
  {
    const Parameters::Container::Ptr params = player->GetParameters();
    const Jni::PropertiesReadHelper props(env, *params);
    return props.Get(propName, defVal);
  }
  return defVal;
}

JNIEXPORT jstring JNICALL Java_app_zxtune_ZXTune_Player_1GetProperty__ILjava_lang_String_2Ljava_lang_String_2
  (JNIEnv* env, jclass /*self*/, jint playerHandle, jstring propName, jstring defVal)
{
  if (const Player::Control::Ptr player = Player::Storage::Instance().Get(playerHandle))
  {
    const Parameters::Container::Ptr params = player->GetParameters();
    const Jni::PropertiesReadHelper props(env, *params);
    return props.Get(propName, defVal);
  }
  return defVal;
}

JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_Player_1SetProperty__ILjava_lang_String_2J
  (JNIEnv* env, jclass /*self*/, jint playerHandle, jstring propName, jlong value)
{
  if (const Player::Control::Ptr player = Player::Storage::Instance().Get(playerHandle))
  {
    const Parameters::Container::Ptr params = player->GetParameters();
    Jni::PropertiesWriteHelper props(env, *params);
    props.Set(propName, value);
  }
}

JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_Player_1SetProperty__ILjava_lang_String_2Ljava_lang_String_2
  (JNIEnv* env, jclass /*self*/, jint playerHandle, jstring propName, jstring value)
{
  if (const Player::Control::Ptr player = Player::Storage::Instance().Get(playerHandle))
  {
    const Parameters::Container::Ptr params = player->GetParameters();
    Jni::PropertiesWriteHelper props(env, *params);
    props.Set(propName, value);
  }
}
