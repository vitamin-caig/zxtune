/*
Abstract:
  Sound backends list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __SOUND_BACKENDS_LIST_H_DEFINED__
#define __SOUND_BACKENDS_LIST_H_DEFINED__

namespace ZXTune
{
  namespace Sound
  {
    class BackendsEnumerator;

    //forward declaration of supported backends
    void RegisterNullBackend(BackendsEnumerator& enumerator);
    void RegisterWAVBackend(BackendsEnumerator& enumerator);
    void RegisterMP3Backend(BackendsEnumerator& enumerator);
    void RegisterOGGBackend(BackendsEnumerator& enumerator);
    void RegisterFLACBackend(BackendsEnumerator& enumerator);
    void RegisterDirectSoundBackend(BackendsEnumerator& enumerator);
    void RegisterWin32Backend(BackendsEnumerator& enumerator);
    void RegisterOSSBackend(BackendsEnumerator& enumerator);
    void RegisterAlsaBackend(BackendsEnumerator& enumerator);
    //void RegisterAYLPTBackend(BackendsEnumerator& enumerator);
    void RegisterSDLBackend(BackendsEnumerator& enumerator);

    inline void RegisterBackends(BackendsEnumerator& enumerator)
    {
      //potentially unsafe backends
      RegisterOSSBackend(enumerator);
      RegisterAlsaBackend(enumerator);
      RegisterDirectSoundBackend(enumerator);
      RegisterWin32Backend(enumerator);
      RegisterSDLBackend(enumerator);
      //stub
      RegisterNullBackend(enumerator);
      //never default backends
      RegisterWAVBackend(enumerator);
      RegisterMP3Backend(enumerator);
      RegisterOGGBackend(enumerator);
      RegisterFLACBackend(enumerator);
      //RegisterAYLPTBackend(enumerator);
    }
  }
}

#endif //__SOUND_BACKENDS_LIST_H_DEFINED__
