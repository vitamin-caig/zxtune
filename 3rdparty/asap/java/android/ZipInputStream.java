/*
 * ZipInputStream.java - ASAP for Android
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

import java.io.FilterInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.util.zip.ZipFile;

class ZipInputStream extends FilterInputStream
{
	private final ZipFile zip;

	private ZipInputStream(ZipFile zip, String filename) throws IOException
	{
		super(zip.getInputStream(zip.getEntry(filename)));
		this.zip = zip;
	}

	ZipInputStream(String zipFilename, String filename) throws IOException
	{
		this(new ZipFile(zipFilename), filename);
	}

	@Override
	public void close() throws IOException
	{
		super.close();
		zip.close();
	}

	InputStream openInputStream(String filename) throws IOException
	{
		return zip.getInputStream(zip.getEntry(filename));
	}
}
