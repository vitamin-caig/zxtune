/*
Abstract:
  Sound backends list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef SOUND_BACKENDS_LIST_H_DEFINED
#define SOUND_BACKENDS_LIST_H_DEFINED

namespace Sound
{
  class BackendsEnumerator;

  //forward declaration of supported backends
  void RegisterNullBackend(BackendsEnumerator& enumerator);
  void RegisterWavBackend(BackendsEnumerator& enumerator);
  void RegisterMp3Backend(BackendsEnumerator& enumerator);
  void RegisterOggBackend(BackendsEnumerator& enumerator);
  void RegisterFlacBackend(BackendsEnumerator& enumerator);
  void RegisterDirectSoundBackend(BackendsEnumerator& enumerator);
  void RegisterWin32Backend(BackendsEnumerator& enumerator);
  void RegisterOssBackend(BackendsEnumerator& enumerator);
  void RegisterAlsaBackend(BackendsEnumerator& enumerator);
  void RegisterSdlBackend(BackendsEnumerator& enumerator);

  inline void RegisterBackends(BackendsEnumerator& enumerator)
  {
    //potentially unsafe backends
    RegisterOssBackend(enumerator);
    RegisterAlsaBackend(enumerator);
    RegisterDirectSoundBackend(enumerator);
    RegisterWin32Backend(enumerator);
    RegisterSdlBackend(enumerator);
    //stub
    RegisterNullBackend(enumerator);
    //never default backends
    RegisterWavBackend(enumerator);
    RegisterMp3Backend(enumerator);
    RegisterOggBackend(enumerator);
    RegisterFlacBackend(enumerator);
  }
}

#endif //__SOUND_BACKENDS_LIST_H_DEFINED__
