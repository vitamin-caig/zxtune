// XMPlay input plugin header (c) 2004-2007 Ian Luck
// new plugins can be submitted to plugins@xmplay.com

#pragma once

#include "xmpfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XMPIN_FACE
#define XMPIN_FACE 3 // "face"
#endif

#define XMPIN_FLAG_CANSTREAM	1 // can stream files (play while downloading from the 'net)
#define XMPIN_FLAG_OWNFILE		2 // can process files without "XMPFILE" routines
#define XMPIN_FLAG_NOXMPFILE	4 // never use "XMPFILE" routines (implies XMPIN_FLAG_OWNFILE)
#define XMPIN_FLAG_LOOP			8 // custom looping
#define XMPIN_FLAG_TAIL			16 // output tail (decay/fadeout)

// SetPosition special positions
#define XMPIN_POS_LOOP			-1 // loop
#define XMPIN_POS_AUTOLOOP		-2 // auto-loop
#define XMPIN_POS_TAIL			-3 // output tail (decay/fadeout)
#define XMPIN_POS_SUBSONG		0x80000000 // subsong (LOWORD=subsong)

// VisRender/DC flags
#define XMPIN_VIS_INIT			1 // DC/buffer is uninitialized
#define XMPIN_VIS_FULL			2 // fullscreen

typedef struct {
	DWORD flags; // XMPIN_FLAG_xxx
	const char *name; // plugin name
	const char *exts; // supported file extensions (description\0ext1/ext2/etc...)

	void (WINAPI *About)(HWND win); // (OPTIONAL)
	void (WINAPI *Config)(HWND win); // present config options to user (OPTIONAL)
	BOOL (WINAPI *CheckFile)(const char *filename, XMPFILE file); // verify file
	BOOL (WINAPI *GetFileInfo)(const char *filename, XMPFILE file, float *length, char *tags[8]); // get track info
		//tags: 0=title,1=artist,2=album,3=year,4=track,5=genre,6=comment,7=filetype

	// playback stuff
	DWORD (WINAPI *Open)(const char *filename, XMPFILE file); // open a file
	void (WINAPI *Close)(); // close file
	void *reserved;
	void (WINAPI *SetFormat)(XMPFORMAT *form); // set sample format

	BOOL (WINAPI *GetTags)(char *tags[8]); // get title elements, return TRUE to delay (OPTIONAL)
	void (WINAPI *GetInfoText)(char *format, char *length); // get main panel info text
	void (WINAPI *GetGeneralInfo)(char *buf); // get General info window text (buf is ~40K)
	void (WINAPI *GetMessage)(char *buf); // get Message info text (OPTIONAL)
	double (WINAPI *SetPosition)(DWORD pos); // seek
	double (WINAPI *GetGranularity)(); // seeking granularity
	DWORD (WINAPI *GetBuffering)(); // get buffering progress (OPTIONAL)
	DWORD (WINAPI *Process)(float *buf, DWORD count); // decode some sample data
	BOOL (WINAPI *WriteFile)(const char *filename); // write file to disk (OPTIONAL)

	void (WINAPI *GetSamples)(char *buf); // get Samples info text (OPTIONAL)
	DWORD (WINAPI *GetSubSongs)(float *length); // get number (and total length) of sub-songs (OPTIONAL)
	char *(WINAPI *GetCues)(); // get CUE sheet (OPTIONAL)

	float (WINAPI *GetDownloaded)(); // get download progress (OPTIONAL)

	// vis stuff (all OPTIONAL)
	const char *visname; // visualisation name
	BOOL (WINAPI *VisOpen)(DWORD colors[3]); // initialize vis
	void (WINAPI *VisClose)(); // close vis
	void (WINAPI *VisSize)(HDC dc, SIZE *size); // get ideal vis dimensions
	BOOL (WINAPI *VisRender)(DWORD *buf, SIZE size, DWORD flags); // render vis
	BOOL (WINAPI *VisRenderDC)(HDC dc, SIZE size, DWORD flags); // render vis
	void (WINAPI *VisButton)(DWORD x, DWORD y); // mouse click in vis
} XMPIN;

#define XMPFUNC_IN_FACE		11

typedef struct {
	void (WINAPI *SetLength)(float length, BOOL seekable); // set track length (-1=unchanged) and if it's seekable
	void (WINAPI *SetGain)(DWORD mode, float gain); // set replaygain (mode 0=track, 1=album)
	BOOL (WINAPI *UpdateTitle)(const char *track); // set track title (NULL=refresh tags/title)
} XMPFUNC_IN;


#ifdef __cplusplus
}
#endif
