/*
 * ASAPPlayer.as - ASAP Flash player
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

package
{
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.events.SampleDataEvent;
	import flash.external.ExternalInterface;
	import flash.media.Sound;
	import flash.media.SoundChannel;
	import flash.net.URLLoader;
	import flash.net.URLLoaderDataFormat;
	import flash.net.URLRequest;
	import flash.utils.ByteArray;
	import net.sf.asap.ASAP;
	import net.sf.asap.ASAPInfo;

	public class ASAPPlayer extends Sprite
	{
		private var filename : String;
		private var song : int;
		private var asap : ASAP = new ASAP();
		private var soundChannel : SoundChannel = null;

		private function callJS(paramName : String) : void
		{
			var js : String = this.loaderInfo.parameters[paramName];
			if (js != null)
				ExternalInterface.call(js);
		}

		private function startPlayback() : void
		{
			var sound : Sound = new Sound();
			function generator(event : SampleDataEvent) : void
			{
				asap.generate(event.data, 8192, 0);
			}
			sound.addEventListener(SampleDataEvent.SAMPLE_DATA, generator);
			this.soundChannel = sound.play();
			function soundCompleteHandler(event : Event) : void
			{
				callJS("onPlaybackEnd");
			}
			this.soundChannel.addEventListener(Event.SOUND_COMPLETE, soundCompleteHandler);
		}

		private function loadCompleteHandler(event : Event) : void
		{
			var module : ByteArray = URLLoader(event.target).data;
			stop();
			this.asap.load(this.filename, module, module.length);
			var info : ASAPInfo = this.asap.getInfo();
			var song : int = this.song;
			if (song < 0)
				song = info.getDefaultSong();
			var duration : int = info.getLoop(song) ? -1 : info.getDuration(song);
			this.asap.playSong(song, duration);
			callJS("onLoad");
			startPlayback();
		}

		public function play(filename : String, song : int = -1) : void
		{
			this.filename = filename;
			this.song = song;
			var loader : URLLoader = new URLLoader();
			loader.dataFormat = URLLoaderDataFormat.BINARY;
			loader.addEventListener(Event.COMPLETE, loadCompleteHandler);
			loader.load(new URLRequest(filename));
		}

		public function pause() : Boolean
		{
			if (this.soundChannel != null) {
				stop();
				return true;
			}
			else {
				startPlayback();
				return false;
			}
		}

		public function stop() : void
		{
			if (this.soundChannel != null) {
				this.soundChannel.stop();
				this.soundChannel = null;
			}
		}

		public function getAuthor() : String
		{
			return this.asap.getInfo().getAuthor();
		}

		public function getTitle() : String
		{
			return this.asap.getInfo().getTitle();
		}

		public function getDate() : String
		{
			return this.asap.getInfo().getDate();
		}

		public function ASAPPlayer()
		{
			ExternalInterface.addCallback("asapPlay", play);
			ExternalInterface.addCallback("asapPause", pause);
			ExternalInterface.addCallback("asapStop", stop);
			ExternalInterface.addCallback("getAuthor", getAuthor);
			ExternalInterface.addCallback("getTitle", getTitle);
			ExternalInterface.addCallback("getDate", getDate);
			var parameters : Object = this.loaderInfo.parameters;
			if (parameters.file != null)
				play(parameters.file, parameters.song != null ? parameters.song : -1);
		}
	}
}
