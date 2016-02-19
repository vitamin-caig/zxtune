/**
*
* @file
*
* @brief  OpenAL subsystem API gate implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "openal_api.h"
//common includes
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>
//boost includes
#include <boost/range/end.hpp>

namespace Sound
{
  namespace OpenAl
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      virtual std::string Base() const
      {
        return "openal";
      }
      
      virtual std::vector<std::string> PosixAlternatives() const
      {
        static const std::string ALTERNATIVES[] =
        {
          "libopenal.so.1",
        };
        return std::vector<std::string>(ALTERNATIVES, boost::end(ALTERNATIVES));
      }
      
      virtual std::vector<std::string> WindowsAlternatives() const
      {
        static const std::string ALTERNATIVES[] =
        {
          "OpenAL32.dll"
        };
        return std::vector<std::string>(ALTERNATIVES, boost::end(ALTERNATIVES));
      }
    };

    const Debug::Stream Dbg("Sound::Backend::OpenAL");

    class DynamicApi : public Api
    {
    public:
      explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
        : Lib(lib)
      {
        Dbg("Library loaded");
      }

      virtual ~DynamicApi()
      {
        Dbg("Library unloaded");
      }

      
      virtual ALCdevice* alcOpenDevice(const ALCchar* devicename)
      {
        static const char NAME[] = "alcOpenDevice";
        typedef ALCdevice* ( *FunctionType)(const ALCchar*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(devicename);
      }
      
      virtual ALCboolean alcCloseDevice(ALCdevice* device)
      {
        static const char NAME[] = "alcCloseDevice";
        typedef ALCboolean ( *FunctionType)(ALCdevice*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(device);
      }
      
      virtual ALCcontext* alcCreateContext(ALCdevice* device, ALCint* attrlist)
      {
        static const char NAME[] = "alcCreateContext";
        typedef ALCcontext* ( *FunctionType)(ALCdevice*, ALCint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(device, attrlist);
      }
      
      virtual ALCboolean alcMakeContextCurrent(ALCcontext* context)
      {
        static const char NAME[] = "alcMakeContextCurrent";
        typedef ALCboolean ( *FunctionType)(ALCcontext*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(context);
      }
      
      virtual ALCcontext* alcGetCurrentContext()
      {
        static const char NAME[] = "alcGetCurrentContext";
        typedef ALCcontext* ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      virtual void alcDestroyContext(ALCcontext* context)
      {
        static const char NAME[] = "alcDestroyContext";
        typedef void ( *FunctionType)(ALCcontext*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(context);
      }
      
      virtual void alGenBuffers(ALsizei n, ALuint* buffers)
      {
        static const char NAME[] = "alGenBuffers";
        typedef void ( *FunctionType)(ALsizei, ALuint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(n, buffers);
      }
      
      virtual void alDeleteBuffers(ALsizei n, ALuint* buffers)
      {
        static const char NAME[] = "alDeleteBuffers";
        typedef void ( *FunctionType)(ALsizei, ALuint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(n, buffers);
      }
      
      virtual void alBufferData(ALuint buffer, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq)
      {
        static const char NAME[] = "alBufferData";
        typedef void ( *FunctionType)(ALuint, ALenum, const ALvoid*, ALsizei, ALsizei);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(buffer, format, data, size, freq);
      }
      
      virtual void alGenSources(ALsizei n, ALuint* sources)
      {
        static const char NAME[] = "alGenSources";
        typedef void ( *FunctionType)(ALsizei, ALuint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(n, sources);
      }
      
      virtual void alDeleteSources(ALsizei n, ALuint *sources)
      {
        static const char NAME[] = "alDeleteSources";
        typedef void ( *FunctionType)(ALsizei, ALuint *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(n, sources);
      }
      
      virtual void alSourceQueueBuffers(ALuint source, ALsizei n, ALuint* buffers)
      {
        static const char NAME[] = "alSourceQueueBuffers";
        typedef void ( *FunctionType)(ALuint, ALsizei, ALuint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source, n, buffers);
      }
      
      virtual void alSourceUnqueueBuffers(ALuint source, ALsizei n, ALuint* buffers)
      {
        static const char NAME[] = "alSourceUnqueueBuffers";
        typedef void ( *FunctionType)(ALuint, ALsizei, ALuint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source, n, buffers);
      }
      
      virtual void alSourcePlay(ALuint source)
      {
        static const char NAME[] = "alSourcePlay";
        typedef void ( *FunctionType)(ALuint);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source);
      }
      
      virtual void alSourceStop(ALuint source)
      {
        static const char NAME[] = "alSourceStop";
        typedef void ( *FunctionType)(ALuint);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source);
      }
      
      virtual void alSourcePause(ALuint source)
      {
        static const char NAME[] = "alSourcePause";
        typedef void ( *FunctionType)(ALuint);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source);
      }
      
      virtual void alGetSourcei(ALuint source, ALenum pname, ALint* value)
      {
        static const char NAME[] = "alGetSourcei";
        typedef void ( *FunctionType)(ALuint, ALenum, ALint*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source, pname, value);
      }
      
      virtual void alSourcef(ALuint source, ALenum pname, ALfloat value)
      {
        static const char NAME[] = "alSourcef";
        typedef void ( *FunctionType)(ALuint, ALenum, ALfloat);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source, pname, value);
      }
      
      virtual void alGetSourcef(ALuint source, ALenum pname, ALfloat* value)
      {
        static const char NAME[] = "alGetSourcef";
        typedef void ( *FunctionType)(ALuint, ALenum, ALfloat*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(source, pname, value);
      }
      
      virtual const ALchar* alGetString(ALenum param)
      {
        static const char NAME[] = "alGetString";
        typedef const ALchar* ( *FunctionType)(ALenum);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(param);
      }
      
      virtual const ALCchar* alcGetString(ALCdevice* device, ALenum param)
      {
        static const char NAME[] = "alcGetString";
        typedef const ALCchar* ( *FunctionType)(ALCdevice*, ALenum);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(device, param);
      }
      
      virtual ALenum alGetError(void)
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
      const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load(NAME);
      return MakePtr<DynamicApi>(lib);
    }
  }
}
