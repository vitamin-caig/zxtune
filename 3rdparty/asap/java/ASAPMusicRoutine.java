/*
 * ASAPMusicRoutine.java - music embeddable in an Atari program
 *
 * Copyright (C) 2011-2012  Piotr Fusik
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

package net.sf.asap;

import java.io.ByteArrayOutputStream;

/**
 * Music embeddable in an Atari program.
 * Use this class to transform music into a routine
 * that can be called from your 6502 assembly language program.
 * Link your program with the binary returned by {@link #getBinary()}.
 * Call the music routine as specified by {@link #getInitAddress()},
 * {@link #isFulltime()}, {@link #getPlayerAddress()} and {@link #getPlayerRate()}.
 */
public class ASAPMusicRoutine
{
	private final byte[] binary;
	private final int initAddress;
	private final boolean fulltime;
	private final int playerAddress;
	private final int playerRate;

	/**
	 * Transforms music data into a form that can be embedded in an Atari 8-bit program.
	 * @param filename Name of the source music file. The extension is used to identify the format. No I/O is performed.
	 * @param module Contents of the source music file. Only <code>moduleLen</code> leading bytes are meaningful.
	 * @param moduleLen Length of the source music file.
	 * @throws Exception if the source is invalid, not supported by ASAP or cannot be converted to ASAPMusicRoutine.
	 */
	public ASAPMusicRoutine(String filename, byte[] module, int moduleLen) throws Exception
	{
		ASAPInfo info = new ASAPInfo();
		info.load(filename, module, moduleLen);
		final ByteArrayOutputStream baos = new ByteArrayOutputStream();
		int[] initAndPlayer = new int[2];
		ASAPWriter.writeExecutable(
			new ByteWriter() { public void run(int data) { baos.write(data); } },
			initAndPlayer, info, module, moduleLen);
		binary = baos.toByteArray();
		initAddress = initAndPlayer[0];
		// TODO: SAP_S
		fulltime = info.type == ASAPModuleType.SAP_D;
		playerAddress = initAndPlayer[1];
		playerRate = info.fastplay;
	}

	/**
	 * Returns music data.
	 * The data is in the format of Atari 8-bit executables:
	 * two <code>0xff</code> bytes followed by multiple blocks loaded
	 * at different addresses.
	 * It is not a complete program, there is no init or run address.
	 * Currently the loading addresses can be as low as <code>0x200</code>
	 * which normally conflicts with the Atari operating system.
	 */
	public byte[] getBinary() { return binary; }

	/**
	 * Returns address of the initialization routine.
	 * The initialization routine is 6502 code which should be called once
	 * before the playback. There can be more than one song available -
	 * load the zero-based song number into the accumulator.
	 * For correct playback, you should reset the audio part of POKEY.
	 * Example stereo-ready music initialization code:
	 * <pre>
	 *     lda #3
	 *     sta $d21f
	 *     sta $d20f
	 *     ldx #8
	 *     lda #0
	 * initPokey_1 sta $d210,x
	 *     sta $d200,x
	 *     dex
	 *     bpl initPokey_1
	 *     lda #0 ; first song
	 *     jsr initMusic ; use the address returned by {@link #getInitAddress()}
	 * </pre>
	 */
	public int getInitAddress() { return initAddress; }

	/**
	 * Returns <code>true</code> if the initialization routine doesn't return
	 * and actually performs the playback.
	 * This usually means that the music contains digitalized sounds
	 * and requires full 6502 processing power.
	 * It is likely that you also need to disable screen DMA
	 * and operating system interrupts for correct playback.
	 */
	public boolean isFulltime() { return fulltime; }

	/**
	 * Returns address of the player routine.
	 * The player routine should be called at a constant rate
	 * with a <code>JSR</code>.
	 */
	public int getPlayerAddress() { return playerAddress; }

	/**
	 * Returns the rate at which the player routine should be called.
	 * The value is expressed as an interval in scanlines (one scanline equals 114 cycles).
	 * Typical value is 312, which means that on a PAL Atari you should
	 * call the player routine once per frame (usually on a vertical blank interrupt).
	 */
	public int getPlayerRate() { return playerRate; }
}
