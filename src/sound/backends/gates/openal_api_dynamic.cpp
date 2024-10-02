/**
 *
 * @file
 *
 * @brief  OpenAL subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/gates/openal_api.h"
// common includes
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound::OpenAl
{
  class LibraryName : public Platform::SharedLibrary::Name
  {
  public:
    LibraryName() = default;

    StringView Base() const override
    {
      return "openal"sv;
    }

    std::vector<StringView> PosixAlternatives() const override
    {
      return {"libopenal.so.1"sv, "OpenAL.framework/OpenAL"sv};
    }

    std::vector<StringView> WindowsAlternatives() const override
    {
      return {"OpenAL32.dll"sv};
    }
  };

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
      : Lib(std::move(lib))
    {
      Debug::Log("Sound::Backend::OpenAL", "Library loaded");
    }

    ~DynamicApi() override
    {
      Debug::Log("Sound::Backend::OpenAL", "Library unloaded");
    }

    // clang-format off

    ALCdevice* alcOpenDevice(const ALCchar* devicename) override
    {
      using FunctionType = decltype(&::alcOpenDevice);
      const auto func = Lib.GetSymbol<FunctionType>("alcOpenDevice");
      return func(devicename);
    }

    ALCboolean alcCloseDevice(ALCdevice* device) override
    {
      using FunctionType = decltype(&::alcCloseDevice);
      const auto func = Lib.GetSymbol<FunctionType>("alcCloseDevice");
      return func(device);
    }

    ALCcontext* alcCreateContext(ALCdevice* device, ALCint* attrlist) override
    {
      using FunctionType = decltype(&::alcCreateContext);
      const auto func = Lib.GetSymbol<FunctionType>("alcCreateContext");
      return func(device, attrlist);
    }

    ALCboolean alcMakeContextCurrent(ALCcontext* context) override
    {
      using FunctionType = decltype(&::alcMakeContextCurrent);
      const auto func = Lib.GetSymbol<FunctionType>("alcMakeContextCurrent");
      return func(context);
    }

    ALCcontext* alcGetCurrentContext() override
    {
      using FunctionType = decltype(&::alcGetCurrentContext);
      const auto func = Lib.GetSymbol<FunctionType>("alcGetCurrentContext");
      return func();
    }

    void alcDestroyContext(ALCcontext* context) override
    {
      using FunctionType = decltype(&::alcDestroyContext);
      const auto func = Lib.GetSymbol<FunctionType>("alcDestroyContext");
      return func(context);
    }

    void alGenBuffers(ALsizei n, ALuint* buffers) override
    {
      using FunctionType = decltype(&::alGenBuffers);
      const auto func = Lib.GetSymbol<FunctionType>("alGenBuffers");
      return func(n, buffers);
    }

    void alDeleteBuffers(ALsizei n, ALuint* buffers) override
    {
      using FunctionType = decltype(&::alDeleteBuffers);
      const auto func = Lib.GetSymbol<FunctionType>("alDeleteBuffers");
      return func(n, buffers);
    }

    void alBufferData(ALuint buffer, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq) override
    {
      using FunctionType = decltype(&::alBufferData);
      const auto func = Lib.GetSymbol<FunctionType>("alBufferData");
      return func(buffer, format, data, size, freq);
    }

    void alGenSources(ALsizei n, ALuint* sources) override
    {
      using FunctionType = decltype(&::alGenSources);
      const auto func = Lib.GetSymbol<FunctionType>("alGenSources");
      return func(n, sources);
    }

    void alDeleteSources(ALsizei n, ALuint *sources) override
    {
      using FunctionType = decltype(&::alDeleteSources);
      const auto func = Lib.GetSymbol<FunctionType>("alDeleteSources");
      return func(n, sources);
    }

    void alSourceQueueBuffers(ALuint source, ALsizei n, ALuint* buffers) override
    {
      using FunctionType = decltype(&::alSourceQueueBuffers);
      const auto func = Lib.GetSymbol<FunctionType>("alSourceQueueBuffers");
      return func(source, n, buffers);
    }

    void alSourceUnqueueBuffers(ALuint source, ALsizei n, ALuint* buffers) override
    {
      using FunctionType = decltype(&::alSourceUnqueueBuffers);
      const auto func = Lib.GetSymbol<FunctionType>("alSourceUnqueueBuffers");
      return func(source, n, buffers);
    }

    void alSourcePlay(ALuint source) override
    {
      using FunctionType = decltype(&::alSourcePlay);
      const auto func = Lib.GetSymbol<FunctionType>("alSourcePlay");
      return func(source);
    }

    void alSourceStop(ALuint source) override
    {
      using FunctionType = decltype(&::alSourceStop);
      const auto func = Lib.GetSymbol<FunctionType>("alSourceStop");
      return func(source);
    }

    void alSourcePause(ALuint source) override
    {
      using FunctionType = decltype(&::alSourcePause);
      const auto func = Lib.GetSymbol<FunctionType>("alSourcePause");
      return func(source);
    }

    void alGetSourcei(ALuint source, ALenum pname, ALint* value) override
    {
      using FunctionType = decltype(&::alGetSourcei);
      const auto func = Lib.GetSymbol<FunctionType>("alGetSourcei");
      return func(source, pname, value);
    }

    void alSourcef(ALuint source, ALenum pname, ALfloat value) override
    {
      using FunctionType = decltype(&::alSourcef);
      const auto func = Lib.GetSymbol<FunctionType>("alSourcef");
      return func(source, pname, value);
    }

    void alGetSourcef(ALuint source, ALenum pname, ALfloat* value) override
    {
      using FunctionType = decltype(&::alGetSourcef);
      const auto func = Lib.GetSymbol<FunctionType>("alGetSourcef");
      return func(source, pname, value);
    }

    const ALchar* alGetString(ALenum param) override
    {
      using FunctionType = decltype(&::alGetString);
      const auto func = Lib.GetSymbol<FunctionType>("alGetString");
      return func(param);
    }

    const ALCchar* alcGetString(ALCdevice* device, ALenum param) override
    {
      using FunctionType = decltype(&::alcGetString);
      const auto func = Lib.GetSymbol<FunctionType>("alcGetString");
      return func(device, param);
    }

    ALenum alGetError() override
    {
      using FunctionType = decltype(&::alGetError);
      const auto func = Lib.GetSymbol<FunctionType>("alGetError");
      return func();
    }

    // clang-format on
  private:
    const Platform::SharedLibraryAdapter Lib;
  };

  Api::Ptr LoadDynamicApi()
  {
    static const LibraryName NAME;
    auto lib = Platform::SharedLibrary::Load(NAME);
    return MakePtr<DynamicApi>(std::move(lib));
  }
}  // namespace Sound::OpenAl
