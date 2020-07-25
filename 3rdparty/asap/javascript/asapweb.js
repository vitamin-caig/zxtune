/*
 * asapweb.js - pure JavaScript ASAP for web browsers
 *
 * Copyright (C) 2009-2012  Piotr Fusik
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

var asap = {
	timerId : null,
	onLoad : new Function(),
	onPlaybackEnd : new Function(),
	stop : function()
	{
		if (window.asap.timerId) {
			clearInterval(window.asap.timerId);
			window.asap.timerId = null;
		}
	},
	play : function(filename, module, song)
	{
		var asap = new ASAP();
		asap.load(filename, module, module.length);
		var info = asap.getInfo();
		if (song == null)
			song = info.getDefaultSong();
		asap.playSong(song, info.getDuration(song));

		var buffer = new Array(8192);

		function audioCallback(samplesRequested)
		{
			buffer.length = asap.generate(buffer, samplesRequested, ASAPSampleFormat.U8);
			for (var i = 0; i < buffer.length; i++)
				buffer[i] = (buffer[i] - 128) / 128;
			if (buffer.length == 0) {
				window.asap.stop();
				window.asap.onPlaybackEnd();
			}
			return buffer;
		}
		function failureCallback()
		{
			alert("Your browser doesn't support JavaScript audio");
		}
		var audio = new XAudioServer(info.getChannels(), ASAP.SAMPLE_RATE, 8192, 16384, audioCallback, 1, failureCallback);
		function heartbeat()
		{
			audio.executeCallback();
		}

		window.asap.stop();
		window.asap.author = info.getAuthor();
		window.asap.title = info.getTitle();
		window.asap.date = info.getDate();
		window.asap.onLoad();
		window.asap.timerId = setInterval(heartbeat, 50);
	}
};
