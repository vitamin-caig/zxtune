// XMPlay plugin functions header (c) 2004-2007 Ian Luck

#pragma once

#include <wtypes.h>

typedef unsigned __int64 QWORD;

#ifdef __cplusplus
extern "C" {
#endif

// Note all texts are UTF-8 on WinNT based systems, and ANSI on Win9x
#define Utf2Uni(src,slen,dst,dlen) MultiByteToWideChar(CP_UTF8,0,src,slen,dst,dlen) // convert UTF-8 to Unicode

typedef void *(WINAPI *InterfaceProc)(DWORD face); // XMPlay interface retrieval function received by plugin

#define XMPFUNC_MISC_FACE		0 // miscellaneous functions (XMPFUNC_MISC)
#define XMPFUNC_REGISTRY_FACE	1 // registry functions (XMPFUNC_REGISTRY)
#define XMPFUNC_FILE_FACE		2 // file functions (XMPFUNC_FILE)
#define XMPFUNC_TEXT_FACE		3 // text functions (XMPFUNC_TEXT)
#define XMPFUNC_STATUS_FACE		4 // playback status functions (XMPFUNC_STATUS)

#define XMPCONFIG_NET_BUFFER	0
#define XMPCONFIG_NET_RESTRICT	1
#define XMPCONFIG_NET_RECONNECT	2
#define XMPCONFIG_NET_PROXY		3
#define XMPCONFIG_NET_PROXYCONF	4
#define XMPCONFIG_NET_TIMEOUT	5
#define XMPCONFIG_NET_PREBUF	6

#define XMPINFO_TEXT_GENERAL	0 // General info text
#define XMPINFO_TEXT_MESSAGE	1 // Message info text
#define XMPINFO_TEXT_SAMPLES	2 // Samples info text

#define XMPINFO_REFRESH_MAIN	1 // main window info area
#define XMPINFO_REFRESH_GENERAL	2 // General info window
#define XMPINFO_REFRESH_MESSAGE	4 // Message info window
#define XMPINFO_REFRESH_SAMPLES	8 // Samples info window

typedef void *XMPFILE;

#define XMPFILE_TYPE_MEMORY		0 // file in memory
#define XMPFILE_TYPE_FILE		1 // local file
#define XMPFILE_TYPE_NETFILE	2 // file on the 'net
#define XMPFILE_TYPE_NETSTREAM	3 // 'net stream (unknown length)

typedef struct {
	DWORD id;				// must be unique and >=0x10000
	const char *text;		// description
	void (WINAPI *proc)();	// handler
} XMPSHORTCUT;

typedef struct {
	DWORD rate;		// sample rate
	DWORD chan;		// channels
	DWORD res;		// bytes per sample (1=8-bit,2=16-bit,3=24-bit,4=float,0=undefined)
} XMPFORMAT;

typedef struct {
	float time;		// cue position
	const char *title;
	const char *performer;
} XMPCUE;

typedef struct { // miscellaneous functions
	DWORD (WINAPI *GetVersion)(); // get XMPlay version (eg. 0x03040001 = 3.4.0.1)
	HWND (WINAPI *GetWindow)(); // get XMPlay window handle
	void *(WINAPI *Alloc)(DWORD len); // allocate memory
	void *(WINAPI *ReAlloc)(void *mem, DWORD len); // re-allocate memory
	void (WINAPI *Free)(void *mem); // free allocated memory/text
	BOOL (WINAPI *CheckCancel)(); // user wants to cancel?
	DWORD (WINAPI *GetConfig)(DWORD option); // get a config (XMPCONFIG_xxx) value
	const char *(WINAPI *GetSkinConfig)(const char *name); // get a skinconfig value
	void (WINAPI *ShowBubble)(const char *text, DWORD time); // show a help bubble (time in ms, 0=default)
	void (WINAPI *RefreshInfo)(DWORD mode); // refresh info displays (XMPINFO_REFRESH_xxx flags)
	char *(WINAPI *GetInfoText)(DWORD mode); // get info window text (XMPINFO_TEXT_xxx)
	char *(WINAPI *FormatInfoText)(char *buf, const char *name, const char *value); // format text for info window (tabs & new-lines)
	char *(WINAPI *GetTag)(int tag); // get a current track's tag
		//tags: 0=title,1=artist,2=album,3=year,4=track,5=genre,6=comment,7=filetype, -1=formatted title,-2=filename,-3=track/cue title
	BOOL (WINAPI *RegisterShortcut)(const XMPSHORTCUT *cut); // add a shortcut
	BOOL (WINAPI *PerformShortcut)(DWORD id); // perform a shortcut action
// version 3.4.0.14
	const XMPCUE *(WINAPI *GetCue)(DWORD cue); // get a cue entry (0=image, 1=1st track)
} XMPFUNC_MISC;

typedef struct { // "registry" functions
	DWORD (WINAPI *Get)(const char *section, const char *key, void *data, DWORD size); // if data=NULL, required size is returned
	DWORD (WINAPI *GetString)(const char *section, const char *key, char *data, DWORD size); // if data=NULL, required size is returned
	BOOL (WINAPI *GetInt)(const char *section, const char *key, int *data);
	BOOL (WINAPI *Set)(const char *section, const char *key, const void *data, DWORD size); // data=NULL = delete key
	BOOL (WINAPI *SetString)(const char *section, const char *key, const char *data);
	BOOL (WINAPI *SetInt)(const char *section, const char *key, const int *data);
} XMPFUNC_REGISTRY;

typedef struct { // file functions
	XMPFILE (WINAPI *Open)(const char *filename); // open a file
	XMPFILE (WINAPI *OpenMemory)(const void *buf, DWORD len); // open a file from memory
	void (WINAPI *Close)(XMPFILE file); // close an opened file
	DWORD (WINAPI *GetType)(XMPFILE file); // return XMPFILE_TYPE_xxx
	DWORD (WINAPI *GetSize)(XMPFILE file); // file size
	const char *(WINAPI *GetFilename)(XMPFILE file); // filename
	const void *(WINAPI *GetMemory)(XMPFILE file); // memory location (XMPFILE_TYPE_MEMORY)
	DWORD (WINAPI *Read)(XMPFILE file, void *buf, DWORD len); // read from file
	BOOL (WINAPI *Seek)(XMPFILE file, DWORD pos); // seek in file
	DWORD (WINAPI *Tell)(XMPFILE file); // get current file pos
	// net-only stuff
	void (WINAPI *NetSetRate)(XMPFILE file, DWORD rate); // set bitrate in bytes/sec (decides buffer size)
	BOOL (WINAPI *NetIsActive)(XMPFILE file); // connection is still up?
	BOOL (WINAPI *NetPreBuf)(XMPFILE file); // pre-buffer data
	DWORD (WINAPI *NetAvailable)(XMPFILE file); // get amount of data ready to go

	char *(WINAPI *ArchiveList)(XMPFILE file); // get archive contents (series of NULL-terminated entries)
	XMPFILE (WINAPI *ArchiveExtract)(XMPFILE file, const char *entry, DWORD len); // decompress file from archive
} XMPFUNC_FILE;

typedef struct { // text functions - return new string in native form (UTF-8/ANSI)
	char *(WINAPI *Ansi)(const char *text, int len); // ANSI string (len=-1=null terminated)
	char *(WINAPI *Unicode)(const WCHAR *text, int len); // Unicode string
	char *(WINAPI *Utf8)(const char *text, int len); // UTF-8 string
} XMPFUNC_TEXT;

typedef struct { // playback status functions
	BOOL (WINAPI *IsPlaying)(); // playing?
	double (WINAPI *GetTime)(); // track position in seconds
	QWORD (WINAPI *GetWritten)(); // samples written to output
	DWORD (WINAPI *GetLatency)(); // samples in output buffer
	const XMPFORMAT *(WINAPI *GetFormat)(BOOL in); // get input/output sample format
} XMPFUNC_STATUS;


#ifdef __cplusplus
}
#endif
