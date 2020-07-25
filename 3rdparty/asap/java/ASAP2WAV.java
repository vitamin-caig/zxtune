/*
 * ASAP2WAV.java - converter of ASAP-supported formats to WAV files
 *
 * Copyright (C) 2007-2011  Piotr Fusik
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

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import net.sf.asap.ASAP;
import net.sf.asap.ASAPInfo;
import net.sf.asap.ASAPSampleFormat;

public class ASAP2WAV
{
	private static String outputFilename = null;
	private static boolean outputHeader = true;
	private static int song = -1;
	private static int format = ASAPSampleFormat.S16_L_E;
	private static int duration = -1;
	private static int muteMask = 0;

	private static void printHelp()
	{
		System.out.print(
			"Usage: java -jar asap2wav.jar [OPTIONS] INPUTFILE...\n" +
			"Each INPUTFILE must be in a supported format:\n" +
			"SAP, CMC, CM3, CMR, CMS, DMC, DLT, MPT, MPD, RMT, TMC, TM8, TM2 or FC.\n" +
			"Options:\n" +
			"-o FILE     --output=FILE      Set output file name\n" +
			"-o -        --output=-         Write to standard output\n" +
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

	private static void setSong(String s)
	{
		song = Integer.parseInt(s);
	}

	private static void setTime(String s) throws Exception
	{
		duration = ASAPInfo.parseDuration(s);
	}

	private static void setMuteMask(String s)
	{
		int mask = 0;
		for (int i = 0; i < s.length(); i++) {
			int c = s.charAt(i);
			if (c >= '1' && c <= '8')
				mask |= 1 << (c - '1');
		}
		muteMask = mask;
	}

	/**
	 * Reads bytes from the stream into the byte array
	 * until end of stream or array is full.
	 * @param is source stream
	 * @param b output array
	 * @return number of bytes read
	 */
	private static int readAndClose(InputStream is, byte[] b) throws IOException
	{
		int got = 0;
		int len = b.length;
		try {
			while (got < len) {
				int i = is.read(b, got, len - got);
				if (i <= 0)
					break;
				got += i;
			}
		}
		finally {
			is.close();
		}
		return got;
	}

	private static void processFile(String inputFilename) throws Exception
	{
		InputStream is = new FileInputStream(inputFilename);
		byte[] module = new byte[ASAPInfo.MAX_MODULE_LENGTH];
		int moduleLen = readAndClose(is, module);
		ASAP asap = new ASAP();
		asap.load(inputFilename, module, moduleLen);
		ASAPInfo info = asap.getInfo();
		if (song < 0)
			song = info.getDefaultSong();
		if (duration < 0) {
			duration = info.getDuration(song);
			if (duration < 0)
				duration = 180 * 1000;
		}
		asap.playSong(song, duration);
		asap.mutePokeyChannels(muteMask);
		if (outputFilename == null) {
			int i = inputFilename.lastIndexOf('.');
			outputFilename = inputFilename.substring(0, i + 1) + (outputHeader ? "wav" : "raw");
		}
		OutputStream os;
		if (outputFilename.equals("-"))
			os = System.out;
		else
			os = new FileOutputStream(outputFilename);
		byte[] buffer = new byte[8192];
		int nBytes;
		if (outputHeader) {
			nBytes = asap.getWavHeader(buffer, format, false);
			os.write(buffer, 0, nBytes);
		}
		do {
			nBytes = asap.generate(buffer, buffer.length, format);
			os.write(buffer, 0, nBytes);
		} while (nBytes == buffer.length);
		os.close();
		outputFilename = null;
		song = -1;
		duration = -1;
	}

	public static void main(String[] args) throws Exception
	{
		boolean noInputFiles = true;
		for (int i = 0; i < args.length; i++) {
			String arg = args[i];
			if (arg.charAt(0) != '-') {
				processFile(arg);
				noInputFiles = false;
			}
			else if (arg.equals("-o"))
				outputFilename = args[++i];
			else if (arg.startsWith("--output="))
				outputFilename = arg.substring(9);
			else if (arg.equals("-s"))
				setSong(args[++i]);
			else if (arg.startsWith("--song="))
				setSong(arg.substring(7));
			else if (arg.equals("-t"))
				setTime(args[++i]);
			else if (arg.startsWith("--time="))
				setTime(arg.substring(7));
			else if (arg.equals("-b") || arg.equals("--byte-samples"))
				format = ASAPSampleFormat.U8;
			else if (arg.equals("-w") || arg.equals("--word-samples"))
				format = ASAPSampleFormat.S16_L_E;
			else if (arg.equals("--raw"))
				outputHeader = false;
			else if (arg.equals("-m"))
				setMuteMask(args[++i]);
			else if (arg.startsWith("--mute="))
				setMuteMask(arg.substring(7));
			else if (arg.equals("-h") || arg.equals("--help")) {
				printHelp();
				noInputFiles = false;
			}
			else if (arg.equals("-v") || arg.equals("--version")) {
				System.out.println("ASAP2WAV (Java) " + ASAPInfo.VERSION);
				noInputFiles = false;
			}
			else
				throw new IllegalArgumentException("unknown option: " + arg);
		}
		if (noInputFiles) {
			printHelp();
			System.exit(1);
		}
	}
}
