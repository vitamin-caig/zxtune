/*
 * AATRStream.java - ASAP for Android
 *
 * Copyright (C) 2013  Piotr Fusik
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

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

class AATRStream extends RandomAccessFile implements RandomAccessInputStream
{
	AATRStream(String filename) throws FileNotFoundException
	{
		super(filename, "r");
	}

	AATR open() throws IOException
	{
		AATR aatr = new AATR();
		if (!aatr.open(this))
			throw new IOException("Invalid ATR file");
		return aatr;
	}

	public boolean run(int offset, byte[] buffer, int length)
	{
		try {
			seek(offset);
			readFully(buffer, 0, length);
			return true;
		}
		catch (IOException ex) {
			return false;
		}
	}
}
