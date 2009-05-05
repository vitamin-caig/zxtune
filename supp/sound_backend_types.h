#ifndef __SOUND_BACKEND_TYPES_H_DEFINED__
#define __SOUND_BACKEND_TYPES_H_DEFINED__

#include "sound_backend.h"

namespace ZXTune
{
  namespace Sound
  {
    /// Platform-independent backends
    Backend::Ptr CreateFileBackend();
    Backend::Ptr CreateSDLBackend();

    /// Windows-specific backends
    Backend::Ptr CreateWinAPIBackend();
    Backend::Ptr CreateDirectXBackend();

    /// *nix-specific backends
    Backend::Ptr CreateAlsaBackend();
    Backend::Ptr CreateOSSBackend();
    Backend::Ptr CreatePhononBackend();
  }
}


#endif //__SOUND_BACKEND_TYPES_H_DEFINED__
