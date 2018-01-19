/*
 * ASAPMIDlet.java - ASAP midlet
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

import java.io.IOException;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.Vector;
import javax.microedition.io.Connector;
import javax.microedition.io.file.FileConnection;
import javax.microedition.io.file.FileSystemRegistry;
import javax.microedition.lcdui.Alert;
import javax.microedition.lcdui.AlertType;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Gauge;
import javax.microedition.lcdui.List;
import javax.microedition.lcdui.StringItem;
import javax.microedition.media.Manager;
import javax.microedition.media.MediaException;
import javax.microedition.media.Player;
import javax.microedition.media.PlayerListener;
import javax.microedition.midlet.MIDlet;

import net.sf.asap.ASAP;
import net.sf.asap.ASAPInfo;
import net.sf.asap.ASAPSampleFormat;

class FileList
{
	final String caption;
	final String path;
	final String[] names;
	final FileList parent;
	final FileList[] sublists;

	private static boolean lessOrEqual(String name1, String name2)
	{
		boolean dir1 = name1.endsWith("/");
		boolean dir2 = name2.endsWith("/");
		if (dir1 != dir2)
			return dir1;
		return (name1.compareTo(name2) <= 0);
	}

	private void sort(int start, int end)
	{
		while (start + 1 < end) {
			int left = start + 1;
			int right = end;
			String pivot = names[start];
			String tmp;
			while (left < right) {
				if (lessOrEqual(names[left], pivot))
					left++;
				else {
					right--;
					tmp = names[left];
					names[left] = names[right];
					names[right] = tmp;
				}
			}
			left--;
			tmp = names[left];
			names[left] = names[start];
			names[start] = tmp;
			sort(start, left);
			start = right;
		}
	}

	FileList(String caption, String path, FileList parent, Enumeration contents)
	{
		this.caption = caption;
		this.path = path;
		this.parent = parent;
		Vector v = new Vector();
		int start = 0;
		if (parent != null) {
			v.addElement("..");
			start = 1;
		}
		while (contents.hasMoreElements()) {
			String name = (String) contents.nextElement();
			if (name.endsWith("/") || ASAPInfo.isOurFile(name))
				v.addElement(name);
		}
		int n = v.size();
		names = new String[n];
		sublists = new FileList[n];
		for (int i = 0; i < n; i++)
			names[i] = (String) v.elementAt(i);
		sort(start, n);
	}
}

class ASAPInputStream extends InputStream
{
	private final ASAP asap;
	private final ASAPMIDlet midlet;
	private final byte[] buffer = new byte[16384];
	private int bufferLen;
	private int bufferOffset = 0;

	private static final int FORMAT = ASAPSampleFormat.U8;

	ASAPInputStream(ASAP asap, ASAPMIDlet midlet)
	{
		this.asap = asap;
		this.midlet = midlet;
		this.bufferLen = asap.getWavHeader(buffer, FORMAT, false);
	}

	public int read()
	{
		int i = bufferOffset;
		if (i >= bufferLen) {
			bufferLen = asap.generate(buffer, buffer.length, FORMAT);
			if (bufferLen == 0)
				return -1;
			midlet.updatePosition();
			i = 0;
		}
		bufferOffset = i + 1;
		return buffer[i] & 0xff;
	}

	public int read(byte[] b, int off, int len)
	{
		int i = bufferOffset;
		if (i >= bufferLen) {
			bufferLen = asap.generate(buffer, buffer.length, FORMAT);
			if (bufferLen == 0)
				return -1;
			midlet.updatePosition();
			i = 0;
		}
		if (len > bufferLen - i)
			len = bufferLen - i;
		System.arraycopy(buffer, i, b, off, len);
		bufferOffset = i + len;
		return len;
	}
}

public class ASAPMIDlet extends MIDlet implements CommandListener, PlayerListener
{
	private Command selectCommand;
	private Command backCommand;
	private Command stopCommand;
	private Command exitCommand;
	private FileList fileList;
	private List fileListControl;
	private byte[] module;
	private ASAP asap;
	private Player player;
	private List songListControl;
	private Gauge gauge;

	private void displayFileList(FileList fileList)
	{
		this.fileList = fileList;
		this.fileListControl = new List(fileList.caption, List.IMPLICIT, fileList.names, null);
		fileListControl.addCommand(selectCommand);
		fileListControl.addCommand(exitCommand);
		fileListControl.setCommandListener(this);
		Display.getDisplay(this).setCurrent(fileListControl);
	}

	private void displayError(String message)
	{
		Alert alert = new Alert("Error", message, null, AlertType.ERROR);
		Display.getDisplay(this).setCurrent(alert);
	}

	private static void appendStringItem(Form form, String label, String text)
	{
		if (text.length() == 0)
			return;
		StringItem si = new StringItem(label, text);
		si.setLayout(StringItem.LAYOUT_NEWLINE_AFTER);
		form.append(si);
	}

	public void updatePosition()
	{
		gauge.setValue(asap.getPosition());
	}

	private static void setTwoDigits(char[] chars, int offset, int value)
	{
		chars[offset] = (char) ('0' + value / 10);
		chars[offset + 1] = (char) ('0' + value % 10);
	}

	private static String durationToString(int duration)
	{
		if (duration < 0 || duration >= 100 * 60 * 1000)
			return "";
		char[] chars = new char[5];
		int seconds = duration / 1000;
		setTwoDigits(chars, 0, seconds / 60);
		chars[2] = ':';
		setTwoDigits(chars, 3, seconds % 60);
		return new String(chars);
	}

	private void playSong(int song) throws Exception
	{
		ASAPInfo info = asap.getInfo();
		int duration = info.getDuration(song);

		gauge = new Gauge(null, false, 1, 0);
		Form playForm = new Form("ASAP " + ASAPInfo.VERSION);
		appendStringItem(playForm, "Name: ", info.getTitleOrFilename());
		appendStringItem(playForm, "Author: ", info.getAuthor());
		appendStringItem(playForm, "Date: ", info.getDate());
		appendStringItem(playForm, "Time: ", durationToString(duration));
		playForm.append(gauge);
		playForm.addCommand(stopCommand);
		playForm.addCommand(exitCommand);
		playForm.setCommandListener(this);
		Display.getDisplay(this).setCurrent(playForm);

		if (duration < 0)
			duration = 180000;
		gauge.setMaxValue(duration);
		asap.playSong(song, duration);
		InputStream is = new ASAPInputStream(asap, this);
		player = Manager.createPlayer(is, "audio/x-wav");
		player.addPlayerListener(this);
		player.start();
	}

	public void startApp()
	{
		selectCommand = new Command("Select", Command.ITEM, 1);
		backCommand = new Command("Back", Command.BACK, 1);
		stopCommand = new Command("Stop", Command.STOP, 1);
		exitCommand = new Command("Exit", Command.EXIT, 2);
		module = new byte[ASAPInfo.MAX_MODULE_LENGTH];
		asap = new ASAP();
		displayFileList(new FileList("Select File", "", null, FileSystemRegistry.listRoots()));
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

	public void commandAction(Command c, Displayable s)
	{
		if (c == selectCommand || c == List.SELECT_COMMAND) {
			try {
				if (songListControl != null) {
					playSong(songListControl.getSelectedIndex());
					songListControl = null;
					return;
				}
				int index = fileListControl.getSelectedIndex();
				String filename = fileList.names[index];
				if (filename.equals("..")) {
					displayFileList(fileList.parent);
					return;
				}
				FileList newList = fileList.sublists[index];
				if (newList != null) {
					displayFileList(newList);
					return;
				}
				String path = fileList.path + filename;
				FileConnection fc = (FileConnection) Connector.open("file:///" + path, Connector.READ);
				if (fc.isDirectory()) {
					newList = new FileList(filename, path, fileList, fc.list());
					fileList.sublists[index] = newList;
					displayFileList(newList);
					return;
				}
				InputStream is = fc.openInputStream();
				int moduleLen = readAndClose(is, module);
				asap.load(filename, module, moduleLen);
				ASAPInfo info = asap.getInfo();
				int songs = info.getSongs();
				if (songs > 1) {
					songListControl = new List(info.getTitleOrFilename(), List.IMPLICIT);
					for (int i = 1; i <= songs; i++)
						songListControl.append("Song " + i, null);
					songListControl.setSelectedIndex(info.getDefaultSong(), true);
					songListControl.addCommand(selectCommand);
					songListControl.addCommand(backCommand);
					songListControl.setCommandListener(this);
					Display.getDisplay(this).setCurrent(songListControl);
				}
				else
					playSong(0);
			} catch (Exception ex) {
				displayError(ex.toString());
			}
		}
		else if (c == backCommand) {
			songListControl = null;
			Display.getDisplay(this).setCurrent(fileListControl);
		}
		else if (c == stopCommand) {
			try {
				player.stop();
			} catch (MediaException ex) {
			}
			Display.getDisplay(this).setCurrent(fileListControl);
		}
		else if (c == exitCommand)
			notifyDestroyed();
	}

	public void playerUpdate(Player player, String event, Object eventData)
	{
		if (event == PlayerListener.END_OF_MEDIA)
			Display.getDisplay(this).setCurrent(fileListControl);
		else if (event == PlayerListener.ERROR)
			displayError((String) eventData);
	}

	public void pauseApp()
	{
	}

	public void destroyApp(boolean unconditional)
	{
	}
}
