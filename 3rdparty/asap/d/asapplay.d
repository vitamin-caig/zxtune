/*
 * asapplay.d - D ASAP player
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

import std.math;
import std.stdio;
import std.file;
import std.conv;
import std.string;

import waveout;
import asap;

int song = -1;
int duration = -1;
bool showInfo;

void processFile(string filename)
{
	auto asap = new ASAP;
	auto mod = cast(ubyte[]) read(filename, ASAPInfo.MaxModuleLength);
	asap.Load(filename, mod, cast(int) mod.length);
	ASAPInfo info = asap.GetInfo();
	if (song < 0)
		song = info.GetDefaultSong();
	if (duration < 0) {
		duration = info.GetDuration(song);
		if (duration < 0)
			duration = 180 * 1000;
	}
	asap.PlaySong(song, duration);
	auto wo = new WaveOut(cast(ushort) info.Channels, 44100, 16);
	scope(exit) wo.close();
	ubyte[] buffer = new ubyte[8192];
	if (showInfo)
		writefln("%s - %s (%d:%02d)", info.GetAuthor(), info.GetTitle(), duration / 60000, duration / 1000 % 60);
	while ((buffer.length = asap.Generate(buffer, cast(int) buffer.length, ASAPSampleFormat.S16LE)) > 0)
		wo.write(buffer);
	song = -1;
	duration = -1;
}

void printHelp()
{
	write(
		"Usage: asapplay [OPTIONS] INPUTFILE...\n" ~
		"Each INPUTFILE must be in a supported format:\n" ~
		"SAP, CMC, CM3, CMR, CMS, DMC, DLT, MPT, MPD, RMT, TMC, TM8, TM2 or FC.\n" ~
		"Options:\n" ~
		"-s SONG     --song=SONG        Select subsong number (zero-based)\n" ~
		"-t TIME     --time=TIME        Set output length (MM:SS format)\n" ~
		"-i          --show-info        Show song info\n" ~
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

int main(string[] args)
{
	bool noInputFiles = true;
	for (size_t i = 1; i < args.length; ++i) {
		auto arg = args[i];
		if (arg[0] != '-') {
			processFile(arg);
			noInputFiles = false;
		}
		else if (arg == "-i" || arg == "--show-info")
			showInfo = true;
		else if (arg == "-s")
			setSong(args[++i]);
		else if (arg.startsWith("--song="))
			setSong(arg[7 .. $]);
		else if (arg == "-t")
			setTime(args[++i]);
		else if (arg.startsWith("--time="))
			setTime(arg[7 .. $]);
		else if (arg == "-h" || arg == "--help") {
			printHelp();
			noInputFiles = false;
		}
		else if (arg == "-v" || arg == "--version") {
			writeln("asapplay (D) ", ASAPInfo.Version);
			noInputFiles = false;
		}
		else
			throw new Exception("unknown option: ", arg);
	}
	if (noInputFiles) {
		printHelp();
		return 1;
	}
	return 0;
}
