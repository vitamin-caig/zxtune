/*
 * asap2wav.d - converter of ASAP-supported formats to WAV files
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
import std.conv;
import std.file;
import std.string;

import asap;

string outputFilename;
bool outputHeader = true;
int song = -1;
ASAPSampleFormat format = ASAPSampleFormat.S16LE;
int duration = -1;
int muteMask = 0;

void printHelp()
{
	write(
		"Usage: asap2wav [OPTIONS] INPUTFILE...\n" ~
		"Each INPUTFILE must be in a supported format:\n" ~
		"SAP, CMC, CM3, CMR, CMS, DMC, DLT, MPT, MPD, RMT, TMC, TM8, TM2 or FC.\n" ~
		"Options:\n" ~
		"-o FILE     --output=FILE      Set output file name\n" ~
		"-s SONG     --song=SONG        Select subsong number (zero-based)\n" ~
		"-t TIME     --time=TIME        Set output length (MM:SS format)\n" ~
		"-b          --byte-samples     Output 8-bit samples\n" ~
		"-w          --word-samples     Output 16-bit samples (default)\n" ~
		"            --raw              Output raw audio (no WAV header)\n" ~
		"-m CHANNELS --mute=CHANNELS    Mute POKEY chanels (1-8)\n" ~
		"-h          --help             Display this information\n" ~
		"-v          --version          Display version information\n"
	);
}

void setSong(string s)
{
	song = to!int(s);
}

void setTime(string s)
{
	duration = ASAPInfo.ParseDuration(s);
}

void setMuteMask(string s)
{
	int mask = 0;
	foreach (dchar c; s) {
		if (c >= '1' && c <= '8')
			mask |= 1 << (c - '1');
	}
	muteMask = mask;
}

void processFile(string inputFilename)
{
	auto asap = new ASAP;
	auto mod = cast(ubyte[]) read(inputFilename, ASAPInfo.MaxModuleLength);
	asap.Load(inputFilename, mod, cast(int) mod.length);
	ASAPInfo info = asap.GetInfo();
	if (song < 0)
		song = info.GetDefaultSong();
	if (duration < 0) {
		duration = info.GetDuration(song);
		if (duration < 0)
			duration = 180 * 1000;
	}
	asap.PlaySong(song, duration);
	asap.MutePokeyChannels(muteMask);
	if (!outputFilename.length) {
		auto i = inputFilename.lastIndexOf('.');
		outputFilename = inputFilename[0 .. i + 1] ~ (outputHeader ? "wav" : "raw");
	}
	auto s = File(outputFilename, "wb");
	auto buffer = new ubyte[8192];
	if (outputHeader) {
		int len = asap.GetWavHeader(buffer, format, false);
		s.rawWrite(buffer[0 .. len]);
	}
	while ((buffer.length = asap.Generate(buffer, cast(int) buffer.length, format)) > 0)
		s.rawWrite(buffer);
	s.close();
	outputFilename = null;
	song = -1;
	duration = -1;
}

int main(string[] args)
{
	bool noInputFiles = true;
	for (int i = 1; i < args.length; i++) {
		string arg = args[i];
		if (arg[0] != '-') {
			processFile(arg);
			noInputFiles = false;
		}
		else if (arg == "-o")
			outputFilename = args[++i];
		else if (arg.startsWith("--output="))
			outputFilename = arg[9 .. $];
		else if (arg == "-s")
			setSong(args[++i]);
		else if (arg.startsWith("--song="))
			setSong(arg[7 .. $]);
		else if (arg == "-t")
			setTime(args[++i]);
		else if (arg.startsWith("--time="))
			setTime(arg[7 .. $]);
		else if (arg == "-b" || arg == "--byte-samples")
			format = ASAPSampleFormat.U8;
		else if (arg == "-w" || arg == "--word-samples")
			format = ASAPSampleFormat.S16LE;
		else if (arg == "--raw")
			outputHeader = false;
		else if (arg == "-m")
			setMuteMask(args[++i]);
		else if (arg.startsWith("--mute="))
			setMuteMask(arg[7 .. $]);
		else if (arg == "-h" || arg == "--help") {
			printHelp();
			noInputFiles = false;
		}
		else if (arg == "-v" || arg == "--version") {
			writeln("ASAP2WAV (D) ", ASAPInfo.Version);
			noInputFiles = false;
		}
		else
			throw new Exception("unknown option: " ~ arg);
	}
	if (noInputFiles) {
		printHelp();
		return 1;
	}
	return 0;
}
