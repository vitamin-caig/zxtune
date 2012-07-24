/*
Abstract:
  Win32 subsystem api gate interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef SOUND_BACKENDS_WIN32_API_H_DEFINED
#define SOUND_BACKENDS_WIN32_API_H_DEFINED

//platform-dependent includes
#include <windows.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace ZXTune
{
  namespace Sound
  {
    namespace Win32
    {
      class Api
      {
      public:
        typedef boost::shared_ptr<Api> Ptr;
        virtual ~Api() {}

        
        virtual UINT waveOutGetNumDevs() = 0;
        virtual MMRESULT waveOutGetDevCaps(UINT_PTR uDeviceID, LPWAVEOUTCAPS pwoc, UINT cbwoc) = 0;
        virtual MMRESULT waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen) = 0;
        virtual MMRESULT waveOutClose(HWAVEOUT hwo) = 0;
        virtual MMRESULT waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) = 0;
        virtual MMRESULT waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) = 0;
        virtual MMRESULT waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, IN UINT cbwh) = 0;
        virtual MMRESULT waveOutGetErrorText(MMRESULT mmrError, LPSTR pszText, UINT cchText) = 0;
        virtual MMRESULT waveOutPause(HWAVEOUT hwo) = 0;
        virtual MMRESULT waveOutRestart(HWAVEOUT hwo) = 0;
        virtual MMRESULT waveOutReset(HWAVEOUT hwo) = 0;
        virtual MMRESULT waveOutGetVolume(HWAVEOUT hwo, LPDWORD pdwVolume) = 0;
        virtual MMRESULT waveOutSetVolume(HWAVEOUT hwo, DWORD dwVolume) = 0;
      };

      //throw exception in case of error
      Api::Ptr LoadDynamicApi();

    }
  }
}

#endif //SOUND_BACKENDS_WIN32_API_H_DEFINED
