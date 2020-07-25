/*
 * SilverASAP.cs - Silverlight version of ASAP
 *
 * Copyright (C) 2010-2013  Piotr Fusik
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
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Net;
using System.Windows;
using System.Windows.Browser;
using System.Windows.Controls;
using System.Windows.Media;

using Sf.Asap;

class ASAPMediaStreamSource : MediaStreamSource
{
	const int BitsPerSample = 16;
	static readonly Dictionary<MediaSampleAttributeKeys, string> SampleAttributes = new Dictionary<MediaSampleAttributeKeys, string>();

	readonly ASAP Asap;
	readonly int Duration;
	MediaStreamDescription MediaStreamDescription;
	readonly byte[] buffer = new byte[8192];

	public ASAPMediaStreamSource(ASAP asap, int duration)
	{
		this.Asap = asap;
		this.Duration = duration;
	}

	static int SwapBytes(int x)
	{
		return x << 24 | (x & 0xff00) << 8 | ((x >> 8) & 0xff00) | ((x >> 24) & 0xff);
	}

	protected override void OpenMediaAsync()
	{
		int channels = this.Asap.GetInfo().GetChannels();
		int blockSize = channels * BitsPerSample >> 3;
		string waveFormatHex = string.Format("0100{0:X2}00{1:X8}{2:X8}{3:X2}00{4:X2}000000",
			channels, SwapBytes(ASAP.SampleRate), SwapBytes(ASAP.SampleRate * blockSize), blockSize, BitsPerSample);
		Dictionary<MediaStreamAttributeKeys, string> streamAttributes = new Dictionary<MediaStreamAttributeKeys, string>();
		streamAttributes[MediaStreamAttributeKeys.CodecPrivateData] = waveFormatHex;
		this.MediaStreamDescription = new MediaStreamDescription(MediaStreamType.Audio, streamAttributes);

		Dictionary<MediaSourceAttributesKeys, string> sourceAttributes = new Dictionary<MediaSourceAttributesKeys, string>();
		sourceAttributes[MediaSourceAttributesKeys.CanSeek] = "True";
		sourceAttributes[MediaSourceAttributesKeys.Duration] = (this.Duration < 0 ? 0 : this.Duration * 10000L).ToString();

		ReportOpenMediaCompleted(sourceAttributes, new MediaStreamDescription[1] { this.MediaStreamDescription });
	}

	protected override void GetSampleAsync(MediaStreamType mediaStreamType)
	{
		int blocksPlayed = this.Asap.GetBlocksPlayed();
		int bufferLen = this.Asap.Generate(buffer, buffer.Length, BitsPerSample == 8 ? ASAPSampleFormat.U8 : ASAPSampleFormat.S16LE);
		Stream s = bufferLen == 0 ? null : new MemoryStream(buffer);
		MediaStreamSample mss = new MediaStreamSample(this.MediaStreamDescription, s, 0, bufferLen,
			blocksPlayed * 10000000L / ASAP.SampleRate, SampleAttributes);
		ReportGetSampleCompleted(mss);
	}

	protected override void SeekAsync(long seekToTime)
	{
		this.Asap.Seek((int) (seekToTime / 10000));
		ReportSeekCompleted(seekToTime);
	}

	public event EventHandler StreamCompleted;

	protected override void CloseMedia()
	{
		if (this.StreamCompleted != null)
			this.StreamCompleted(this, new EventArgs());
	}

	protected override void SwitchMediaStreamAsync(MediaStreamDescription mediaStreamDescription)
	{
		throw new NotImplementedException();
	}

	protected override void GetDiagnosticAsync(MediaStreamSourceDiagnosticKind diagnosticKind)
	{
		throw new NotImplementedException();
	}

}

public class SilverASAP : Application
{
	readonly MediaElement MediaElement = new MediaElement();
	string Filename;
	int Song = -1;
	WebClient WebClient = null;
	ASAPInfo Info;

	public SilverASAP()
	{
		this.Startup += this.Application_Startup;
	}

	void CallJS(string paramName)
	{
		string js;
		if (this.Host.InitParams.TryGetValue(paramName, out js))
			HtmlPage.Window.Eval(js);
	}

	void MediaElement_MediaEnded(object sender, RoutedEventArgs e)
	{
		CallJS("onPlaybackEnd");
	}

	void WebClient_OpenReadCompleted(object sender, OpenReadCompletedEventArgs e)
	{
		this.WebClient = null;
		if (e.Cancelled || e.Error != null)
			return;
		byte[] module = new byte[e.Result.Length];
		int moduleLen = e.Result.Read(module, 0, module.Length);

		ASAP asap = new ASAP();
		asap.Load(this.Filename, module, moduleLen);
		this.Info = asap.GetInfo();
		if (this.Song < 0)
			this.Song = this.Info.GetDefaultSong();
		int duration = this.Info.GetLoop(this.Song) ? -1 : this.Info.GetDuration(this.Song);
		asap.PlaySong(this.Song, duration);

		Stop();
		CallJS("onLoad");
		this.MediaElement.SetSource(new ASAPMediaStreamSource(asap, duration));
	}

	[ScriptableMember]
	public void Play(string filename, int song)
	{
		this.Filename = filename;
		this.Song = song;
		this.WebClient = new WebClient();
		this.WebClient.OpenReadCompleted += WebClient_OpenReadCompleted;
		this.WebClient.OpenReadAsync(new Uri(filename, UriKind.Relative));
	}

	[ScriptableMember]
	public void Play(string filename)
	{
		Play(filename, -1);
	}

	[ScriptableMember]
	public bool Pause()
	{
		if (this.MediaElement.CurrentState == MediaElementState.Playing) {
			this.MediaElement.Pause();
			return true;
		}
		else {
			this.MediaElement.Play();
			return false;
		}
	}

	[ScriptableMember]
	public void Stop()
	{
		if (this.WebClient != null)
			this.WebClient.CancelAsync();
		this.MediaElement.Stop();
		// in Opera Stop() doesn't work when MediaElement is in the Opening state:
		this.MediaElement.Source = null;
	}

	[ScriptableMember]
	public string Author
	{
		get
		{
			return this.Info.GetAuthor();
		}
	}

	[ScriptableMember]
	public string Title
	{
		get
		{
			return this.Info.GetTitle();
		}
	}

	[ScriptableMember]
	public string Date
	{
		get
		{
			return this.Info.GetDate();
		}
	}

	void Application_Startup(object sender, StartupEventArgs e)
	{
		this.RootVisual = this.MediaElement;
		this.MediaElement.Volume = 1;
		this.MediaElement.AutoPlay = true;
		this.MediaElement.MediaEnded += MediaElement_MediaEnded;
		HtmlPage.RegisterScriptableObject("ASAP", this);
		string filename;
		if (e.InitParams.TryGetValue("file", out filename)) {
			string s;
			int song = -1;
			if (e.InitParams.TryGetValue("song", out s))
				song = int.Parse(s, CultureInfo.InvariantCulture);
			Play(filename, song);
		}
	}
}
