/**
 *
 * @file
 *
 * @brief  Win32 subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/gates/win32_api.h"

#include "debug/log.h"
#include "platform/shared_library_adapter.h"

#include "make_ptr.h"

namespace Sound::Win32
{
  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
      : Lib(std::move(lib))
    {
      Debug::Log("Sound::Backend::Win32", "Library loaded");
    }

    ~DynamicApi() override
    {
      Debug::Log("Sound::Backend::Win32", "Library unloaded");
    }

    // clang-format off

    UINT waveOutGetNumDevs() override
    {
      using FunctionType = decltype(&::waveOutGetNumDevs);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutGetNumDevs");
      return func();
    }

    MMRESULT waveOutGetDevCapsW(UINT_PTR uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc) override
    {
      using FunctionType = decltype(&::waveOutGetDevCapsW);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutGetDevCapsW");
      return func(uDeviceID, pwoc, cbwoc);
    }

    MMRESULT waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen) override
    {
      using FunctionType = decltype(&::waveOutOpen);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutOpen");
      return func(phwo, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen);
    }

    MMRESULT waveOutClose(HWAVEOUT hwo) override
    {
      using FunctionType = decltype(&::waveOutClose);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutClose");
      return func(hwo);
    }

    MMRESULT waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) override
    {
      using FunctionType = decltype(&::waveOutPrepareHeader);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutPrepareHeader");
      return func(hwo, pwh, cbwh);
    }

    MMRESULT waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) override
    {
      using FunctionType = decltype(&::waveOutUnprepareHeader);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutUnprepareHeader");
      return func(hwo, pwh, cbwh);
    }

    MMRESULT waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, IN UINT cbwh) override
    {
      using FunctionType = decltype(&::waveOutWrite);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutWrite");
      return func(hwo, pwh, cbwh);
    }

    MMRESULT waveOutGetErrorTextA(MMRESULT mmrError, LPSTR pszText, UINT cchText) override
    {
      using FunctionType = decltype(&::waveOutGetErrorTextA);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutGetErrorTextA");
      return func(mmrError, pszText, cchText);
    }

    MMRESULT waveOutPause(HWAVEOUT hwo) override
    {
      using FunctionType = decltype(&::waveOutPause);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutPause");
      return func(hwo);
    }

    MMRESULT waveOutRestart(HWAVEOUT hwo) override
    {
      using FunctionType = decltype(&::waveOutRestart);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutRestart");
      return func(hwo);
    }

    MMRESULT waveOutReset(HWAVEOUT hwo) override
    {
      using FunctionType = decltype(&::waveOutReset);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutReset");
      return func(hwo);
    }

    MMRESULT waveOutGetVolume(HWAVEOUT hwo, LPDWORD pdwVolume) override
    {
      using FunctionType = decltype(&::waveOutGetVolume);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutGetVolume");
      return func(hwo, pdwVolume);
    }

    MMRESULT waveOutSetVolume(HWAVEOUT hwo, DWORD dwVolume) override
    {
      using FunctionType = decltype(&::waveOutSetVolume);
      const auto func = Lib.GetSymbol<FunctionType>("waveOutSetVolume");
      return func(hwo, dwVolume);
    }

    // clang-format on
  private:
    const Platform::SharedLibraryAdapter Lib;
  };

  Api::Ptr LoadDynamicApi()
  {
    auto lib = Platform::SharedLibrary::Load("winmm"sv);
    return MakePtr<DynamicApi>(std::move(lib));
  }
}  // namespace Sound::Win32
