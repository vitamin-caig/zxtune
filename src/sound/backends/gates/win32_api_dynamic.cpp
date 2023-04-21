/**
 *
 * @file
 *
 * @brief  Win32 subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/gates/win32_api.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound
{
  namespace Win32
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

      
      UINT waveOutGetNumDevs() override
      {
        static const char NAME[] = "waveOutGetNumDevs";
        typedef UINT (WINAPI *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      MMRESULT waveOutGetDevCapsW(UINT_PTR uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc) override
      {
        static const char NAME[] = "waveOutGetDevCapsW";
        typedef MMRESULT (WINAPI *FunctionType)(UINT_PTR, LPWAVEOUTCAPSW, UINT);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(uDeviceID, pwoc, cbwoc);
      }
      
      MMRESULT waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen) override
      {
        static const char NAME[] = "waveOutOpen";
        typedef MMRESULT (WINAPI *FunctionType)(LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(phwo, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen);
      }
      
      MMRESULT waveOutClose(HWAVEOUT hwo) override
      {
        static const char NAME[] = "waveOutClose";
        typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(hwo);
      }
      
      MMRESULT waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) override
      {
        static const char NAME[] = "waveOutPrepareHeader";
        typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT, LPWAVEHDR, UINT);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(hwo, pwh, cbwh);
      }
      
      MMRESULT waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) override
      {
        static const char NAME[] = "waveOutUnprepareHeader";
        typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT, LPWAVEHDR, UINT);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(hwo, pwh, cbwh);
      }
      
      MMRESULT waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, IN UINT cbwh) override
      {
        static const char NAME[] = "waveOutWrite";
        typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT, LPWAVEHDR, IN UINT);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(hwo, pwh, cbwh);
      }
      
      MMRESULT waveOutGetErrorTextA(MMRESULT mmrError, LPSTR pszText, UINT cchText) override
      {
        static const char NAME[] = "waveOutGetErrorTextA";
        typedef MMRESULT (WINAPI *FunctionType)(MMRESULT, LPSTR, UINT);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(mmrError, pszText, cchText);
      }
      
      MMRESULT waveOutPause(HWAVEOUT hwo) override
      {
        static const char NAME[] = "waveOutPause";
        typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(hwo);
      }
      
      MMRESULT waveOutRestart(HWAVEOUT hwo) override
      {
        static const char NAME[] = "waveOutRestart";
        typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(hwo);
      }
      
      MMRESULT waveOutReset(HWAVEOUT hwo) override
      {
        static const char NAME[] = "waveOutReset";
        typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(hwo);
      }
      
      MMRESULT waveOutGetVolume(HWAVEOUT hwo, LPDWORD pdwVolume) override
      {
        static const char NAME[] = "waveOutGetVolume";
        typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT, LPDWORD);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(hwo, pdwVolume);
      }
      
      MMRESULT waveOutSetVolume(HWAVEOUT hwo, DWORD dwVolume) override
      {
        static const char NAME[] = "waveOutSetVolume";
        typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT, DWORD);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(hwo, dwVolume);
      }
      
    private:
      const Platform::SharedLibraryAdapter Lib;
    };


    Api::Ptr LoadDynamicApi()
    {
      auto lib = Platform::SharedLibrary::Load("winmm"_sv);
      return MakePtr<DynamicApi>(std::move(lib));
    }
  }  // namespace Win32
}  // namespace Sound
