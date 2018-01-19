/*
 * waveout.d - convenience wrapper for Windows waveOut API and ALSA
 *
 * Copyright (C) 2011  Adrian Matoga
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

import std.stdio;
import std.exception;
import std.algorithm;
import std.conv;
import std.string;

version (Windows)
{

import win32.windef: UINT, DWORD, DWORD_PTR, NULL;
import win32.winbase: GetCurrentThreadId, INVALID_HANDLE_VALUE;
import win32.winuser: PostThreadMessage, GetMessage, PeekMessage, MSG, WM_USER, PM_REMOVE;
import win32.mmsystem;

pragma (lib, "winmm");

enum IDM_WOM_DONE = WM_USER;

class WaveOutException : Exception
{
	this(string msg)
	{
		super(msg);
	}
}

extern (Windows)
private void waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	WaveOut wo = *(cast(WaveOut*) dwInstance);
	if (uMsg == WOM_DONE)
		PostThreadMessage(wo.threadId, IDM_WOM_DONE, 0, 0);
}

class WaveOut
{
	this(ushort channels, uint samplesPerSec, ushort bitsPerSample, size_t bufferSize = 16384)
	{
		thisObj = this;
		threadId = GetCurrentThreadId();
		enum bufferCount = 2;
		buffers = new char[][bufferCount];
		foreach (i; 0 .. bufferCount) {
			buffers[i] = new char[bufferSize];
			headers ~= WAVEHDR(buffers[i].ptr, buffers[i].length, 0, 0, 0, 0, NULL, 0);
		}
		WAVEFORMATEX wfx;
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nChannels = channels;
		wfx.nSamplesPerSec = samplesPerSec;
		wfx.nBlockAlign = cast(ushort) (channels * bitsPerSample / 8);
		wfx.nAvgBytesPerSec = samplesPerSec * wfx.nBlockAlign;
		wfx.wBitsPerSample = bitsPerSample;
		wfx.cbSize = 0;
		if (waveOutOpen(&hwo, WAVE_MAPPER, &wfx, cast(DWORD_PTR) &waveOutProc, cast(DWORD_PTR) &thisObj, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
			throw new WaveOutException("waveOutOpen failed");
		scope (failure) {
			waveOutClose(hwo);
			foreach (i; 0 .. bufferCount)
				if (headers[i].dwFlags & WHDR_PREPARED)
					waveOutUnprepareHeader(hwo, &headers[i], headers[i].sizeof);
		}
		foreach (i; 0 .. bufferCount)
			if (waveOutPrepareHeader(hwo, &headers[i], headers[i].sizeof) != MMSYSERR_NOERROR)
				throw new WaveOutException("waveOutPrepareHeader failed");
	}

	~this()
	{
		close();
	}

	void close()
	{
		if (hwo == INVALID_HANDLE_VALUE)
			return;
		waveOutReset(hwo);
		foreach (ref hdr; headers) {
			if (hdr.dwFlags & WHDR_PREPARED)
				waveOutUnprepareHeader(hwo, &hdr, hdr.sizeof);
		}
		waveOutClose(hwo);
		hwo = INVALID_HANDLE_VALUE;
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {}
	}

	void write(const(ubyte)[] buf)
	{
		while (buf.length) {
			if (usedBuffers == buffers.length) {
				MSG msg;
				if (GetMessage(&msg, NULL, 0, 0) <= 0 || msg.message != IDM_WOM_DONE)
					throw new WaveOutException("GetMessage failed");
				--usedBuffers;
			}
			size_t toEnd = buffers[currentBuffer].length - offset;
			size_t toWrite = min(buf.length, toEnd);
			buffers[currentBuffer][offset .. offset + toWrite] = cast(const(char[])) buf[0 .. toWrite];
			buf = buf[toWrite .. $];
			if ((offset += toWrite) >= buffers[currentBuffer].length) {
				if (waveOutWrite(hwo, &headers[currentBuffer], headers[currentBuffer].sizeof) != MMSYSERR_NOERROR)
					throw new WaveOutException("waveOutWrite failed");
				offset = 0;
				++usedBuffers;
				if (++currentBuffer >= buffers.length)
					currentBuffer = 0;
			}
		}
	}

private:
	WaveOut thisObj;
	DWORD threadId;

	HWAVEOUT hwo = INVALID_HANDLE_VALUE;

	WAVEHDR[] headers;
	char[][] buffers;

	size_t currentBuffer;
	size_t offset;
	size_t usedBuffers;
}

} // version (Windows)

version (linux)
{

import alsa.pcm;

class WaveOut
{
	this(ushort channels, uint samplesPerSec, ushort bitsPerSample, size_t bufferSize = 16384)
	{
		int err;
		if ((err = snd_pcm_open(&hpcm, "default".toStringz(), snd_pcm_stream_t.SND_PCM_STREAM_PLAYBACK, 0)) < 0)
			throw new Exception("Cannot open default audio device");
		if ((err = snd_pcm_set_params(
			hpcm, bitsPerSample == 8 ? snd_pcm_format_t.SND_PCM_FORMAT_U8 : snd_pcm_format_t.SND_PCM_FORMAT_S16_LE,
			snd_pcm_access_t.SND_PCM_ACCESS_RW_INTERLEAVED,
			channels, samplesPerSec, 1, 50000)) < 0) {
			close();
			throw new Exception("Cannot set audio device params");
		}
		bytesPerSample = bitsPerSample / 8 * channels;
	}

	~this()
	{
		close();
	}

	void close()
	{
		if (hpcm is null)
			return;
		snd_pcm_close(hpcm);
		hpcm = null;
	}

	void write(const(ubyte)[] buf)
	{
		snd_pcm_sframes_t frames = snd_pcm_writei(hpcm, buf.ptr, buf.length / bytesPerSample);
		if (frames < 0) {
			frames = snd_pcm_recover(hpcm, cast(int) frames, 0);
			if (frames < 0)
				throw new Exception("snd_pcm_writei failed");
		}
	}

private:
	snd_pcm_t* hpcm;
	int bytesPerSample;
}

} // version (linux)
