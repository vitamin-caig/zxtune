/*
 * ASAPApplet.java - ASAP applet
 *
 * Copyright (C) 2007-2012  Piotr Fusik
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

import java.applet.Applet;
import java.awt.Color;
import java.awt.Graphics;
import java.io.InputStream;
import java.io.IOException;
import java.net.URL;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.SourceDataLine;
import netscape.javascript.JSObject;

import net.sf.asap.ASAP;
import net.sf.asap.ASAPInfo;
import net.sf.asap.ASAPSampleFormat;

public class ASAPApplet extends Applet implements Runnable
{
	private final ASAP asap = new ASAP();
	private int song;
	private SourceDataLine line;
	private boolean running;
	private boolean paused;

	private Color background;
	private Color foreground;

	public void update(Graphics g)
	{
		if (running)
			paint(g);
		else
			super.update(g);
	}

	public void paint(Graphics g)
	{
		if (!running)
			return;
		int channels = 4 * asap.getInfo().getChannels();
		int channelWidth = getWidth() / channels;
		int totalHeight = getHeight();
		int unitHeight = totalHeight / 15;
		for (int i = 0; i < channels; i++) {
			int height = asap.getPokeyChannelVolume(i) * unitHeight;
			g.setColor(background);
			g.fillRect(i * channelWidth, 0, channelWidth, totalHeight - height);
			g.setColor(foreground);
			g.fillRect(i * channelWidth, totalHeight - height, channelWidth, height);
		}
	/*
		ASAPInfo info = asap.getInfo();
		g.drawString("Author: " + info.getAuthor(), 10, 20);
		g.drawString("Name: " + info.getTitleOrFilename(), 10, 40);
		g.drawString("Date: " + info.getDate(), 10, 60);
		if (info.getSongs() > 1)
			g.drawString("Song " + (song + 1) + " of " + info.getSongs(), 10, 80);
	*/
	}

	private void callJS(String paramName)
	{
		String js = getParameter(paramName);
		if (js != null)
			JSObject.getWindow(this).eval(js);
	}

	public void run()
	{
		byte[] buffer = new byte[8192];
		int len;
		do {
			synchronized (asap) {
				len = asap.generate(buffer, buffer.length, ASAPSampleFormat.S16_L_E);
			}
			synchronized (this) {
				while (paused) {
					try {
						wait();
					} catch (InterruptedException e) {
						return;
					}
				}
			}
			line.write(buffer, 0, len);
			repaint();
			if (len != buffer.length) {
				callJS("onPlaybackEnd");
				running = false;
			}
		} while (running);
		repaint();
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

	public void play(String filename, int song)
	{
		byte[] module;
		int moduleLen;
		try {
			InputStream is = new URL(getDocumentBase(), filename).openStream();
			module = new byte[ASAPInfo.MAX_MODULE_LENGTH];
			moduleLen = readAndClose(is, module);
		} catch (IOException e) {
			showStatus("ERROR LOADING " + filename);
			return;
		}
		ASAPInfo info;
		synchronized (asap) {
			try {
				asap.load(filename, module, moduleLen);
				info = asap.getInfo();
				if (song < 0)
					song = info.getDefaultSong();
				asap.playSong(song, info.getLoop(song) ? -1 : info.getDuration(song));
			} catch (Exception e) {
				showStatus(e.getMessage());
				return;
			}
		}
		AudioFormat format = new AudioFormat(ASAP.SAMPLE_RATE, 16, info.getChannels(), true, false);
		try {
			line = (SourceDataLine) AudioSystem.getLine(new DataLine.Info(SourceDataLine.class, format));
			line.open(format);
		} catch (LineUnavailableException e) {
			showStatus("ERROR OPENING AUDIO");
			return;
		}
		callJS("onLoad");
		line.start();
		if (!running) {
			running = true;
			new Thread(this).start();
		}
		synchronized (this) {
			paused = false;
			notify();
		}
		repaint();
	}

	private Color getColor(String parameter, Color defaultColor)
	{
		String s = getParameter(parameter);
		if (s == null || s.length() == 0)
			return defaultColor;
		if (s.charAt(0) == '#')
			s = s.substring(1);
		return new Color(Integer.parseInt(s, 16));
	}

	public void start()
	{
		background = getColor("background", Color.BLACK);
		setBackground(background);
		foreground = getColor("foreground", Color.GREEN);
		String filename = getParameter("file");
		if (filename == null)
			return;
		int song = -1;
		String s = getParameter("song");
		if (s == null || s.length() == 0)
			song = -1;
		else
			song = Integer.parseInt(s);
		play(filename, song);
	}

	public void stop()
	{
		running = false;
	}

	public synchronized boolean togglePause()
	{
		paused = !paused;
		if (!paused)
			notify();
		return paused;
	}

	public String getAuthor()
	{
		return asap.getInfo().getAuthor();
	}

	public String getTitle()
	{
		return asap.getInfo().getTitle();
	}

	public String getDate()
	{
		return asap.getInfo().getDate();
	}
}
