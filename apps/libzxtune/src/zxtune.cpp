/**
 *
 * @file
 *
 * @brief LibZXTune implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#define ZXTUNE_API ZXTUNE_API_EXPORT
#include "../zxtune.h"
// common includes
#include <contract.h>
#include <cycle_buffer.h>
#include <error_tools.h>
#include <make_ptr.h>
#include <progress_callback.h>
// library includes
#include <binary/container.h>
#include <binary/container_factories.h>
#include <core/core_parameters.h>
#include <core/module_open.h>
#include <module/holder.h>
#include <parameters/container.h>
#include <platform/version/api.h>
#include <sound/sound_parameters.h>
// std includes
#include <map>

namespace Text
{
  extern const Char PROGRAM_NAME[] = {'l', 'i', 'b', 'z', 'x', 't', 'u', 'n', 'e', 0};
}

namespace
{
  template<class ObjPtrType>
  struct ObjectTraits
  {
    static ZXTuneHandle GetHandle(ObjPtrType obj)
    {
      return reinterpret_cast<ZXTuneHandle>(obj.get());
    }
  };

  template<class PtrType>
  class HandlesCache
  {
  public:
    ZXTuneHandle Add(PtrType val)
    {
      const ZXTuneHandle result = ObjectTraits<PtrType>::GetHandle(val);
      Require(result != ZXTuneHandle());
      Require(Handles.insert(typename Handle2Object::value_type(result, val)).second);
      return result;
    }

    void Delete(ZXTuneHandle handle)
    {
      Handles.erase(handle);
    }

    PtrType Get(ZXTuneHandle handle) const
    {
      const typename Handle2Object::const_iterator it = Handles.find(handle);
      Require(it != Handles.end());
      return it->second;
    }

    static HandlesCache<PtrType>& Instance()
    {
      static HandlesCache<PtrType> self;
      return self;
    }

  private:
    typedef std::map<ZXTuneHandle, PtrType> Handle2Object;
    Handle2Object Handles;
  };

  typedef HandlesCache<Binary::Container::Ptr> ContainersCache;
  typedef HandlesCache<Module::Holder::Ptr> ModulesCache;

  static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
  static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
  static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");

  class BufferRender : public Sound::Receiver
  {
  public:
    typedef std::shared_ptr<BufferRender> Ptr;

    BufferRender()
      : Buffer(32768) = default;

    void ApplyData(Sound::Chunk data) override
    {
      Buffer.Put(data.begin(), data.size());
    }

    void Flush() override {}

    std::size_t GetCurrentSample() const
    {
      return DoneSamples;
    }

    std::size_t GetSamples(std::size_t count, Sound::Sample* target)
    {
      const Sound::Sample* part1 = 0;
      std::size_t part1Size = 0;
      const Sound::Sample* part2 = 0;
      std::size_t part2Size = 0;
      if (const std::size_t toGet = Buffer.Peek(count, part1, part1Size, part2, part2Size))
      {
        std::memcpy(target, part1, part1Size * sizeof(*part1));
        if (part2)
        {
          std::memcpy(target + part1Size, part2, part2Size * sizeof(*part2));
        }
        Buffer.Consume(toGet);
        DoneSamples += toGet;
        return toGet;
      }
      return 0;
    }

    std::size_t DropSamples(std::size_t count)
    {
      const std::size_t toDrop = Buffer.Consume(count);
      DoneSamples += toDrop;
      return toDrop;
    }

    void Reset()
    {
      Buffer.Reset();
      DoneSamples = 0;
    }

  private:
    CycleBuffer<Sound::Sample> Buffer;
    std::size_t DoneSamples = 0;
  };

  class PlayerWrapper
  {
  public:
    typedef std::shared_ptr<PlayerWrapper> Ptr;

    PlayerWrapper(Parameters::Container::Ptr params, Module::Renderer::Ptr renderer, BufferRender::Ptr buffer)
      : Params(params)
      , Renderer(renderer)
      , Buffer(buffer)
    {}

    std::size_t RenderSound(Sound::Sample* target, std::size_t samples)
    {
      std::size_t result = 0;
      while (samples)
      {
        const std::size_t got = Buffer->GetSamples(samples, target);
        target += got;
        samples -= got;
        result += got;
        if (0 == samples || !Renderer->RenderFrame())
        {
          break;
        }
      }
      return result;
    }

    std::size_t Seek(std::size_t samples)
    {
      if (samples < Buffer->GetCurrentSample())
      {
        Reset();
      }
      while (samples != Buffer->GetCurrentSample())
      {
        const std::size_t toDrop = samples - Buffer->GetCurrentSample();
        if (const std::size_t dropped = Buffer->DropSamples(toDrop))
        {
          continue;
        }
        if (!Renderer->RenderFrame())
        {
          break;
        }
      }
      return Buffer->GetCurrentSample();
    }

    void Reset()
    {
      Renderer->Reset();
      Buffer->Reset();
    }

    Parameters::Container::Ptr GetParameters() const
    {
      return Params;
    }

    static Ptr Create(Module::Holder::Ptr holder)
    {
      const Parameters::Container::Ptr params = Parameters::Container::Create();
      // copy initial properties
      holder->GetModuleProperties()->Process(*params);
      const BufferRender::Ptr buffer = MakePtr<BufferRender>();
      const Module::Renderer::Ptr renderer = holder->CreateRenderer(params, buffer);
      return MakePtr<PlayerWrapper>(params, renderer, buffer);
    }

  private:
    const Parameters::Container::Ptr Params;
    const Module::Renderer::Ptr Renderer;
    const BufferRender::Ptr Buffer;
  };

  typedef HandlesCache<PlayerWrapper::Ptr> PlayersCache;

  bool FindDefaultValue(const Parameters::NameType& name, Parameters::IntType& value)
  {
    typedef std::pair<Parameters::NameType, Parameters::IntType> Name2Val;
    static const Name2Val DEFAULTS[] = {
        Name2Val(Parameters::ZXTune::Sound::FREQUENCY, Parameters::ZXTune::Sound::FREQUENCY_DEFAULT),
        Name2Val(Parameters::ZXTune::Core::AYM::CLOCKRATE, Parameters::ZXTune::Core::AYM::CLOCKRATE_DEFAULT),
        Name2Val(Parameters::ZXTune::Sound::FRAMEDURATION, Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT),
    };
    for (const auto& def : DEFAULTS)
    {
      if (def.first == name)
      {
        value = def.second;
        return true;
      }
    }
    return false;
  }
}  // namespace

const char* ZXTune_GetVersion()
{
  static const String VERSION = Platform::Version::GetProgramVersionString();
  return VERSION.c_str();
}

ZXTuneHandle ZXTune_CreateData(const void* data, size_t size)
{
  try
  {
    const Binary::Container::Ptr result = Binary::CreateContainer(data, size);
    return ContainersCache::Instance().Add(result);
  }
  catch (const std::exception&)
  {
    return ZXTuneHandle();
  }
}

void ZXTune_CloseData(ZXTuneHandle data)
{
  ContainersCache::Instance().Delete(data);
}

ZXTuneHandle ZXTune_OpenModule(ZXTuneHandle data)
{
  try
  {
    const Parameters::Container::Ptr params = Parameters::Container::Create();
    const Binary::Container::Ptr src = ContainersCache::Instance().Get(data);
    const Module::Holder::Ptr result = Module::Open(*params, *src);
    return ModulesCache::Instance().Add(result);
  }
  catch (const Error&)
  {
    return ZXTuneHandle();
  }
  catch (const std::exception&)
  {
    return ZXTuneHandle();
  }
}

void ZXTune_CloseModule(ZXTuneHandle module)
{
  ModulesCache::Instance().Delete(module);
}

bool ZXTune_GetModuleInfo(ZXTuneHandle module, ZXTuneModuleInfo* info)
{
  try
  {
    Require(info != 0);
    const Module::Holder::Ptr holder = ModulesCache::Instance().Get(module);
    const Module::Information::Ptr modinfo = holder->GetModuleInformation();
    info->Positions = modinfo->PositionsCount();
    info->LoopPosition = modinfo->LoopPosition();
    info->Frames = modinfo->FramesCount();
    info->LoopFrame = modinfo->LoopFrame();
    info->Channels = modinfo->ChannelsCount();
    return true;
  }
  catch (const Error&)
  {
    return false;
  }
  catch (const std::exception&)
  {
    return false;
  }
}

ZXTuneHandle ZXTune_CreatePlayer(ZXTuneHandle module)
{
  try
  {
    const Module::Holder::Ptr holder = ModulesCache::Instance().Get(module);
    const PlayerWrapper::Ptr result = PlayerWrapper::Create(holder);
    return PlayersCache::Instance().Add(result);
  }
  catch (const Error&)
  {
    return ZXTuneHandle();
  }
  catch (const std::exception&)
  {
    return ZXTuneHandle();
  }
}

void ZXTune_DestroyPlayer(ZXTuneHandle player)
{
  PlayersCache::Instance().Delete(player);
}

// returns actually rendered bytes
int ZXTune_RenderSound(ZXTuneHandle player, void* buffer, size_t samples)
{
  try
  {
    const PlayerWrapper::Ptr wrapper = PlayersCache::Instance().Get(player);
    return wrapper->RenderSound(static_cast<Sound::Sample*>(buffer), samples);
  }
  catch (const Error&)
  {
    return -1;
  }
  catch (const std::exception&)
  {
    return -1;
  }
}

int ZXTune_SeekSound(ZXTuneHandle player, size_t sample)
{
  try
  {
    const PlayerWrapper::Ptr wrapper = PlayersCache::Instance().Get(player);
    return wrapper->Seek(sample);
  }
  catch (const Error&)
  {
    return -1;
  }
  catch (const std::exception&)
  {
    return -1;
  }
}

bool ZXTune_ResetSound(ZXTuneHandle player)
{
  try
  {
    const PlayerWrapper::Ptr wrapper = PlayersCache::Instance().Get(player);
    wrapper->Reset();
    return true;
  }
  catch (const Error&)
  {
    return false;
  }
  catch (const std::exception&)
  {
    return false;
  }
}

bool ZXTune_GetPlayerParameterInt(ZXTuneHandle player, const char* paramName, int* paramValue)
{
  try
  {
    Require(paramValue != 0);
    const PlayerWrapper::Ptr wrapper = PlayersCache::Instance().Get(player);
    const Parameters::Accessor::Ptr props = wrapper->GetParameters();
    const Parameters::NameType name(paramName);
    Parameters::IntType value;
    if (!props->FindValue(name, value) && !FindDefaultValue(name, value))
    {
      return false;
    }
    *paramValue = static_cast<int>(value);
    return true;
  }
  catch (const Error&)
  {
    return false;
  }
  catch (const std::exception&)
  {
    return false;
  }
}

bool ZXTune_SetPlayerParameterInt(ZXTuneHandle player, const char* paramName, int paramValue)
{
  try
  {
    const PlayerWrapper::Ptr wrapper = PlayersCache::Instance().Get(player);
    const Parameters::Modifier::Ptr props = wrapper->GetParameters();
    const Parameters::NameType name(paramName);
    props->SetValue(name, paramValue);
    return true;
  }
  catch (const Error&)
  {
    return false;
  }
  catch (const std::exception&)
  {
    return false;
  }
}
