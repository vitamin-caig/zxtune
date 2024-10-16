/**
 *
 * @file
 *
 * @brief  SDL subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/gates/sdl_api.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound::Sdl
{
  class LibraryName : public Platform::SharedLibrary::Name
  {
  public:
    LibraryName() = default;

    StringView Base() const override
    {
      return "SDL"_sv;
    }

    std::vector<StringView> PosixAlternatives() const override
    {
      return {"libSDL-1.2.so.0"_sv};
    }

    std::vector<StringView> WindowsAlternatives() const override
    {
      return {};
    }
  };

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
      : Lib(std::move(lib))
    {
      Debug::Log("Sound::Backend::Sdl", "Library loaded");
    }

    ~DynamicApi() override
    {
      Debug::Log("Sound::Backend::Sdl", "Library unloaded");
    }

    // clang-format off

    char* SDL_GetError() override
    {
      using FunctionType = decltype(&::SDL_GetError);
      const auto func = Lib.GetSymbol<FunctionType>("SDL_GetError");
      return func();
    }

    const SDL_version* SDL_Linked_Version() override
    {
      using FunctionType = decltype(&::SDL_Linked_Version);
      const auto func = Lib.GetSymbol<FunctionType>("SDL_Linked_Version");
      return func();
    }

    int SDL_Init(Uint32 flags) override
    {
      using FunctionType = decltype(&::SDL_Init);
      const auto func = Lib.GetSymbol<FunctionType>("SDL_Init");
      return func(flags);
    }

    int SDL_InitSubSystem(Uint32 flags) override
    {
      using FunctionType = decltype(&::SDL_InitSubSystem);
      const auto func = Lib.GetSymbol<FunctionType>("SDL_InitSubSystem");
      return func(flags);
    }

    void SDL_QuitSubSystem(Uint32 flags) override
    {
      using FunctionType = decltype(&::SDL_QuitSubSystem);
      const auto func = Lib.GetSymbol<FunctionType>("SDL_QuitSubSystem");
      return func(flags);
    }

    Uint32 SDL_WasInit(Uint32 flags) override
    {
      using FunctionType = decltype(&::SDL_WasInit);
      const auto func = Lib.GetSymbol<FunctionType>("SDL_WasInit");
      return func(flags);
    }

    void SDL_Quit() override
    {
      using FunctionType = decltype(&::SDL_Quit);
      const auto func = Lib.GetSymbol<FunctionType>("SDL_Quit");
      return func();
    }

    int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) override
    {
      using FunctionType = decltype(&::SDL_OpenAudio);
      const auto func = Lib.GetSymbol<FunctionType>("SDL_OpenAudio");
      return func(desired, obtained);
    }

    void SDL_PauseAudio(int pause_on) override
    {
      using FunctionType = decltype(&::SDL_PauseAudio);
      const auto func = Lib.GetSymbol<FunctionType>("SDL_PauseAudio");
      return func(pause_on);
    }

    void SDL_CloseAudio() override
    {
      using FunctionType = decltype(&::SDL_CloseAudio);
      const auto func = Lib.GetSymbol<FunctionType>("SDL_CloseAudio");
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
}  // namespace Sound::Sdl
