/*
 * ASAPShellEx.cpp - ASAP Column Handler and Property Handler shell extensions
 *
 * Copyright (C) 2010-2013  Piotr Fusik
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

/* There are two separate implementations for different Windows versions:
   Column Handler (IColumnProvider) works in Windows XP and 2003 (probably 2000 too and maybe 98).
   Property Handler (IInitializeWithStream+IPropertyStore+IPropertyStoreCapabilities) works in Windows Vista and 7. */

#include <stdio.h>
#define _WIN32_IE 0x500
#include <shlobj.h>
#include <shlwapi.h>

#ifdef __MINGW32__
/* missing in MinGW */

extern "C" const FMTID FMTID_AudioSummaryInformation =
	{ 0x64440490, 0x4c8b, 0x11d1, { 0x8b, 0x70, 0x08, 0x00, 0x36, 0xb1, 0x1a, 0x03 } };

#ifndef __MINGW64__
/* missing just in 32-bit MinGW */
extern "C" const FMTID FMTID_MUSIC =
	{ 0x56a3372e, 0xce9c, 0x11d2, { 0x9f, 0x0e, 0x00, 0x60, 0x97, 0xc6, 0x86, 0xf6 } };
extern "C" const FMTID FMTID_SummaryInformation =
	{ 0xf29f85e0, 0x4ff9, 0x1068, { 0xab, 0x91, 0x08, 0x00, 0x2b, 0x27, 0xb3, 0xd9 } };
#define PIDSI_ARTIST          2
#define PIDSI_YEAR            5
#define PIDASI_TIMELENGTH     3
#define PIDASI_CHANNEL_COUNT  7
#define SHCDF_UPDATEITEM      1

typedef SHCOLUMNID PROPERTYKEY;
#define REFPROPERTYKEY const PROPERTYKEY &
#define REFPROPVARIANT const PROPVARIANT &

#undef INTERFACE

static const IID IID_IInitializeWithStream =
	{ 0xb824b49d, 0x22ac, 0x4161, { 0xac, 0x8a, 0x99, 0x16, 0xe8, 0xfa, 0x3f, 0x7f } };
#define INTERFACE IInitializeWithStream
DECLARE_INTERFACE_(IInitializeWithStream, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;
	STDMETHOD(Initialize)(THIS_ IStream *, DWORD) PURE;
};
#undef INTERFACE

static const IID IID_IPropertyStore =
	{ 0x886d8eeb, 0x8cf2, 0x4446, { 0x8d, 0x02, 0xcd, 0xba, 0x1d, 0xbd, 0xcf, 0x99 } };
#define INTERFACE IPropertyStore
DECLARE_INTERFACE_(IPropertyStore, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;
	STDMETHOD(GetCount)(THIS_ DWORD *) PURE;
	STDMETHOD(GetAt)(THIS_ DWORD, PROPERTYKEY *) PURE;
	STDMETHOD(GetValue)(THIS_ REFPROPERTYKEY, PROPVARIANT *) PURE;
	STDMETHOD(SetValue)(THIS_ REFPROPERTYKEY, REFPROPVARIANT) PURE;
	STDMETHOD(Commit)(THIS) PURE;
};
#undef INTERFACE

static const IID IID_IPropertyStoreCapabilities =
    { 0xc8e2d566, 0x186e, 0x4d49, { 0xbf, 0x41, 0x69, 0x09, 0xea, 0xd5, 0x6a, 0xcc } };
#define INTERFACE IPropertyStoreCapabilities
DECLARE_INTERFACE_(IPropertyStoreCapabilities, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;
	STDMETHOD(IsPropertyWritable)(THIS_ REFPROPERTYKEY) PURE;
};
#undef INTERFACE

#endif
/* end missing in MinGW */
#endif

#include "asap-infowriter.h"

static const char extensions[][5] =
	{ ".sap", ".cmc", ".cm3", ".cmr", ".cms", ".dmc", ".dlt", ".mpt", ".mpd", ".rmt", ".tmc", ".tm8", ".tm2", ".fc" };
#define N_EXTS (int) (sizeof(extensions) / sizeof(extensions[0]))

static HINSTANCE g_hDll;
static LONG g_cRef = 0;
static enum { WINDOWS_OLD, WINDOWS_XP, WINDOWS_VISTA } g_windowsVer;

static void DllAddRef(void)
{
	InterlockedIncrement(&g_cRef);
}

static void DllRelease(void)
{
	InterlockedDecrement(&g_cRef);
}

#define CLSID_ASAPMetadataHandler_str "{5AE26367-B5CF-444D-B163-2CBC99B41287}"
static const GUID CLSID_ASAPMetadataHandler =
	{ 0x5ae26367, 0xb5cf, 0x444d, { 0xb1, 0x63, 0x2c, 0xbc, 0x99, 0xb4, 0x12, 0x87 } };

struct CMyPropertyDef
{
	SHCOLUMNID scid;
	UINT cChars;
	DWORD csFlags;
	LPCWSTR wszTitle;

	void CopyTo(SHCOLUMNINFO *psci) const
	{
		psci->scid = this->scid;
		psci->vt = VT_LPWSTR;
		psci->fmt = LVCFMT_LEFT;
		psci->cChars = this->cChars;
		psci->csFlags = this->csFlags;
		lstrcpyW(psci->wszTitle, this->wszTitle);
		lstrcpyW(psci->wszDescription, this->wszTitle);
	}
};

static const CMyPropertyDef g_propertyDefs[] = {
	{ { FMTID_SummaryInformation, PIDSI_TITLE }, 25, SHCOLSTATE_TYPE_STR | SHCOLSTATE_SLOW, L"Title" },
	{ { FMTID_SummaryInformation, PIDSI_AUTHOR }, 25, SHCOLSTATE_TYPE_STR | SHCOLSTATE_SLOW, L"Author" },
	{ { FMTID_MUSIC, PIDSI_ARTIST }, 25, SHCOLSTATE_TYPE_STR | SHCOLSTATE_SLOW, L"Artist" },
	{ { FMTID_MUSIC, PIDSI_YEAR }, 4, SHCOLSTATE_TYPE_STR | SHCOLSTATE_SLOW, L"Year" },
	{ { FMTID_AudioSummaryInformation, PIDASI_TIMELENGTH }, 8, SHCOLSTATE_TYPE_STR | SHCOLSTATE_SLOW, L"Duration" },
	{ { FMTID_AudioSummaryInformation, PIDASI_CHANNEL_COUNT }, 9, SHCOLSTATE_TYPE_INT | SHCOLSTATE_SLOW, L"Channels" },
	{ { CLSID_ASAPMetadataHandler, 1 }, 8, SHCOLSTATE_TYPE_INT | SHCOLSTATE_SLOW, L"Subsongs" },
	{ { CLSID_ASAPMetadataHandler, 2 }, 8, SHCOLSTATE_TYPE_STR | SHCOLSTATE_SLOW, L"PAL/NTSC" }
};
#define N_PROPERTYDEFS (sizeof(g_propertyDefs) / sizeof(g_propertyDefs[0]))

class CMyLock
{
	PCRITICAL_SECTION m_pLock;

public:

	CMyLock(PCRITICAL_SECTION pLock)
	{
		m_pLock = pLock;
		EnterCriticalSection(pLock);
	}

	~CMyLock()
	{
		LeaveCriticalSection(m_pLock);
	}
};

class CMemoryByteWriter
{
	static void WriteByte(void *obj, int value)
	{
		CMemoryByteWriter *self = (CMemoryByteWriter *) obj;
		if (self->length < ASAPInfo_MAX_MODULE_LENGTH)
			self->module[self->length++] = (byte) value;
	}

public:

	byte module[ASAPInfo_MAX_MODULE_LENGTH];
	int length;

	CMemoryByteWriter() : length(0) { }

	ByteWriter GetByteWriter()
	{
		ByteWriter bw = { this, WriteByte };
		return bw;
	}
};

class CASAPMetadataHandler : IColumnProvider, IInitializeWithStream, IPropertyStore, IPropertyStoreCapabilities
{
	LONG m_cRef;
	CRITICAL_SECTION m_lock;
	WCHAR m_filename[MAX_PATH];
	BOOL m_hasInfo;
	IStream *m_pstream;
	ASAPInfo *m_pinfo;

	~CASAPMetadataHandler()
	{
		ASAPInfo_Delete(m_pinfo);
		if (m_pstream != NULL)
			m_pstream->Release();
		DeleteCriticalSection(&m_lock);
		DllRelease();
	}

	HRESULT LoadFile(LPCWSTR wszFile, IStream *pstream, DWORD grfMode)
	{
		m_hasInfo = FALSE;
		if (m_pstream != NULL) {
			m_pstream->Release();
			m_pstream = NULL;
		}

		int cbFilename = WideCharToMultiByte(CP_ACP, 0, wszFile, -1, NULL, 0, NULL, NULL);
		char filename[cbFilename];
		if (WideCharToMultiByte(CP_ACP, 0, wszFile, -1, filename, cbFilename, NULL, NULL) <= 0)
			return HRESULT_FROM_WIN32(GetLastError());

		byte module[ASAPInfo_MAX_MODULE_LENGTH];
		int module_len;
		if (pstream != NULL) {
			HRESULT hr = pstream->Read(module, ASAPInfo_MAX_MODULE_LENGTH, (ULONG *) &module_len);
			if (FAILED(hr))
				return hr;
		}
		else {
			HANDLE fh = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
			if (fh == INVALID_HANDLE_VALUE)
				return HRESULT_FROM_WIN32(GetLastError());
			if (!ReadFile(fh, module, ASAPInfo_MAX_MODULE_LENGTH, (LPDWORD) &module_len, NULL)) {
				HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
				CloseHandle(fh);
				return hr;
			}
			CloseHandle(fh);
		}

		m_hasInfo = ASAPInfo_Load(m_pinfo, filename, module, module_len);
		if (m_hasInfo && (grfMode & STGM_READWRITE) != 0) {
			const char *ext = strrchr(filename, '.');
			if (ext != NULL && _stricmp(ext, ".sap") == 0) {
				m_pstream = pstream;
				pstream->AddRef();
			}
		}
		return S_OK;
	}

	static HRESULT GetString(PROPVARIANT *pvarData, const char *s)
	{
		pvarData->vt = VT_BSTR;
		// pvarData->bstrVal = A2BSTR(s); - just don't want dependency on ATL
		int cch = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
		pvarData->bstrVal = SysAllocStringLen(NULL, cch - 1);
		if (pvarData->bstrVal == NULL)
			return E_OUTOFMEMORY;
		if (MultiByteToWideChar(CP_ACP, 0, s, -1, pvarData->bstrVal, cch) <= 0)
			return HRESULT_FROM_WIN32(GetLastError());
		return S_OK;
	}

	HRESULT GetAuthors(PROPVARIANT *pvarData, BOOL vista)
	{
		const char *author = ASAPInfo_GetAuthor(m_pinfo);
		if (!vista)
			return GetString(pvarData, author);
		if (author[0] == '\0') {
			pvarData->vt = VT_EMPTY;
			return S_OK;
		}
		// split on " & "
		int i;
		const char *s = author;
		for (i = 1; ; i++) {
			s = strstr(s, " & ");
			if (s == NULL)
				break;
			s += 3;
		}
		pvarData->vt = VT_VECTOR | VT_LPSTR;
		pvarData->calpstr.cElems = i;
		LPSTR *pElems = (LPSTR *) CoTaskMemAlloc(i * sizeof(LPSTR));
		pvarData->calpstr.pElems = pElems;
		if (pElems == NULL)
			return E_OUTOFMEMORY;
		s = author;
		for (i = 0; ; i++) {
			const char *e = strstr(s, " & ");
			size_t len = e != NULL ? e - s : strlen(s);
			pElems[i] = (LPSTR) CoTaskMemAlloc(len + 1);
			if (pElems[i] == NULL)
				return E_OUTOFMEMORY;
			memcpy(pElems[i], s, len);
			pElems[i][len] = '\0';
			if (e == NULL)
				break;
			s = e + 3;
		}
		return S_OK;
	}

	HRESULT GetData(LPCSHCOLUMNID pscid, PROPVARIANT *pvarData, BOOL vista)
	{
		if (!m_hasInfo)
			return S_FALSE;

		if (pscid->fmtid == FMTID_SummaryInformation) {
			if (pscid->pid == PIDSI_TITLE)
				return GetString(pvarData, ASAPInfo_GetTitle(m_pinfo));
			if (pscid->pid == PIDSI_AUTHOR)
				return GetAuthors(pvarData, vista);
		}
		else if (pscid->fmtid == FMTID_MUSIC) {
			if (pscid->pid == PIDSI_ARTIST)
				return GetAuthors(pvarData, vista);
			if (pscid->pid == PIDSI_YEAR) {
				int year = ASAPInfo_GetYear(m_pinfo);
				if (year < 0) {
					pvarData->vt = VT_EMPTY;
					return S_OK;
				}
				pvarData->vt = VT_UI8;
				pvarData->ulVal = (ULONG) year;
				return S_OK;
			}
		}
		else if (pscid->fmtid == FMTID_AudioSummaryInformation) {
			if (pscid->pid == PIDASI_TIMELENGTH) {
				int duration = ASAPInfo_GetDuration(m_pinfo, ASAPInfo_GetDefaultSong(m_pinfo));
				if (duration < 0) {
					pvarData->vt = VT_EMPTY;
					return S_OK;
				}
				if (g_windowsVer == WINDOWS_OLD) {
					duration /= 1000;
					char timeStr[16];
					sprintf(timeStr, "%02d:%02d:%02d", duration / 3600, duration / 60 % 60, duration % 60);
					return GetString(pvarData, timeStr);
				}
				else {
					pvarData->vt = VT_UI8;
					pvarData->uhVal.QuadPart = 10000ULL * duration;
					return S_OK;
				}
			}
			if (pscid->pid == PIDASI_CHANNEL_COUNT) {
				pvarData->vt = VT_UI4;
				pvarData->ulVal = (ULONG) ASAPInfo_GetChannels(m_pinfo);
				return S_OK;
			}
		}
		else if (pscid->fmtid == CLSID_ASAPMetadataHandler) {
			if (pscid->pid == 1) {
				pvarData->vt = VT_UI4;
				pvarData->ulVal = (ULONG) ASAPInfo_GetSongs(m_pinfo);
				return S_OK;
			}
			if (pscid->pid == 2)
				return GetString(pvarData, ASAPInfo_IsNtsc(m_pinfo) ? "NTSC" : "PAL");
		}
		return S_FALSE;
	}

	static HRESULT AppendString(char *dest, int *offset, LPCWSTR wszVal)
	{
		int i = *offset;
		while (*wszVal != 0) {
			if (i >= ASAPInfo_MAX_TEXT_LENGTH) {
				dest[i] = '\0';
				return INPLACE_S_TRUNCATED;
			}
			WCHAR c = *wszVal++;
			if (c < ' ' || c > 'z')
				return E_FAIL;
			dest[i++] = (char) c;
		}
		dest[i] = '\0';
		*offset = i;
		return S_OK;
	}

	HRESULT SetString(cibool (*psetfunc)(ASAPInfo *, const char *), REFPROPVARIANT propvar)
	{
		char s[ASAPInfo_MAX_TEXT_LENGTH + 1];
		int offset = 0;
		HRESULT hr = S_OK;
		switch (propvar.vt) {
		case VT_EMPTY:
			s[0] = '\0';
			break;
		case VT_UI4:
			if (sprintf(s, "%lu", propvar.ulVal) != 4)
				return E_FAIL;
			break;
		case VT_LPWSTR:
			hr = AppendString(s, &offset, propvar.pwszVal);
			if (FAILED(hr))
				return hr;
			break;
		case VT_VECTOR | VT_LPWSTR:
			ULONG i;
			s[0] = '\0';
			for (i = 0; i < propvar.calpwstr.cElems; i++) {
				if (i > 0) {
					hr = AppendString(s, &offset, L" & ");
					if (FAILED(hr))
						return hr;
				}
				hr = AppendString(s, &offset, propvar.calpwstr.pElems[i]);
				if (FAILED(hr))
					return hr;
			}
			break;
		default:
			return E_NOTIMPL;
		}
		if (!psetfunc(m_pinfo, s))
			return E_FAIL;
		return hr;
	}

public:

	CASAPMetadataHandler() : m_cRef(1), m_hasInfo(FALSE), m_pstream(NULL)
	{
		DllAddRef();
		InitializeCriticalSection(&m_lock);
		m_filename[0] = '\0';
		m_pinfo = ASAPInfo_New();
	}

	STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
	{
		if (riid == IID_IUnknown || riid == IID_IColumnProvider) {
			*ppv = (IColumnProvider *) this;
			AddRef();
			return S_OK;
		}
		if (riid == IID_IInitializeWithStream) {
			*ppv = (IInitializeWithStream *) this;
			AddRef();
			return S_OK;
		}
		if (riid == IID_IPropertyStore) {
			*ppv = (IPropertyStore *) this;
			AddRef();
			return S_OK;
		}
		if (riid == IID_IPropertyStoreCapabilities) {
			*ppv = (IPropertyStoreCapabilities *) this;
			AddRef();
			return S_OK;
		}
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_cRef);
	}

	STDMETHODIMP_(ULONG) Release()
	{
		ULONG r = InterlockedDecrement(&m_cRef);
		if (r == 0)
			delete this;
		return r;
	}

	// IColumnProvider

	STDMETHODIMP Initialize(LPCSHCOLUMNINIT psci)
	{
		return S_OK;
	}

	STDMETHODIMP GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci)
	{
		if (dwIndex >= 0 && dwIndex < N_PROPERTYDEFS) {
			g_propertyDefs[dwIndex].CopyTo(psci);
			return S_OK;
		}
		return S_FALSE;
	}

	STDMETHODIMP GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
	{
		if ((pscd->dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_OFFLINE)) != 0)
			return S_FALSE;
		char ext[4];
		for (int i = 0; i < 3; i++) {
			WCHAR c = pscd->pwszExt[1 + i];
			if (c <= ' ' || c > 'z')
				return S_FALSE;
			ext[i] = (char) c;
		}
		if (pscd->pwszExt[5] != '\0')
			return S_FALSE;
		ext[3] = '\0';
		if (!ASAPInfo_IsOurExt(ext))
			return S_FALSE;

		CMyLock lck(&m_lock);
		if ((pscd->dwFlags & SHCDF_UPDATEITEM) != 0 || lstrcmpW(m_filename, pscd->wszFile) != 0) {
			lstrcpyW(m_filename, pscd->wszFile);
			HRESULT hr = LoadFile(pscd->wszFile, NULL, STGM_READ);
			if (FAILED(hr))
				return hr;
		}
		return GetData(pscid, (PROPVARIANT *) pvarData, FALSE);
	}

	// IInitializeWithStream

	STDMETHODIMP Initialize(IStream *pstream, DWORD grfMode)
	{
		STATSTG statstg;
		HRESULT hr = pstream->Stat(&statstg, STATFLAG_DEFAULT);
		if (FAILED(hr))
			return hr;
		CMyLock lck(&m_lock);
		hr = LoadFile(statstg.pwcsName, pstream, grfMode);
		CoTaskMemFree(statstg.pwcsName);
		return hr;
	}

	// IPropertyStore

	STDMETHODIMP GetCount(DWORD *cProps)
	{
		CMyLock lck(&m_lock);
		return m_hasInfo ? N_PROPERTYDEFS : 0;
	}

	STDMETHODIMP GetAt(DWORD iProp, PROPERTYKEY *pkey)
	{
		CMyLock lck(&m_lock);
		if (m_hasInfo && iProp >= 0 && iProp < N_PROPERTYDEFS) {
			*pkey = g_propertyDefs[iProp].scid;
			return S_OK;
		}
		return E_INVALIDARG;
	}

	STDMETHODIMP GetValue(REFPROPERTYKEY key, PROPVARIANT *pv)
	{
		CMyLock lck(&m_lock);
		HRESULT hr = GetData(&key, pv, TRUE);
		if (hr == S_FALSE) {
			pv->vt = VT_EMPTY;
			return S_OK;
		}
		return hr;
	}

	STDMETHODIMP SetValue(REFPROPERTYKEY key, REFPROPVARIANT propvar)
	{
		CMyLock lck(&m_lock);
		if (m_pstream == NULL)
			return STG_E_ACCESSDENIED;
		if (key.fmtid == FMTID_SummaryInformation) {
			if (key.pid == PIDSI_TITLE)
				return SetString(ASAPInfo_SetTitle, propvar);
			if (key.pid == PIDSI_AUTHOR)
				return SetString(ASAPInfo_SetAuthor, propvar);
		}
		else if (key.fmtid == FMTID_MUSIC) {
			if (key.pid == PIDSI_ARTIST)
				return SetString(ASAPInfo_SetAuthor, propvar);
			if (key.pid == PIDSI_YEAR)
				return SetString(ASAPInfo_SetDate, propvar);
		}
		return E_FAIL;
	}

	STDMETHODIMP Commit(void)
	{
		CMyLock lck(&m_lock);
		if (m_pstream == NULL)
			return STG_E_ACCESSDENIED;
		LARGE_INTEGER liZero;
		liZero.LowPart = 0;
		liZero.HighPart = 0;
		HRESULT hr = m_pstream->Seek(liZero, STREAM_SEEK_SET, NULL);
		if (SUCCEEDED(hr)) {
			byte module[ASAPInfo_MAX_MODULE_LENGTH];
			int module_len;
			hr = m_pstream->Read(module, ASAPInfo_MAX_MODULE_LENGTH, (ULONG *) &module_len);
			if (SUCCEEDED(hr)) {
				hr = m_pstream->Seek(liZero, STREAM_SEEK_SET, NULL);
				if (SUCCEEDED(hr)) {
					CMemoryByteWriter mbw;
					if (!ASAPWriter_Write("dummy.sap", mbw.GetByteWriter(), m_pinfo, module, module_len, FALSE))
						hr = E_FAIL;
					else {
						ULARGE_INTEGER liSize;
						liSize.LowPart = mbw.length;
						liSize.HighPart = 0;
						hr = m_pstream->SetSize(liSize);
						if (SUCCEEDED(hr)) {
							hr = m_pstream->Write(mbw.module, mbw.length, NULL);
							if (SUCCEEDED(hr))
								hr = m_pstream->Commit(STGC_DEFAULT);
						}
					}
				}
			}
		}
		m_pstream->Release();
		m_pstream = NULL;
		return hr;
	}

	// IPropertyStoreCapabilities

	STDMETHODIMP IsPropertyWritable(REFPROPERTYKEY key)
	{
		if (key.fmtid == FMTID_SummaryInformation) {
			if (key.pid == PIDSI_TITLE || key.pid == PIDSI_AUTHOR)
				return S_OK;
		}
		else if (key.fmtid == FMTID_MUSIC) {
			if (key.pid == PIDSI_ARTIST || key.pid == PIDSI_YEAR)
				return S_OK;
		}
		return S_FALSE;
	}
};

class CASAPMetadataHandlerFactory : public IClassFactory
{
public:

	STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
	{
		if (riid == IID_IUnknown || riid == IID_IClassFactory) {
			*ppv = (IClassFactory *) this;
			DllAddRef();
			return S_OK;
		}
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		DllAddRef();
		return 2;
	}

	STDMETHODIMP_(ULONG) Release()
	{
		DllRelease();
		return 1;
	}

	STDMETHODIMP CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppv)
	{
		*ppv = NULL;
		if (punkOuter != NULL)
			return CLASS_E_NOAGGREGATION;
		CASAPMetadataHandler *punk = new CASAPMetadataHandler;
		if (punk == NULL)
			return E_OUTOFMEMORY;
		HRESULT hr = punk->QueryInterface(riid, ppv);
		punk->Release();
		return hr;
	}

	STDMETHODIMP LockServer(BOOL fLock)
	{
		if (fLock)
			DllAddRef();
		else
			DllRelease();
		return S_OK;
	};
};

STDAPI_(BOOL) __declspec(dllexport) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		g_hDll = hInstance;
		OSVERSIONINFO osvi;
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);
		g_windowsVer = WINDOWS_OLD;
		if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
			if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1)
				g_windowsVer = WINDOWS_XP;
			else if (osvi.dwMajorVersion >= 6)
				g_windowsVer = WINDOWS_VISTA;
		}
	}
	return TRUE;
}

STDAPI __declspec(dllexport) DllRegisterServer(void)
{
	HKEY hk1;
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, "CLSID\\" CLSID_ASAPMetadataHandler_str, 0, NULL, 0, KEY_WRITE, NULL, &hk1, NULL) != ERROR_SUCCESS)
		return E_FAIL;
	HKEY hk2;
	if (RegCreateKeyEx(hk1, "InProcServer32", 0, NULL, 0, KEY_WRITE, NULL, &hk2, NULL) != ERROR_SUCCESS) {
		RegCloseKey(hk1);
		return E_FAIL;
	}
	char szModulePath[MAX_PATH];
	DWORD nModulePathLen = GetModuleFileName(g_hDll, szModulePath, MAX_PATH);
	static const char szThreadingModel[] = "Both";
	if (RegSetValueEx(hk2, NULL, 0, REG_SZ, (CONST BYTE *) szModulePath, nModulePathLen) != ERROR_SUCCESS
	 || RegSetValueEx(hk2, "ThreadingModel", 0, REG_SZ, (CONST BYTE *) szThreadingModel, sizeof(szThreadingModel)) != ERROR_SUCCESS) {
		RegCloseKey(hk2);
		RegCloseKey(hk1);
		return E_FAIL;
	}
	RegCloseKey(hk2);
	RegCloseKey(hk1);
	if (g_windowsVer == WINDOWS_VISTA) {
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers", 0, KEY_WRITE, &hk1) != ERROR_SUCCESS)
			return E_FAIL;
		for (int i = 0; i < N_EXTS; i++) {
			if (RegCreateKeyEx(hk1, extensions[i], 0, NULL, 0, KEY_WRITE, NULL, &hk2, NULL) != ERROR_SUCCESS) {
				RegCloseKey(hk1);
				return E_FAIL;
			}
			static const char CLSID_ASAPMetadataHandler_str2[] = CLSID_ASAPMetadataHandler_str;
			if (RegSetValueEx(hk2, NULL, 0, REG_SZ, (CONST BYTE *) CLSID_ASAPMetadataHandler_str2, sizeof(CLSID_ASAPMetadataHandler_str2)) != ERROR_SUCCESS) {
				RegCloseKey(hk2);
				RegCloseKey(hk1);
				return E_FAIL;
			}
			RegCloseKey(hk2);
		}
		RegCloseKey(hk1);
	}
	else {
		if (RegCreateKeyEx(HKEY_CLASSES_ROOT, "Folder\\shellex\\ColumnHandlers\\" CLSID_ASAPMetadataHandler_str, 0, NULL, 0, KEY_WRITE, NULL, &hk1, NULL) != ERROR_SUCCESS)
			return E_FAIL;
		RegCloseKey(hk1);
	}
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", 0, KEY_SET_VALUE, &hk1) != ERROR_SUCCESS)
		return E_FAIL;
	static const char szDescription[] = "ASAP Metadata Handler";
	if (RegSetValueEx(hk1, CLSID_ASAPMetadataHandler_str, 0, REG_SZ, (CONST BYTE *) szDescription, sizeof(szDescription)) != ERROR_SUCCESS) {
		RegCloseKey(hk1);
		return E_FAIL;
	}
	RegCloseKey(hk1);
	return S_OK;
}

STDAPI __declspec(dllexport) DllUnregisterServer(void)
{
	HKEY hk1;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", 0, KEY_SET_VALUE, &hk1) == ERROR_SUCCESS) {
		RegDeleteValue(hk1, CLSID_ASAPMetadataHandler_str);
		RegCloseKey(hk1);
	}
	if (g_windowsVer == WINDOWS_VISTA) {
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers", 0, DELETE, &hk1) == ERROR_SUCCESS) {
			for (int i = 0; i < N_EXTS; i++)
				RegDeleteKey(hk1, extensions[i]);
			RegCloseKey(hk1);
		}
	}
	else
		RegDeleteKey(HKEY_CLASSES_ROOT, "Folder\\shellex\\ColumnHandlers\\" CLSID_ASAPMetadataHandler_str);
	RegDeleteKey(HKEY_CLASSES_ROOT, "CLSID\\" CLSID_ASAPMetadataHandler_str "\\InProcServer32");
	RegDeleteKey(HKEY_CLASSES_ROOT, "CLSID\\" CLSID_ASAPMetadataHandler_str);
	return S_OK;
}

STDAPI __declspec(dllexport) DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	if (ppv == NULL)
		return E_INVALIDARG;
	if (rclsid == CLSID_ASAPMetadataHandler) {
		static CASAPMetadataHandlerFactory g_ClassFactory;
		return g_ClassFactory.QueryInterface(riid, ppv);
	}
	*ppv = NULL;
	return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI __declspec(dllexport) DllCanUnloadNow(void)
{
    return g_cRef == 0 ? S_OK : S_FALSE;
}

static HRESULT DoPropertySchema(LPCSTR funcName)
{
	HRESULT hr = CoInitialize(NULL);
	if (SUCCEEDED(hr)) {
		WCHAR szSchemaPath[MAX_PATH];
		hr = E_FAIL;
		if (GetModuleFileNameW(g_hDll, szSchemaPath, MAX_PATH)
		 && PathRemoveFileSpecW(szSchemaPath)
		 && PathAppendW(szSchemaPath, L"ASAPShellEx.propdesc")) {
			HMODULE propsysDll = LoadLibrary("propsys.dll");
			if (propsysDll != NULL) {
				typedef HRESULT (__stdcall *FuncType)(PCWSTR);
				FuncType func = (FuncType) GetProcAddress(propsysDll, funcName);
				if (func != NULL)
					hr = func(szSchemaPath);
				FreeLibrary(propsysDll);
			}
		}
		CoUninitialize();
	}
	return hr;
}

STDAPI __declspec(dllexport) InstallPropertySchema(void)
{
	HRESULT hr = DoPropertySchema("PSRegisterPropertySchema");
	if (SUCCEEDED(hr))
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	return hr;
}

STDAPI __declspec(dllexport) UninstallPropertySchema(void)
{
	return DoPropertySchema("PSUnregisterPropertySchema");
}
