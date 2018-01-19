/*
 * airasap.js - ASAP for Adobe AIR
 *
 * Copyright (C) 2010-2012  Piotr Fusik
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

var air = {
	ByteArray : window.runtime.flash.utils.ByteArray,
	Event : window.runtime.flash.events.Event,
	File : window.runtime.flash.filesystem.File,
	FileFilter : window.runtime.flash.net.FileFilter,
	FileMode : window.runtime.flash.filesystem.FileMode,
	FileStream : window.runtime.flash.filesystem.FileStream,
	InvokeEvent : window.runtime.flash.events.InvokeEvent,
	NativeApplication : window.runtime.flash.desktop.NativeApplication,
	SampleDataEvent : window.runtime.flash.events.SampleDataEvent,
	Sound : window.runtime.flash.media.Sound
};

var asap = new ASAP();
var info;
var soundChannel;
var song;

function stop()
{
	if (soundChannel != null) {
		soundChannel.stop();
		soundChannel = null;
	}
}

function showTag(id, value)
{
	document.getElementById(id).innerText = value;
}

function generate(event)
{
	if (asap.generate(event.data, 8192, 0) < 8192)
		soundChannel = null;
}

function playSong(i)
{
	song = i;
	showTag("song", i + 1);
	var duration = info.getLoop(i) ? -1 : info.getDuration(i);
	asap.playSong(i, duration);
	if (soundChannel == null) {
		var sound = new air.Sound();
		sound.addEventListener(air.SampleDataEvent.SAMPLE_DATA, generate);
		soundChannel = sound.play();
	}
}

function prevSong()
{
	if (song > 0)
		playSong(song - 1);
}

function nextSong()
{
	if (song + 1 < info.getSongs())
		playSong(song + 1);
}

function displaySongPanel(display)
{
	document.getElementById("song_panel").style.display = display;
}

function playFile(file)
{
	var s = new air.FileStream();
	s.open(file, air.FileMode.READ);
	var module = new air.ByteArray();
	s.readBytes(module);
	s.close();
	stop();
	try {
		asap.load(file.nativePath, module, module.length);
		info = asap.getInfo();
	} catch (ex) {
		displaySongPanel("none");
		alert(ex);
		return;
	}
	showTag("name", info.getTitleOrFilename());
	showTag("author", info.getAuthor());
	showTag("date", info.getDate());
	if (info.getSongs() > 1) {
		showTag("songs", info.getSongs());
		displaySongPanel("block");
	}
	else
		displaySongPanel("none");
	playSong(info.getDefaultSong());
}

function onFileSelect(event)
{
	playFile(event.target);
}

function selectFile()
{
	var selector = new air.File();
	selector.addEventListener(air.Event.SELECT, onFileSelect);
	selector.browseForOpen("Select Atari 8-bit music", [
		new air.FileFilter("All supported", "*.sap;*.cmc;*.cm3;*.cmr;*.cms;*.dmc;*.dlt;*.mpt;*.mpd;*.rmt;*.tmc;*.tm8;*.tm2;*.fc"),
		new air.FileFilter("Slight Atari Player (*.sap)", "*.sap"),
		new air.FileFilter("Chaos Music Composer (*.cmc;*.cm3;*.cmr;*.cms;*.dmc)", "*.cmc;*.cm3;*.cmr;*.cms;*.dmc"),
		new air.FileFilter("Delta Music Composer (*.dlt)", "*.dlt"),
		new air.FileFilter("Music ProTracker (*.mpt;*.mpd)", "*.mpt;*.mpd"),
		new air.FileFilter("Raster Music Tracker (*.rmt)", "*.rmt"),
		new air.FileFilter("Theta Music Composer 1.x (*.tmc;*.tm8)", "*.tmc;*.tm8"),
		new air.FileFilter("Theta Music Composer 2.x (*.tm2)", "*.tm2"),
		new air.FileFilter("Future Composer (*.fc)", "*.fc")
	]);
}

function onInvoke(event)
{
	if (event.arguments.length == 1)
		playFile(new air.File(event.arguments[0]));
}

air.NativeApplication.nativeApplication.addEventListener(air.InvokeEvent.INVOKE, onInvoke);
