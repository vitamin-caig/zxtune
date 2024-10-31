/**
 *
 * @file
 *
 * @brief  SDL subsystem API gate interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <SDL/SDL.h>

#include <memory>

namespace Sound::Sdl
{
  class Api
  {
  public:
    using Ptr = std::shared_ptr<Api>;
    virtual ~Api() = default;

    // clang-format off

    virtual char* SDL_GetError() = 0;
    virtual const SDL_version* SDL_Linked_Version() = 0;
    virtual int SDL_Init(Uint32 flags) = 0;
    virtual int SDL_InitSubSystem(Uint32 flags) = 0;
    virtual void SDL_QuitSubSystem(Uint32 flags) = 0;
    virtual Uint32 SDL_WasInit(Uint32 flags) = 0;
    virtual void SDL_Quit() = 0;
    virtual int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) = 0;
    virtual void SDL_PauseAudio(int pause_on) = 0;
    virtual void SDL_CloseAudio() = 0;
    // clang-format on
  };

  // throw exception in case of error
  Api::Ptr LoadDynamicApi();
}  // namespace Sound::Sdl
