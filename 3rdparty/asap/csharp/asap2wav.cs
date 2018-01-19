/*
 * asap2wav.cs - converter of ASAP-supported formats to WAV files
 *
 * Copyright (C) 2008-2012  Piotr Fusik
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

using System;
using System.IO;

using Sf.Asap;

public class Asap2Wav
{
	static string OutputFilename = null;
	static bool OutputHeader = true;
	static int Song = -1;
	static ASAPSampleFormat Format = ASAPSampleFormat.S16LE;
	static int Duration = -1;
	static int MuteMask = 0;

	static void PrintHelp()
	{
		Console.Write(
			"Usage: asap2wav [OPTIONS] INPUTFILE...\n" +
			"Each INPUTFILE must be in a supported format:\n" +
			"SAP, CMC, CM3, CMR, CMS, DMC, DLT, MPT, MPD, RMT, TMC, TM8, TM2 or FC.\n" +
			"Options:\n" +
			"-o FILE     --output=FILE      Set output file name\n" +
			"-s SONG     --song=SONG        Select subsong number (zero-based)\n" +
			"-t TIME     --time=TIME        Set output length (MM:SS format)\n" +
			"-b          --byte-samples     Output 8-bit samples\n" +
			"-w          --word-samples     Output 16-bit samples (default)\n" +
			"            --raw              Output raw audio (no WAV header)\n" +
			"-m CHANNELS --mute=CHANNELS    Mute POKEY chanels (1-8)\n" +
			"-h          --help             Display this information\n" +
			"-v          --version          Display version information\n"
		);
	}

	static void SetSong(string s)
	{
		Song = int.Parse(s);
	}

	static void SetTime(string s)
	{
		Duration = ASAPInfo.ParseDuration(s);
	}

	static void SetMuteMask(string s)
	{
		int mask = 0;
		foreach (char c in s) {
			if (c >= '1' && c <= '8')
				mask |= 1 << (c - '1');
		}
		MuteMask = mask;
	}

	static void ProcessFile(string inputFilename)
	{
		byte[] module = new byte[ASAPInfo.MaxModuleLength];
		int moduleLen;
		using (Stream s = File.OpenRead(inputFilename)) {
			moduleLen = s.Read(module, 0, module.Length);
		}
		ASAP asap = new ASAP();
		asap.Load(inputFilename, module, moduleLen);
		ASAPInfo info = asap.GetInfo();
		if (Song < 0)
			Song = info.GetDefaultSong();
		if (Duration < 0) {
			Duration = info.GetDuration(Song);
			if (Duration < 0)
				Duration = 180 * 1000;
		}
		asap.PlaySong(Song, Duration);
		asap.MutePokeyChannels(MuteMask);
		if (OutputFilename == null) {
			int i = inputFilename.LastIndexOf('.');
			OutputFilename = inputFilename.Substring(0, i + 1) + (OutputHeader ? "wav" : "raw");
		}
		using (Stream s = File.Create(OutputFilename)) {
			byte[] buffer = new byte[8192];
			int nBytes;
			if (OutputHeader) {
				nBytes = asap.GetWavHeader(buffer, Format, false);
				s.Write(buffer, 0, nBytes);
			}
			do {
				nBytes = asap.Generate(buffer, buffer.Length, Format);
				s.Write(buffer, 0, nBytes);
			} while (nBytes == buffer.Length);
		}
		OutputFilename = null;
		Song = -1;
		Duration = -1;
	}

	public static int Main(string[] args)
	{
		bool noInputFiles = true;
		for (int i = 0; i < args.Length; i++) {
			string arg = args[i];
			if (arg[0] != '-') {
				ProcessFile(arg);
				noInputFiles = false;
			}
			else if (arg == "-o")
				OutputFilename = args[++i];
			else if (arg.StartsWith("--output="))
				OutputFilename = arg.Substring(9);
			else if (arg == "-s")
				SetSong(args[++i]);
			else if (arg.StartsWith("--song="))
				SetSong(arg.Substring(7));
			else if (arg == "-t")
				SetTime(args[++i]);
			else if (arg.StartsWith("--time="))
				SetTime(arg.Substring(7));
			else if (arg == "-b" || arg == "--byte-samples")
				Format = ASAPSampleFormat.U8;
			else if (arg == "-w" || arg == "--word-samples")
				Format = ASAPSampleFormat.S16LE;
			else if (arg == "--raw")
				OutputHeader = false;
			else if (arg == "-m")
				SetMuteMask(args[++i]);
			else if (arg.StartsWith("--mute="))
				SetMuteMask(arg.Substring(7));
			else if (arg == "-h" || arg == "--help") {
				PrintHelp();
				noInputFiles = false;
			}
			else if (arg == "-v" || arg == "--version") {
				Console.WriteLine("ASAP2WAV (.NET) " + ASAPInfo.Version);
				noInputFiles = false;
			}
			else
				throw new ArgumentException("unknown option: " + arg);
		}
		if (noInputFiles) {
			PrintHelp();
			return 1;
		}
		return 0;
	}
}
