/**
*
* @file
*
* @brief  PulseAudio subsystem API gate implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "paudio_api.h"
//common includes
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound
{
  namespace PulseAudio
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      virtual std::string Base() const
      {
        return "pulse-simple";
      }
      
      virtual std::vector<std::string> PosixAlternatives() const
      {
        static const std::string ALTERNATIVES[] =
        {
          "libpulse-simple.so.0",
          "libpulse-simple.so.0.1",
          "libpulse-simple.so.0.1.0"
        };
        return std::vector<std::string>(ALTERNATIVES, std::end(ALTERNATIVES));
      }
      
      virtual std::vector<std::string> WindowsAlternatives() const
      {
        return std::vector<std::string>();
      }
    };

    const Debug::Stream Dbg("Sound::Backend::PulseAudio");

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

      
      virtual const char* pa_get_library_version(void)
      {
        static const char NAME[] = "pa_get_library_version";
        typedef const char* ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      virtual const char* pa_strerror(int error)
      {
        static const char NAME[] = "pa_strerror";
        typedef const char* ( *FunctionType)(int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(error);
      }
      
      virtual pa_simple* pa_simple_new(const char* server, const char* name, pa_stream_direction_t dir, const char* dev, const char* stream, const pa_sample_spec* ss, const pa_channel_map* map, const pa_buffer_attr* attr, int* error)
      {
        static const char NAME[] = "pa_simple_new";
        typedef pa_simple* ( *FunctionType)(const char*, const char*, pa_stream_direction_t, const char*, const char*, const pa_sample_spec*, const pa_channel_map*, const pa_buffer_attr*, int*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(server, name, dir, dev, stream, ss, map, attr, error);
      }
      
      virtual int pa_simple_write(pa_simple* s, const void* data, size_t bytes, int* error)
      {
        static const char NAME[] = "pa_simple_write";
        typedef int ( *FunctionType)(pa_simple*, const void*, size_t, int*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(s, data, bytes, error);
      }
      
      virtual int pa_simple_flush(pa_simple* s, int* error)
      {
        static const char NAME[] = "pa_simple_flush";
        typedef int ( *FunctionType)(pa_simple*, int*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(s, error);
      }
      
      virtual void pa_simple_free(pa_simple* s)
      {
        static const char NAME[] = "pa_simple_free";
        typedef void ( *FunctionType)(pa_simple*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(s);
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
