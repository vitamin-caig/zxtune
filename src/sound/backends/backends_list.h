/**
 *
 * @file
 *
 * @brief  Backends factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

namespace Sound
{
  class BackendsStorage;

  // forward declaration of supported backends
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
  void RegisterAyLptBackend(BackendsStorage& storage);
  void RegisterPulseAudioBackend(BackendsStorage& storage);
  void RegisterOpenAlBackend(BackendsStorage& storage);

  inline void RegisterSystemBackends(BackendsStorage& storage)
  {
    RegisterOssBackend(storage);
    RegisterAlsaBackend(storage);
    RegisterPulseAudioBackend(storage);
    RegisterDirectSoundBackend(storage);
    RegisterWin32Backend(storage);
    RegisterSdlBackend(storage);
    RegisterOpenAlBackend(storage);
    RegisterAyLptBackend(storage);
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
    // potentially unsafe backends
    RegisterSystemBackends(storage);
    // stub
    RegisterNullBackend(storage);
    // never default backends
    RegisterFileBackends(storage);
  }
}  // namespace Sound
