//
//  exSID_ftdiwrap.c
//	An FTDI access wrapper for exSID USB
//
//  (C) 2016 Thibaut VARENE
//  License: GPLv2 - http://www.gnu.org/licenses/gpl-2.0.html
//
//  Coding style is somewhat unorthodox ;P

/**
 * @file
 * exSID USB FTDI access wrapper
 * @author Thibaut VARENE
 * @date 2016
 * @note Primary target is libftdi (cleaner API), adaptations are made for others.
 */


#include "exSID_defs.h"
#include <stdio.h>

#ifdef	HAVE_DLFCN_H
 #include <dlfcn.h>
 #define TEXT(x) x
#elif defined (_WIN32)
 #include <windows.h>
 #include <time.h>	// missing include in libftdi on WIN32
#else
 #error dl not supported
#endif

#ifdef	HAVE_FTD2XX
 #include <ftd2xx.h>
 #ifndef XSFW_SUPPORT
  #define XSFW_SUPPORT
 #endif
#else
 #warning libftd2xx support disabled.
#endif

#ifdef	HAVE_FTDI
 #include <ftdi.h>
 #ifndef XSFW_SUPPORT
  #define XSFW_SUPPORT
 #endif
#else
 #warning libftdi support disabled.
#endif

#ifndef XSFW_SUPPORT
 #error No known method to access FTDI chip
#endif

#define	XSFW_WRAPDECL
#include "exSID_ftdiwrap.h"

#define EXSID_INTERFACES "libftdi, libftd2xx"	// XXX TODO Should be set by configure

#ifdef _WIN32
 static HMODULE dlhandle = NULL;

 static char *_xSfw_dlerror() {
	DWORD dwError = GetLastError();
	char* lpMsgBuf = NULL;
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		0,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf,
		0,
		NULL);
	return lpMsgBuf;
 }

 #define _xSfw_dlopen(libName)		LoadLibrary(libName)
 #define _xSfw_dlsym(hModule, lpProcName)	GetProcAddress(hModule, lpProcName)
 #define _xSfw_dlclose(hModule)		FreeLibrary(hModule)
 #define _xSfw_clear_dlerror()		SetLastError(0)
 #define _xSfw_free_errstr(str)		LocalFree(str)
#else	// ! _WIN32
 static void * dlhandle = NULL;
 #define _xSfw_dlopen(filename)		dlopen(filename, RTLD_NOW|RTLD_LOCAL)
 #define _xSfw_dlsym(handle, symbol)	dlsym(handle, symbol)
 #define _xSfw_dlclose(handle)		dlclose(handle)
 #define _xSfw_dlerror()		dlerror()
 #define _xSfw_clear_dlerror()		dlerror()
 #define _xSfw_free_errstr(str)		/* nothing */
#endif	// _WIN32


/** Flag to signal which of the supported libraries is in use */
typedef enum {
	XS_LIBNONE,
	XS_LIBFTDI,
	XS_LIBFTD2XX,
} libtype_t;

static libtype_t libtype = XS_LIBNONE;

// private functions
static int (* _xSfw_set_baudrate)(void * ftdi, int baudrate);
static int (* _xSfw_set_line_property)(void * ftdi, int bits, int sbit, int parity);
static int (* _xSfw_setflowctrl)(void * ftdi, int flowctrl);
static int (* _xSfw_set_latency_timer)(void * ftdi, unsigned char latency);

// callbacks for ftdi
#ifdef	HAVE_FTDI
static int (* _ftdi_usb_open_desc)(void *, int, int, const char *, const char *);
#endif

// callbacks for FTD2XX
#ifdef	HAVE_FTD2XX
static int (*_FT_Write)(void *, LPVOID, int, unsigned int *);
static int (*_FT_Read)(void *, LPVOID, int, unsigned int *);
static int (*_FT_OpenEx)(const char *, int, void **);
static int (*_FT_ResetDevice)(void *);
static int (*_FT_SetBaudRate)(void *, int);
static int (*_FT_SetDataCharacteristics)(void *, int, int, int);
static int (*_FT_SetFlowControl)(void *, int, int, int);
static int (*_FT_SetLatencyTimer)(void *, unsigned char);
static int (*_FT_Close)(void *);
#endif

// wrappers for ftdi
#ifdef	HAVE_FTDI
static int _xSfwftdi_usb_open_desc(void ** ftdi, int vid, int pid, const char * desc, const char * serial)
{
	return _ftdi_usb_open_desc(*ftdi, vid, pid, desc, serial);
}
#endif

// wrappers for FTD2XX
#ifdef	HAVE_FTD2XX
static int _xSfwFT_write_data(void * restrict ftdi, const unsigned char * restrict buf, int size)
{
	DWORD dummysize;	// DWORD in unsigned int
	int rval;
	if(unlikely(rval = _FT_Write(ftdi, (LPVOID)buf, size, &dummysize)))
		return -rval;
	else
		return dummysize;
}

static int _xSfwFT_read_data(void * restrict ftdi, unsigned char * restrict buf, int size)
{
	DWORD dummysize;	// DWORD in unsigned int
	int rval;
	if (unlikely(rval = _FT_Read(ftdi, (LPVOID)buf, size, &dummysize)))
		return -rval;
	else
		return dummysize;
}

static int _xSfwFT_usb_open_desc(void ** ftdi, int vid, int pid, const char * desc, const char * serial)
{
	return -_FT_OpenEx(desc, FT_OPEN_BY_DESCRIPTION, ftdi);
}

static int _xSfwFT_usb_close(void * ftdi)
{
	return -_FT_Close(ftdi);
}

static char * _xSfwFT_get_error_string(void * ftdi)
{
	return "FTD2XX error";
}
#endif

/**
 * Attempt to dlopen a known working library to access FTDI chip.
 * Will try libftd2xx first, then libftdi.
 * @return 0 on success, -1 on error.
 */
int xSfw_dlopen()
{
#define XSFW_DLSYM(a, b)			\
	*(void **)(&a) = _xSfw_dlsym(dlhandle, b);	\
	if (a == NULL) {	\
		dlerrorstr = _xSfw_dlerror();	\
		goto dlfail;	\
	}

	char * dlerrorstr = NULL;

	if (dlhandle) {
		xsdbg("recursive dlopen()!\n");
		return 0;
	}

#ifdef	HAVE_FTDI
	// try libftdi1 first - XXX TODO version check
	if ((dlhandle = _xSfw_dlopen(TEXT("libftdi1" SHLIBEXT)))) {
		_xSfw_clear_dlerror();	// clear dlerror
		XSFW_DLSYM(xSfw_new, "ftdi_new");
		XSFW_DLSYM(xSfw_free, "ftdi_free");
		XSFW_DLSYM(xSfw_write_data, "ftdi_write_data");
		XSFW_DLSYM(xSfw_read_data, "ftdi_read_data");
		XSFW_DLSYM(_ftdi_usb_open_desc, "ftdi_usb_open_desc");
		xSfw_usb_open_desc = _xSfwftdi_usb_open_desc;
		XSFW_DLSYM(_xSfw_set_baudrate, "ftdi_set_baudrate");
		XSFW_DLSYM(_xSfw_set_line_property, "ftdi_set_line_property");
		XSFW_DLSYM(_xSfw_setflowctrl, "ftdi_setflowctrl");
		XSFW_DLSYM(_xSfw_set_latency_timer, "ftdi_set_latency_timer");
		XSFW_DLSYM(xSfw_usb_close, "ftdi_usb_close");
		XSFW_DLSYM(xSfw_get_error_string, "ftdi_get_error_string");
		libtype = XS_LIBFTDI;
		xsdbg("Using libftdi\n");
	}
	else
#endif
#ifdef	HAVE_FTD2XX
#ifdef _WIN32
# define LIBFTD2XX "ftd2xx"
#else
# define LIBFTD2XX "libftd2xx"
#endif
	// otherwise try libftd2xx - XXX TODO version check
	if ((dlhandle = _xSfw_dlopen(TEXT(LIBFTD2XX SHLIBEXT)))) {
		_xSfw_clear_dlerror();	// clear dlerror
		xSfw_new = NULL;
		xSfw_free = NULL;
		XSFW_DLSYM(_FT_Write, "FT_Write");
		xSfw_write_data = _xSfwFT_write_data;
		XSFW_DLSYM(_FT_Read, "FT_Read");
		xSfw_read_data = _xSfwFT_read_data;
		XSFW_DLSYM(_FT_OpenEx, "FT_OpenEx");
		xSfw_usb_open_desc = _xSfwFT_usb_open_desc;
		XSFW_DLSYM(_FT_ResetDevice, "FT_ResetDevice");
		XSFW_DLSYM(_FT_SetBaudRate, "FT_SetBaudRate");
		XSFW_DLSYM(_FT_SetDataCharacteristics, "FT_SetDataCharacteristics");
		XSFW_DLSYM(_FT_SetFlowControl, "FT_SetFlowControl");
		XSFW_DLSYM(_FT_SetLatencyTimer, "FT_SetLatencyTimer");
		XSFW_DLSYM(_FT_Close, "FT_Close");
		xSfw_usb_close = _xSfwFT_usb_close;
		xSfw_get_error_string = _xSfwFT_get_error_string;
		libtype = XS_LIBFTD2XX;
		xsdbg("Using libftd2xx\n");
	}
	else
#endif
	// if none worked, fail.
	{
		xsdbg("No method found to access FTDI interface.\n"
			"Are any of the following libraries installed?\n"
			"\t" EXSID_INTERFACES);
		return -1;
	}

	return 0;

dlfail:
	xsdbg("dlsym error: %s", dlerrorstr);
	_xSfw_free_errstr(dlerrorstr);
	xSfw_dlclose(dlhandle);
	return -1;
}

/**
 * Setup FTDI chip to match exSID firmware.
 * Defaults to 8N1, no flow control.
 * @param ftdi ftdi handle
 * @param baudrate Target baudrate
 * @param latency Target latency
 * @return 0 on success, rval on error.
 */
int xSfw_usb_setup(void * ftdi, int baudrate, int latency)
{
	int rval = 0;

#ifdef	HAVE_FTDI
	if (XS_LIBFTDI == libtype) {
		// ftdi_usb_open_desc() performs device reset
		rval = _xSfw_set_baudrate(ftdi, baudrate);
		if (rval < 0) {
			xsdbg("SBR error");
			goto setupfail;
		}
		rval = _xSfw_set_line_property(ftdi, BITS_8 , STOP_BIT_1, NONE);
		if (rval < 0) {
			xsdbg("SLP error");
			goto setupfail;
		}
		rval = _xSfw_setflowctrl(ftdi, SIO_DISABLE_FLOW_CTRL);
		if (rval < 0) {
			xsdbg("SFC error");
			goto setupfail;
		}
		rval = _xSfw_set_latency_timer(ftdi, latency);
		if (rval < 0) {
			xsdbg("SLT error");
			goto setupfail;
		}
	}
	else
#endif
#ifdef	HAVE_FTD2XX
	if (XS_LIBFTD2XX == libtype) {
		rval = -_FT_ResetDevice(ftdi);
		if (rval < 0) {
			xsdbg("RSD error");
			goto setupfail;
		}
		rval = -_FT_SetBaudRate(ftdi, baudrate);
		if (rval < 0) {
			xsdbg("SBR error");
			goto setupfail;
		}
		rval = -_FT_SetDataCharacteristics(ftdi, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
		if (rval < 0) {
			xsdbg("SLP error");
			goto setupfail;
		}
		rval = -_FT_SetFlowControl(ftdi, FT_FLOW_NONE, 0, 0);
		if (rval < 0) {
			xsdbg("SFC error");
			goto setupfail;
		}
		rval = -_FT_SetLatencyTimer(ftdi, latency);
		if (rval < 0) {
			xsdbg("SLT error");
			goto setupfail;
		}
	}
	else
#endif
		xsdbg("Unkown access method\n");
setupfail:
	return rval;
}

/**
 * Release dlopen'd library.
 */
void xSfw_dlclose()
{
	if (dlhandle != NULL) {
		_xSfw_dlclose(dlhandle);
		dlhandle = NULL;
	}
}
