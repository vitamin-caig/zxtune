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
  class BackendsStorage;

  //forward declaration of supported backends
  void RegisterNullBackend(BackendsStorage& storage);
  void RegisterWavBackend(BackendsStorage& storage);
  void RegisterMp3Backend(BackendsStorage& storage);
  void RegisterOggBackend(BackendsStorage& storage);
  void RegisterFlacBackend(BackendsStorage& storage);
  void RegisterDirectSoundBackend(BackendsStorage& storage);
  void RegisterWin32Backend(BackendsStorage& storage);
  void RegisterOssBackend(BackendsStorage& storage);
  void RegisterAlsaBackend(BackendsStorage& storage);
  void RegisterSdlBackend(BackendsStorage& storage);

  inline void RegisterSystemBackends(BackendsStorage& storage)
  {
    RegisterOssBackend(storage);
    RegisterAlsaBackend(storage);
    RegisterDirectSoundBackend(storage);
    RegisterWin32Backend(storage);
    RegisterSdlBackend(storage);
  }

  inline void RegisterFileBackends(BackendsStorage& storage)
  {
    RegisterWavBackend(storage);
    RegisterMp3Backend(storage);
    RegisterOggBackend(storage);
    RegisterFlacBackend(storage);
  }

  inline void RegisterAllBackends(BackendsStorage& storage)
  {
    //potentially unsafe backends
    RegisterSystemBackends(storage);
    //stub
    RegisterNullBackend(storage);
    //never default backends
    RegisterFileBackends(storage);
  }
}

#endif //__SOUND_BACKENDS_LIST_H_DEFINED__
