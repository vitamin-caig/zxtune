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

// std includes
#include <memory>
// platform-dependent includes
#include <SDL/SDL.h>

namespace Sound
{
  namespace Sdl
  {
    class Api
    {
    public:
      typedef std::shared_ptr<Api> Ptr;
      virtual ~Api() = default;

      
      virtual char* SDL_GetError(void) = 0;
      virtual const SDL_version* SDL_Linked_Version(void) = 0;
      virtual int SDL_Init(Uint32 flags) = 0;
      virtual int SDL_InitSubSystem(Uint32 flags) = 0;
      virtual void SDL_QuitSubSystem(Uint32 flags) = 0;
      virtual Uint32 SDL_WasInit(Uint32 flags) = 0;
      virtual void SDL_Quit(void) = 0;
      virtual int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) = 0;
      virtual void SDL_PauseAudio(int pause_on) = 0;
      virtual void SDL_CloseAudio(void) = 0;
    };

    //throw exception in case of error
    Api::Ptr LoadDynamicApi();

  }
}  // namespace Sound
