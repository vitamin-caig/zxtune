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
// library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound
{
  namespace OpenAl
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      LibraryName() {}

      StringView Base() const override
      {
        return "openal"_sv;
      }

      std::vector<StringView> PosixAlternatives() const override
      {
        return {"libopenal.so.1"_sv, "OpenAL.framework/OpenAL"_sv};
      }

      std::vector<StringView> WindowsAlternatives() const override
      {
        return {"OpenAL32.dll"_sv};
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

      
      ALCdevice* alcOpenDevice(const ALCchar* devicename) override
      {
        static const char NAME[] = "alcOpenDevice";
        typedef ALCdevice* ( *FunctionType)(const ALCchar*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(devicename);
      }
      
      ALCboolean alcCloseDevice(ALCdevice* device) override
      {
        static const char NAME[] = "alcCloseDevice";
        typedef ALCboolean ( *FunctionType)(ALCdevice*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(device);
      }
      
      ALCcontext* alcCreateContext(ALCdevice* device, ALCint* attrlist) override
      {
        static const char NAME[] = "alcCreateContext";
        typedef ALCcontext* ( *FunctionType)(ALCdevice*, ALCint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(device, attrlist);
      }
      
      ALCboolean alcMakeContextCurrent(ALCcontext* context) override
      {
        static const char NAME[] = "alcMakeContextCurrent";
        typedef ALCboolean ( *FunctionType)(ALCcontext*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(context);
      }
      
      ALCcontext* alcGetCurrentContext() override
      {
        static const char NAME[] = "alcGetCurrentContext";
        typedef ALCcontext* ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      void alcDestroyContext(ALCcontext* context) override
      {
        static const char NAME[] = "alcDestroyContext";
        typedef void ( *FunctionType)(ALCcontext*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(context);
      }
      
      void alGenBuffers(ALsizei n, ALuint* buffers) override
      {
        static const char NAME[] = "alGenBuffers";
        typedef void ( *FunctionType)(ALsizei, ALuint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(n, buffers);
      }
      
      void alDeleteBuffers(ALsizei n, ALuint* buffers) override
      {
        static const char NAME[] = "alDeleteBuffers";
        typedef void ( *FunctionType)(ALsizei, ALuint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(n, buffers);
      }
      
      void alBufferData(ALuint buffer, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq) override
      {
        static const char NAME[] = "alBufferData";
        typedef void ( *FunctionType)(ALuint, ALenum, const ALvoid*, ALsizei, ALsizei);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(buffer, format, data, size, freq);
      }
      
      void alGenSources(ALsizei n, ALuint* sources) override
      {
        static const char NAME[] = "alGenSources";
        typedef void ( *FunctionType)(ALsizei, ALuint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(n, sources);
      }
      
      void alDeleteSources(ALsizei n, ALuint *sources) override
      {
        static const char NAME[] = "alDeleteSources";
        typedef void ( *FunctionType)(ALsizei, ALuint *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(n, sources);
      }
      
      void alSourceQueueBuffers(ALuint source, ALsizei n, ALuint* buffers) override
      {
        static const char NAME[] = "alSourceQueueBuffers";
        typedef void ( *FunctionType)(ALuint, ALsizei, ALuint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source, n, buffers);
      }
      
      void alSourceUnqueueBuffers(ALuint source, ALsizei n, ALuint* buffers) override
      {
        static const char NAME[] = "alSourceUnqueueBuffers";
        typedef void ( *FunctionType)(ALuint, ALsizei, ALuint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source, n, buffers);
      }
      
      void alSourcePlay(ALuint source) override
      {
        static const char NAME[] = "alSourcePlay";
        typedef void ( *FunctionType)(ALuint);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source);
      }
      
      void alSourceStop(ALuint source) override
      {
        static const char NAME[] = "alSourceStop";
        typedef void ( *FunctionType)(ALuint);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source);
      }
      
      void alSourcePause(ALuint source) override
      {
        static const char NAME[] = "alSourcePause";
        typedef void ( *FunctionType)(ALuint);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source);
      }
      
      void alGetSourcei(ALuint source, ALenum pname, ALint* value) override
      {
        static const char NAME[] = "alGetSourcei";
        typedef void ( *FunctionType)(ALuint, ALenum, ALint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source, pname, value);
      }
      
      void alSourcef(ALuint source, ALenum pname, ALfloat value) override
      {
        static const char NAME[] = "alSourcef";
        typedef void ( *FunctionType)(ALuint, ALenum, ALfloat);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source, pname, value);
      }
      
      void alGetSourcef(ALuint source, ALenum pname, ALfloat* value) override
      {
        static const char NAME[] = "alGetSourcef";
        typedef void ( *FunctionType)(ALuint, ALenum, ALfloat*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source, pname, value);
      }
      
      const ALchar* alGetString(ALenum param) override
      {
        static const char NAME[] = "alGetString";
        typedef const ALchar* ( *FunctionType)(ALenum);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(param);
      }
      
      const ALCchar* alcGetString(ALCdevice* device, ALenum param) override
      {
        static const char NAME[] = "alcGetString";
        typedef const ALCchar* ( *FunctionType)(ALCdevice*, ALenum);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(device, param);
      }
      
      ALenum alGetError(void) override
      {
        static const char NAME[] = "alGetError";
        typedef ALenum ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
    private:
      const Platform::SharedLibraryAdapter Lib;
    };


    Api::Ptr LoadDynamicApi()
    {
      static const LibraryName NAME;
      auto lib = Platform::SharedLibrary::Load(NAME);
      return MakePtr<DynamicApi>(std::move(lib));
    }
  }  // namespace OpenAl
}  // namespace Sound
