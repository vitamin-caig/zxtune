/**
*
* @file
*
* @brief  SDL subsystem API gate implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "sound/backends/gates/sdl_api.h"
//common includes
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound
{
  namespace Sdl
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      LibraryName()
      {
      }
      
      std::string Base() const override
      {
        return "SDL";
      }
      
      std::vector<std::string> PosixAlternatives() const override
      {
        static const std::string ALTERNATIVES[] =
        {
          "libSDL-1.2.so.0",
        };
        return std::vector<std::string>(ALTERNATIVES, std::end(ALTERNATIVES));
      }
      
      std::vector<std::string> WindowsAlternatives() const override
      {
        return std::vector<std::string>();
      }
    };


    class DynamicApi : public Api
    {
    public:
      explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
        : Lib(lib)
      {
        Debug::Log("Sound::Backend::Sdl", "Library loaded");
      }

      ~DynamicApi() override
      {
        Debug::Log("Sound::Backend::Sdl", "Library unloaded");
      }

      
      char* SDL_GetError(void) override
      {
        static const char NAME[] = "SDL_GetError";
        typedef char* ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      const SDL_version* SDL_Linked_Version(void) override
      {
        static const char NAME[] = "SDL_Linked_Version";
        typedef const SDL_version* ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      int SDL_Init(Uint32 flags) override
      {
        static const char NAME[] = "SDL_Init";
        typedef int ( *FunctionType)(Uint32);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(flags);
      }
      
      int SDL_InitSubSystem(Uint32 flags) override
      {
        static const char NAME[] = "SDL_InitSubSystem";
        typedef int ( *FunctionType)(Uint32);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(flags);
      }
      
      void SDL_QuitSubSystem(Uint32 flags) override
      {
        static const char NAME[] = "SDL_QuitSubSystem";
        typedef void ( *FunctionType)(Uint32);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(flags);
      }
      
      Uint32 SDL_WasInit(Uint32 flags) override
      {
        static const char NAME[] = "SDL_WasInit";
        typedef Uint32 ( *FunctionType)(Uint32);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(flags);
      }
      
      void SDL_Quit(void) override
      {
        static const char NAME[] = "SDL_Quit";
        typedef void ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) override
      {
        static const char NAME[] = "SDL_OpenAudio";
        typedef int ( *FunctionType)(SDL_AudioSpec *, SDL_AudioSpec *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(desired, obtained);
      }
      
      void SDL_PauseAudio(int pause_on) override
      {
        static const char NAME[] = "SDL_PauseAudio";
        typedef void ( *FunctionType)(int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pause_on);
      }
      
      void SDL_CloseAudio(void) override
      {
        static const char NAME[] = "SDL_CloseAudio";
        typedef void ( *FunctionType)();
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
