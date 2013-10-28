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
#include "global_options.h"
#include "module.h"
#include "player.h"
#include "properties.h"
#include "zxtune.h"
//library includes
#include <parameters/merged_accessor.h>
#include <sound/mixer_factory.h>
//std includes
#include <deque>
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

    virtual void ApplyData(const Sound::Chunk::Ptr& data)
    {
      Buffers.push_back(Buff(data));
    }

    virtual void Flush()
    {
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
  private:
    struct Buff
    {
      explicit Buff(Sound::Chunk::Ptr data)
        : Data(data)
        , Avail(data->size())
      {
      }
      
      std::size_t Get(std::size_t count, void* target)
      {
        const std::size_t toCopy = std::min(count, Avail);
        std::memcpy(target, &Data->back() + 1 - Avail, toCopy * sizeof(Data->front()));
        Avail -= toCopy;
        return toCopy;
      }
    
      Sound::Chunk::Ptr Data;
      std::size_t Avail;
    };
    std::deque<Buff> Buffers;
  };

  class PlayerControl : public Player::Control
  {
  public:
    PlayerControl(Parameters::Container::Ptr params, Module::Renderer::Ptr render, BufferTarget::Ptr buffer)
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
      typedef std::vector<Module::Analyzer::ChannelState> ChannelsState;
      ChannelsState result;
      Analyser->GetState(result);
      uint_t doneEntries = 0;
      for (ChannelsState::const_iterator it = result.begin(), lim = result.end(); it != lim && doneEntries != maxEntries; ++it, ++doneEntries)
      {
        bands[doneEntries] = it->Band;
        levels[doneEntries] = it->Level;
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
    const Module::Renderer::Ptr Renderer;
    const BufferTarget::Ptr Buffer;
    const Module::TrackState::Ptr TrackState;
    const Module::Analyzer::Ptr Analyser;
  };

  Player::Control::Ptr CreateControl(const Module::Holder::Ptr module)
  {
    const Parameters::Accessor::Ptr globalParameters = Parameters::GlobalOptions();
    const Parameters::Container::Ptr localParameters = Parameters::Container::Create();
    const Parameters::Accessor::Ptr internalProperties = module->GetModuleProperties();
    const Parameters::Accessor::Ptr properties = Parameters::CreateMergedAccessor(localParameters, internalProperties, globalParameters);
    const BufferTarget::Ptr buffer = boost::make_shared<BufferTarget>();
    const Module::Renderer::Ptr renderer = module->CreateRenderer(properties, buffer);
    return boost::make_shared<PlayerControl>(localParameters, renderer, buffer);
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
  int Create(Module::Holder::Ptr module)
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
