/*
 * loadsap.cs - print files with ASAP loading errors
 *
 * Copyright (C) 2011  Piotr Fusik
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

public class LoadSap
{
	static void Check(string path)
	{
		if (Directory.Exists(path)) {
			foreach (string child in Directory.GetFileSystemEntries(path))
				Check(child);
		}
		else if (ASAPInfo.IsOurFile(path)) {
			byte[] module = File.ReadAllBytes(path);
			try {
				ASAP asap = new ASAP();
				asap.Load(path, module, module.Length);
				asap.PlaySong(asap.GetInfo().GetDefaultSong(), -1);
			}
			catch (Exception ex) {
				Console.WriteLine("{0}: {1}", path, ex.Message);
			}
		}
	}

	public static int Main(string[] args)
	{
		if (args.Length == 0) {
			Console.WriteLine("Usage: loadsap FILE_OR_DIR...");
			return 1;
		}
		foreach (string path in args)
			Check(path);
		return 0;
	}
}
