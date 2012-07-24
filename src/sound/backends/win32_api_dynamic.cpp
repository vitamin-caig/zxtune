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
#include <logging.h>
#include <shared_library_adapter.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  const std::string THIS_MODULE("Sound::Backend::Win32");

  using namespace ZXTune::Sound::Win32;

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(SharedLibrary::Ptr lib)
      : Lib(lib)
    {
      Log::Debug(THIS_MODULE, "Library loaded");
    }

    virtual ~DynamicApi()
    {
      Log::Debug(THIS_MODULE, "Library unloaded");
    }

    
    virtual UINT waveOutGetNumDevs()
    {
      static const char* NAME = "waveOutGetNumDevs";
      typedef UINT (*FunctionType)();
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func();
    }
    
    virtual MMRESULT waveOutGetDevCaps(UINT_PTR uDeviceID, LPWAVEOUTCAPS pwoc, UINT cbwoc)
    {
      static const char* NAME = "waveOutGetDevCaps";
      typedef MMRESULT (*FunctionType)(UINT_PTR, LPWAVEOUTCAPS, UINT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(uDeviceID, pwoc, cbwoc);
    }
    
    virtual MMRESULT waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
    {
      static const char* NAME = "waveOutOpen";
      typedef MMRESULT (*FunctionType)(LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(phwo, uDeviceID, pwfx, dwCallback, dwInstance, fdwOpen);
    }
    
    virtual MMRESULT waveOutClose(HWAVEOUT hwo)
    {
      static const char* NAME = "waveOutClose";
      typedef MMRESULT (*FunctionType)(HWAVEOUT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo);
    }
    
    virtual MMRESULT waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
    {
      static const char* NAME = "waveOutPrepareHeader";
      typedef MMRESULT (*FunctionType)(HWAVEOUT, LPWAVEHDR, UINT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo, pwh, cbwh);
    }
    
    virtual MMRESULT waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh)
    {
      static const char* NAME = "waveOutUnprepareHeader";
      typedef MMRESULT (*FunctionType)(HWAVEOUT, LPWAVEHDR, UINT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo, pwh, cbwh);
    }
    
    virtual MMRESULT waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, IN UINT cbwh)
    {
      static const char* NAME = "waveOutWrite";
      typedef MMRESULT (*FunctionType)(HWAVEOUT, LPWAVEHDR, IN UINT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo, pwh, cbwh);
    }
    
    virtual MMRESULT waveOutGetErrorText(MMRESULT mmrError, LPSTR pszText, UINT cchText)
    {
      static const char* NAME = "waveOutGetErrorText";
      typedef MMRESULT (*FunctionType)(MMRESULT, LPSTR, UINT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(mmrError, pszText, cchText);
    }
    
    virtual MMRESULT waveOutPause(HWAVEOUT hwo)
    {
      static const char* NAME = "waveOutPause";
      typedef MMRESULT (*FunctionType)(HWAVEOUT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo);
    }
    
    virtual MMRESULT waveOutRestart(HWAVEOUT hwo)
    {
      static const char* NAME = "waveOutRestart";
      typedef MMRESULT (*FunctionType)(HWAVEOUT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo);
    }
    
    virtual MMRESULT waveOutReset(HWAVEOUT hwo)
    {
      static const char* NAME = "waveOutReset";
      typedef MMRESULT (*FunctionType)(HWAVEOUT);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo);
    }
    
    virtual MMRESULT waveOutGetVolume(HWAVEOUT hwo, LPDWORD pdwVolume)
    {
      static const char* NAME = "waveOutGetVolume";
      typedef MMRESULT (*FunctionType)(HWAVEOUT, LPDWORD);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(hwo, pdwVolume);
    }
    
    virtual MMRESULT waveOutSetVolume(HWAVEOUT hwo, DWORD dwVolume)
    {
      static const char* NAME = "waveOutSetVolume";
      typedef MMRESULT (*FunctionType)(HWAVEOUT, DWORD);
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
