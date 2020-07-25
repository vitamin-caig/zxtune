/*
 * asap_dsf.cpp - ASAP DirectShow source filter
 *
 * Copyright (C) 2008-2013  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <memory>
#include <stdio.h>
#include <streams.h>
#include <tchar.h>
#ifndef UNDER_CE
#include <shlwapi.h>
#include <initguid.h>
#endif
#include <qnetwork.h>

#ifdef ASAP_DSF_TRANSCODE
// seems it doesn't work
#include <wmpservices.h>
static const IID IID_IWMPTranscodePolicy = { 0xb64cbac3, 0x401c, 0x4327, { 0xa3, 0xe8, 0xb9, 0xfe, 0xb3, 0xa8, 0xc2, 0x5c } };
#endif

#include "asap.h"

static const TCHAR extensions[][5] =
	{ _T(".sap"), _T(".cmc"), _T(".cm3"), _T(".cmr"), _T(".cms"), _T(".dmc"), _T(".dlt"),
	  _T(".mpt"), _T(".mpd"), _T(".rmt"), _T(".tmc"), _T(".tm8"), _T(".tm2"), _T(".fc") };
#define N_EXTS (sizeof(extensions) / sizeof(extensions[0]))
#define EXT_FILTER _T("*.sap;*.cmc;*.cm3;*.cmr;*.cms;*.dmc;*.dlt;*.mpt;*.mpd;*.rmt;*.tmc;*.tm8;*.tm2;*.fc")

#define BITS_PER_SAMPLE      16
#define MIN_BUFFERED_BLOCKS  4096

#define SZ_ASAP_SOURCE       L"ASAP Source Filter"

static const TCHAR CLSID_ASAPSource_str[] = _T("{8E6205A0-19E2-4037-AF32-B29A9B9D0C93}");
static const GUID CLSID_ASAPSource = { 0x8e6205a0, 0x19e2, 0x4037, { 0xaf, 0x32, 0xb2, 0x9a, 0x9b, 0x9d, 0xc, 0x93 } };

class CASAPSourceStream : public CSourceStream, IMediaSeeking
{
	CCritSec m_cs;
	ASAP *m_asap;
	BOOL m_loaded;
	int m_channels;
	int m_song;
	int m_duration;
	LONGLONG m_blocks;

public:

	CASAPSourceStream(HRESULT *phr, CSource *pFilter)
		: CSourceStream(NAME("ASAPSourceStream"), phr, pFilter, L"Out"), m_asap(NULL), m_loaded(FALSE), m_duration(0)
	{
	}

	~CASAPSourceStream()
	{
		ASAP_Delete(m_asap);
	}

	DECLARE_IUNKNOWN

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
	{
		if (riid == IID_IMediaSeeking)
			return GetInterface((IMediaSeeking *) this, ppv);
		return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
	}

	HRESULT InternalPlaySong(int song)
	{
		const ASAPInfo *info = ASAP_GetInfo(m_asap);
		m_duration = ASAPInfo_GetDuration(info, song);
		if (!ASAP_PlaySong(m_asap, song, m_duration)) {
			m_loaded = FALSE;
			return E_FAIL;
		}
		m_song = song;
		m_blocks = 0;
		return S_OK;
	}

	HRESULT Load(LPCOLESTR pszFileName)
	{
		int cbFilename = WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, NULL, 0, NULL, NULL);
		std::auto_ptr<char> filename(new char[cbFilename]);
		CheckPointer(filename.get(), E_OUTOFMEMORY); // apparently Windows CE returns NULL
		if (WideCharToMultiByte(CP_ACP, 0, pszFileName, -1, filename.get(), cbFilename, NULL, NULL) <= 0)
			return HRESULT_FROM_WIN32(GetLastError());

		// read file
		HANDLE fh = CreateFile(
#ifdef UNICODE
			pszFileName,
#else
			filename.get(),
#endif
			GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fh == INVALID_HANDLE_VALUE)
			return HRESULT_FROM_WIN32(GetLastError());
		std::auto_ptr<BYTE> module(new BYTE[ASAPInfo_MAX_MODULE_LENGTH]);
		CheckPointer(module.get(), E_OUTOFMEMORY); // apparently Windows CE returns NULL
		int module_len;
		BOOL ok = ReadFile(fh, module.get(), ASAPInfo_MAX_MODULE_LENGTH, (LPDWORD) &module_len, NULL);
		CloseHandle(fh);
		if (!ok)
			return HRESULT_FROM_WIN32(GetLastError());

		// load into ASAP
		CAutoLock lck(&m_cs);
		if (m_asap == NULL) {
			m_asap = ASAP_New();
			CheckPointer(m_asap, E_OUTOFMEMORY);
		}
		m_loaded = ASAP_Load(m_asap, filename.get(), module.get(), module_len);
		if (!m_loaded)
			return E_FAIL;
		const ASAPInfo *info = ASAP_GetInfo(m_asap);
		m_channels = ASAPInfo_GetChannels(info);
		return InternalPlaySong(ASAPInfo_GetDefaultSong(info));
	}

	HRESULT GetMediaType(CMediaType *pMediaType)
	{
		CheckPointer(pMediaType, E_POINTER);
		CAutoLock lck(&m_cs);
		if (!m_loaded)
			return E_FAIL;
		WAVEFORMATEX *wfx = (WAVEFORMATEX *) pMediaType->AllocFormatBuffer(sizeof(WAVEFORMATEX));
		CheckPointer(wfx, E_OUTOFMEMORY);
		pMediaType->SetType(&MEDIATYPE_Audio);
		pMediaType->SetSubtype(&MEDIASUBTYPE_PCM);
		pMediaType->SetTemporalCompression(FALSE);
		pMediaType->SetSampleSize(m_channels * (BITS_PER_SAMPLE / 8));
		pMediaType->SetFormatType(&FORMAT_WaveFormatEx);
		wfx->wFormatTag = WAVE_FORMAT_PCM;
		wfx->nChannels = m_channels;
		wfx->nSamplesPerSec = ASAP_SAMPLE_RATE;
		wfx->nBlockAlign = m_channels * (BITS_PER_SAMPLE / 8);
		wfx->nAvgBytesPerSec = ASAP_SAMPLE_RATE * wfx->nBlockAlign;
		wfx->wBitsPerSample = BITS_PER_SAMPLE;
		wfx->cbSize = 0;
		return S_OK;
	}

	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest)
	{
		CheckPointer(pAlloc, E_POINTER);
		CheckPointer(pRequest, E_POINTER);
		CAutoLock lck(&m_cs);
		if (!m_loaded)
			return E_FAIL;
		if (pRequest->cBuffers == 0)
			pRequest->cBuffers = 2;
		int bytes = MIN_BUFFERED_BLOCKS * m_channels * (BITS_PER_SAMPLE / 8);
		if (pRequest->cbBuffer < bytes)
			pRequest->cbBuffer = bytes;
		ALLOCATOR_PROPERTIES actual;
		HRESULT hr = pAlloc->SetProperties(pRequest, &actual);
		if (FAILED(hr))
			return hr;
		if (actual.cbBuffer < bytes)
			return E_FAIL;
		return S_OK;
	}

	HRESULT FillBuffer(IMediaSample *pSample)
	{
		CheckPointer(pSample, E_POINTER);
		CAutoLock lck(&m_cs);
		if (!m_loaded)
			return E_FAIL;
		BYTE *pData;
		HRESULT hr = pSample->GetPointer(&pData);
		if (FAILED(hr))
			return hr;
		int cbData = pSample->GetSize();
		cbData = ASAP_Generate(m_asap, pData, cbData, BITS_PER_SAMPLE == 8 ? ASAPSampleFormat_U8 : ASAPSampleFormat_S16_L_E);
		if (cbData == 0)
			return S_FALSE;
		pSample->SetActualDataLength(cbData);
		LONGLONG startTime = m_blocks * UNITS / ASAP_SAMPLE_RATE;
		m_blocks += cbData / (m_channels * (BITS_PER_SAMPLE / 8));
		LONGLONG endTime = m_blocks * UNITS / ASAP_SAMPLE_RATE;
		pSample->SetTime(&startTime, &endTime);
		pSample->SetSyncPoint(TRUE);
		return S_OK;
	}

	STDMETHODIMP GetCapabilities(DWORD *pCapabilities)
	{
		CheckPointer(pCapabilities, E_POINTER);
		*pCapabilities = AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanSeekForwards | AM_SEEKING_CanSeekBackwards | AM_SEEKING_CanGetDuration;
		return S_OK;
	}

	STDMETHODIMP CheckCapabilities(DWORD *pCapabilities)
	{
		CheckPointer(pCapabilities, E_POINTER);
		DWORD result = *pCapabilities & (AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanSeekForwards | AM_SEEKING_CanSeekBackwards | AM_SEEKING_CanGetDuration);
		if (result == *pCapabilities)
			return S_OK;
		*pCapabilities = result;
		if (result != 0)
			return S_FALSE;
		return E_FAIL;
	}

	STDMETHODIMP IsFormatSupported(const GUID *pFormat)
	{
		CheckPointer(pFormat, E_POINTER);
		if (IsEqualGUID(*pFormat, TIME_FORMAT_MEDIA_TIME))
			return S_OK;
		return S_FALSE;
	}

	STDMETHODIMP QueryPreferredFormat(GUID *pFormat)
	{
		CheckPointer(pFormat, E_POINTER);
		*pFormat = TIME_FORMAT_MEDIA_TIME;
		return S_OK;
	}

	STDMETHODIMP GetTimeFormat(GUID *pFormat)
	{
		CheckPointer(pFormat, E_POINTER);
		*pFormat = TIME_FORMAT_MEDIA_TIME;
		return S_OK;
	}

	STDMETHODIMP IsUsingTimeFormat(const GUID *pFormat)
	{
		CheckPointer(pFormat, E_POINTER);
		if (IsEqualGUID(*pFormat, TIME_FORMAT_MEDIA_TIME))
			return S_OK;
		return S_FALSE;
	}

	STDMETHODIMP SetTimeFormat(const GUID *pFormat)
	{
		CheckPointer(pFormat, E_POINTER);
		if (IsEqualGUID(*pFormat, TIME_FORMAT_MEDIA_TIME))
			return S_OK;
		return E_INVALIDARG;
	}

	STDMETHODIMP GetDuration(LONGLONG *pDuration)
	{
		CheckPointer(pDuration, E_POINTER);
		*pDuration = m_duration * (UNITS / MILLISECONDS);
		return S_OK;
	}

	STDMETHODIMP GetStopPosition(LONGLONG *pStop)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags)
	{
		if ((dwCurrentFlags & AM_SEEKING_PositioningBitsMask) == AM_SEEKING_AbsolutePositioning) {
			CheckPointer(pCurrent, E_POINTER);
			int position = (int) (*pCurrent / (UNITS / MILLISECONDS));
			CAutoLock lck(&m_cs);
			ASAP_Seek(m_asap, position);
			m_blocks = 0;
			if ((dwCurrentFlags & AM_SEEKING_ReturnTime) != 0)
				*pCurrent = position * (UNITS / MILLISECONDS);
			return S_OK;
		}
		return E_INVALIDARG;
	}

	STDMETHODIMP GetPositions(LONGLONG *pCurrent, LONGLONG *pStop)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP SetRate(double dRate)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP GetRate(double *dRate)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP GetPreroll(LONGLONG *pllPreroll)
	{
		return E_NOTIMPL;
	}

	HRESULT GetSongs(DWORD *pcSongs)
	{
		CAutoLock lck(&m_cs);
		if (!m_loaded)
			return VFW_E_NOT_CONNECTED;
		const ASAPInfo *info = ASAP_GetInfo(m_asap);
		*pcSongs = ASAPInfo_GetSongs(info);
		return S_OK;
	}

	HRESULT PlaySong(int song)
	{
		CAutoLock lck(&m_cs);
		if (!m_loaded)
			return VFW_E_NOT_CONNECTED;
		return InternalPlaySong(song);
	}

	HRESULT IsSong(int song, DWORD *pdwFlags)
	{
		CAutoLock lck(&m_cs);
		if (!m_loaded)
			return VFW_E_NOT_CONNECTED;
		if (song < 0 || song >= ASAPInfo_GetSongs(ASAP_GetInfo(m_asap)))
			return S_FALSE;
		if (pdwFlags != NULL)
			*pdwFlags = song == m_song ? AMSTREAMSELECTINFO_EXCLUSIVE : 0;
		return S_OK;
	}

	HRESULT GetMetadata(BOOL author, BSTR *pbstr)
	{
		CAutoLock lck(&m_cs);
		if (!m_loaded)
			return VFW_E_NOT_FOUND;
		const ASAPInfo *info = ASAP_GetInfo(m_asap);
		const char *s = author ? ASAPInfo_GetAuthor(info) : ASAPInfo_GetTitle(info);
		// *pbstr = A2BSTR(s); - just don't want dependency on ATL
		int cch = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
		*pbstr = SysAllocStringLen(NULL, cch - 1);
		CheckPointer(*pbstr, E_OUTOFMEMORY);
		if (MultiByteToWideChar(CP_ACP, 0, s, -1, *pbstr, cch) <= 0)
			return HRESULT_FROM_WIN32(GetLastError());
		return S_OK;
	}
};

class CASAPSource : public CSource, IFileSourceFilter, IAMStreamSelect, IAMMediaContent
#ifdef ASAP_DSF_TRANSCODE
	, IWMPTranscodePolicy
#endif
{
	CASAPSourceStream *m_pin;
	LPWSTR m_filename;
	CMediaType m_mt;

	CASAPSource(IUnknown *pUnk, HRESULT *phr)
		: CSource(NAME("ASAPSource"), pUnk, CLSID_ASAPSource), m_pin(NULL), m_filename(NULL)
	{
		m_pin = new CASAPSourceStream(phr, this);
		if (m_pin == NULL && phr != NULL)
			*phr = E_OUTOFMEMORY; // apparently Windows CE returns NULL
	}

	~CASAPSource()
	{
		free(m_filename);
		delete m_pin;
	}

public:

	DECLARE_IUNKNOWN

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
	{
		if (riid == IID_IFileSourceFilter)
			return GetInterface((IFileSourceFilter *) this, ppv);
		if (riid == IID_IAMStreamSelect)
			return GetInterface((IAMStreamSelect *) this, ppv);
		if (riid == IID_IAMMediaContent)
			return GetInterface((IAMMediaContent *) this, ppv);
#ifdef ASAP_DSF_TRANSCODE
		if (riid == IID_IWMPTranscodePolicy)
			return GetInterface((IWMPTranscodePolicy *) this, ppv);
#endif
		return CSource::NonDelegatingQueryInterface(riid, ppv);
	}

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt)
	{
		CheckPointer(pszFileName, E_POINTER);
		HRESULT hr = m_pin->Load(pszFileName);
		if (FAILED(hr))
			return hr;
		free(m_filename);
#ifndef UNDER_CE
		m_filename = _wcsdup(pszFileName);
		CheckPointer(m_filename, E_OUTOFMEMORY);
#else
		size_t cbFileName = (lstrlenW(pszFileName) + 1) * sizeof(WCHAR);
		m_filename = (LPWSTR) malloc(cbFileName);
		CheckPointer(m_filename, E_OUTOFMEMORY);
		CopyMemory(m_filename, pszFileName, cbFileName);
#endif
		if (pmt == NULL) {
			m_mt.SetType(&MEDIATYPE_Stream);
			m_mt.SetSubtype(&MEDIASUBTYPE_NULL);
		}
		else
			m_mt = *pmt;
		return S_OK;
	}

	STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt)
	{
		CheckPointer(ppszFileName, E_POINTER);
		if (m_filename == NULL)
			return E_FAIL;
#ifndef UNDER_CE
		HRESULT hr = SHStrDupW(m_filename, ppszFileName);
		if (FAILED(hr))
			return hr;
#else
		size_t cbFilename = (lstrlenW(m_filename) + 1) * sizeof(WCHAR);
		*ppszFileName = (LPOLESTR) CoTaskMemAlloc(cbFilename);
		CheckPointer(*ppszFileName, E_OUTOFMEMORY);
		CopyMemory(*ppszFileName, m_filename, cbFilename);
#endif
		if (pmt != NULL)
			CopyMediaType(pmt, &m_mt);
		return S_OK;
	}

	STDMETHODIMP Count(DWORD *pcStreams)
	{
		CheckPointer(pcStreams, E_POINTER);
		return m_pin->GetSongs(pcStreams);
	}

	STDMETHODIMP Enable(long lIndex, DWORD dwFlags)
	{
		if (dwFlags != AMSTREAMSELECTENABLE_ENABLE)
			return E_NOTIMPL;
		return m_pin->PlaySong(lIndex);
		/* found this in Musepack Demuxer, doesn't do the trick for Media Player Classic Home Cinema
		HRESULT hr = m_pin->PlaySong(lIndex);
		if (FAILED(hr))
			return hr;
		IMediaSeeking *ms;
		if (m_pGraph != NULL && m_pGraph->QueryInterface(IID_IMediaSeeking, (void **) &ms)) {
			LONGLONG zero = 0;
			ms->SetPositions(&zero, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
			ms->Release();
		}
		return S_OK;
		*/
	}

	STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE **ppmt, DWORD *pdwFlags, LCID *plcid,
		DWORD *pdwGroup, WCHAR **ppszName, IUnknown **ppObject, IUnknown **ppUnk)
	{
		HRESULT hr = m_pin->IsSong(lIndex, pdwFlags);
		if (hr != S_OK)
			return hr;
		if (ppmt != NULL)
			*ppmt = NULL;
		if (plcid != NULL)
			*plcid = 0;
		if (pdwGroup != NULL)
			*pdwGroup = 0;
		if (ppszName != NULL) {
			*ppszName = (LPWSTR) CoTaskMemAlloc(8 * sizeof(WCHAR));
			CheckPointer(*ppszName, E_OUTOFMEMORY);
			swprintf(*ppszName, L"Song %d", lIndex + 1);
		}
		if (ppObject != NULL)
			*ppObject = NULL;
		if (ppUnk != NULL)
			*ppUnk = NULL;
		return S_OK;
	}

	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) { return E_NOTIMPL; }
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) { return E_NOTIMPL; }
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) { return E_NOTIMPL; }
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams,
		VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP get_AuthorName(BSTR* pbstrAuthorName)
	{
		return m_pin->GetMetadata(TRUE, pbstrAuthorName);
	}

	STDMETHODIMP get_Title(BSTR* pbstrTitle)
	{
		return m_pin->GetMetadata(FALSE, pbstrTitle);
	}

	STDMETHODIMP get_Rating(BSTR* pbstrRating) { return E_NOTIMPL; }
	STDMETHODIMP get_Description(BSTR* pbstrDescription) { return E_NOTIMPL; }
	STDMETHODIMP get_Copyright(BSTR* pbstrCopyright) { return E_NOTIMPL; }
	STDMETHODIMP get_BaseURL(BSTR* pbstrBaseURL) { return E_NOTIMPL; }
	STDMETHODIMP get_LogoURL(BSTR* pbstrLogoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_LogoIconURL(BSTR* pbstrLogoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_WatermarkURL(BSTR* pbstrWatermarkURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoURL(BSTR* pbstrMoreInfoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoText(BSTR* pbstrMoreInfoText) { return E_NOTIMPL; }

#ifdef ASAP_DSF_TRANSCODE
	STDMETHODIMP allowTranscode(VARIANT_BOOL *pvarfAllow)
	{
		*pvarfAllow = VARIANT_TRUE;
		return S_OK;
	}
#endif

	static CUnknown * WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr)
	{
		CASAPSource *pNewFilter = new CASAPSource(pUnk, phr);
		if (phr != NULL && pNewFilter == NULL)
			*phr = E_OUTOFMEMORY;
		return pNewFilter;
	}
};

static const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_PCM
};

static const AMOVIESETUP_PIN sudASAPSourceStream =
{
	L"Output",
	FALSE,
	TRUE,
	FALSE,
	FALSE,
	&CLSID_NULL,
	NULL,
	1,
	&sudPinTypes
};

static const AMOVIESETUP_FILTER sudASAPSource =
{
	&CLSID_ASAPSource,
	SZ_ASAP_SOURCE,
	MERIT_NORMAL,
	1,
	&sudASAPSourceStream
};

CFactoryTemplate g_Templates[1] =
{
	{
		SZ_ASAP_SOURCE,
		&CLSID_ASAPSource,
		CASAPSource::CreateInstance,
		NULL,
		&sudASAPSource
	}
};

int g_cTemplates = 1;

static void WriteASAPValue(LPCTSTR lpSubKey, LPCTSTR value)
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpSubKey, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
		return;
	if (value != NULL)
		RegSetValueEx(hKey, _T("ASAP"), 0, REG_SZ, (const BYTE *) value, ((DWORD) _tcslen(value) + 1) * sizeof(TCHAR));
	else
		RegDeleteValue(hKey, _T("ASAP"));
	RegCloseKey(hKey);
}

STDAPI DllRegisterServer()
{
	HRESULT hr = AMovieDllRegisterServer2(TRUE);
	if (FAILED(hr))
		return hr;

	HKEY hMTKey;
	HKEY hWMPKey;
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, _T("Media Type\\Extensions"), 0, NULL, 0, KEY_WRITE, NULL, &hMTKey, NULL) != ERROR_SUCCESS)
		return E_FAIL;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Multimedia\\WMPlayer\\Extensions"), 0, NULL, 0, KEY_WRITE, NULL, &hWMPKey, NULL) != ERROR_SUCCESS)
		return E_FAIL;
	for (int i = 0; i < N_EXTS; i++) {
		HKEY hKey;
		if (RegCreateKeyEx(hMTKey, extensions[i], 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS
		 || RegSetValueEx(hKey, _T("Source Filter"), 0, REG_SZ, (const BYTE *) &CLSID_ASAPSource_str, sizeof(CLSID_ASAPSource_str)) != ERROR_SUCCESS
		 || RegCloseKey(hKey) != ERROR_SUCCESS)
			 return E_FAIL;
#ifdef ASAP_DSF_TRANSCODE
		static const DWORD perms = 15 + 128;
#else
		static const DWORD perms = 15;
#endif
		static const DWORD runtime = 7;
		if (RegCreateKeyEx(hWMPKey, extensions[i], 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS
		 || RegSetValueEx(hKey, _T("Permissions"), 0, REG_DWORD, (const BYTE *) &perms, sizeof(perms)) != ERROR_SUCCESS
		 || RegSetValueEx(hKey, _T("Runtime"), 0, REG_DWORD, (const BYTE *) &runtime, sizeof(runtime)) != ERROR_SUCCESS
		 || RegCloseKey(hKey) != ERROR_SUCCESS)
			 return E_FAIL;
	}
	if (RegCloseKey(hWMPKey) != ERROR_SUCCESS || RegCloseKey(hMTKey) != ERROR_SUCCESS)
		return E_FAIL;
	WriteASAPValue(_T("Software\\Microsoft\\MediaPlayer\\Player\\Extensions\\MUIDescriptions"), _T("ASAP files (") EXT_FILTER _T(")"));
	WriteASAPValue(_T("Software\\Microsoft\\MediaPlayer\\Player\\Extensions\\Types"), EXT_FILTER);
	return S_OK;
}

STDAPI DllUnregisterServer()
{
	HKEY hWMPKey;
	HKEY hMTKey;
	WriteASAPValue(_T("Software\\Microsoft\\MediaPlayer\\Player\\Extensions\\MUIDescriptions"), NULL);
	WriteASAPValue(_T("Software\\Microsoft\\MediaPlayer\\Player\\Extensions\\Types"), NULL);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Multimedia\\WMPlayer\\Extensions"), 0, DELETE, &hWMPKey) != ERROR_SUCCESS)
		return E_FAIL;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, _T("Media Type\\Extensions"), 0, DELETE, &hMTKey) != ERROR_SUCCESS)
		return E_FAIL;
	for (int i = 0; i < N_EXTS; i++) {
		RegDeleteKey(hWMPKey, extensions[i]);
		RegDeleteKey(hMTKey, extensions[i]);
	}
	RegCloseKey(hMTKey);
	RegCloseKey(hWMPKey);

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE) hInstance, dwReason, lpReserved);
}
