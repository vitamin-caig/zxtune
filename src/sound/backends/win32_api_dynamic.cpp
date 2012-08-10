/*
Abstract:
  Win32 subsystem api dynamic gate implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "win32_api.h"
//common includes
#include <debug_log.h>
#include <shared_library_adapter.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune::Sound::Win32;

  const Debug::Stream Dbg("Sound::Backend::Win32");

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(SharedLibrary::Ptr lib)
      : Lib(lib)
    {
      Dbg("Library loaded");
    }

    virtual ~DynamicApi()
    {
      Dbg("Library unloaded");
    }

    
    virtual UINT waveOutGetNumDevs()
    {
      static const char NAME[] = "waveOutGetNumDevs";
      typedef UINT (WINAPI *FunctionType)();
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func();
    }
    
    virtual MMRESULT waveOutGetDevCapsA(UINT_PTR uDeviceID, LPWAVEOUTCAPSA pwoc, UINT cbwoc)
    {
      static const char NAME[] = "waveOutGetDevCapsA";
      typedef MMRESULT (WINAPI *FunctionType)(UINT_PTR, LPWAVEOUTCAPSA, UINT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(uDeviceID, pwoc, cbwoc);
    }
    
    virtual MMRESULT waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
    {
      static const char NAME[] = "waveOutOpen";
      typedef MMRESULT (WINAPI *FunctionType)(LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(phwo, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen);
    }
    
    virtual MMRESULT waveOutClose(HWAVEOUT hwo)
    {
      static const char NAME[] = "waveOutClose";
      typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo);
    }
    
    virtual MMRESULT waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
    {
      static const char NAME[] = "waveOutPrepareHeader";
      typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT, LPWAVEHDR, UINT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo, pwh, cbwh);
    }
    
    virtual MMRESULT waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
    {
      static const char NAME[] = "waveOutUnprepareHeader";
      typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT, LPWAVEHDR, UINT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo, pwh, cbwh);
    }
    
    virtual MMRESULT waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, IN UINT cbwh)
    {
      static const char NAME[] = "waveOutWrite";
      typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT, LPWAVEHDR, IN UINT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo, pwh, cbwh);
    }
    
    virtual MMRESULT waveOutGetErrorTextA(MMRESULT mmrError, LPSTR pszText, UINT cchText)
    {
      static const char NAME[] = "waveOutGetErrorTextA";
      typedef MMRESULT (WINAPI *FunctionType)(MMRESULT, LPSTR, UINT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(mmrError, pszText, cchText);
    }
    
    virtual MMRESULT waveOutPause(HWAVEOUT hwo)
    {
      static const char NAME[] = "waveOutPause";
      typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo);
    }
    
    virtual MMRESULT waveOutRestart(HWAVEOUT hwo)
    {
      static const char NAME[] = "waveOutRestart";
      typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo);
    }
    
    virtual MMRESULT waveOutReset(HWAVEOUT hwo)
    {
      static const char NAME[] = "waveOutReset";
      typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo);
    }
    
    virtual MMRESULT waveOutGetVolume(HWAVEOUT hwo, LPDWORD pdwVolume)
    {
      static const char NAME[] = "waveOutGetVolume";
      typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT, LPDWORD);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo, pdwVolume);
    }
    
    virtual MMRESULT waveOutSetVolume(HWAVEOUT hwo, DWORD dwVolume)
    {
      static const char NAME[] = "waveOutSetVolume";
      typedef MMRESULT (WINAPI *FunctionType)(HWAVEOUT, DWORD);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo, dwVolume);
    }
    
  private:
    const SharedLibraryAdapter Lib;
  };

}

namespace ZXTune
{
  namespace Sound
  {
    namespace Win32
    {
      Api::Ptr LoadDynamicApi()
      {
        const SharedLibrary::Ptr lib = SharedLibrary::Load("winmm");
        return boost::make_shared<DynamicApi>(lib);
      }
    }
  }
}
