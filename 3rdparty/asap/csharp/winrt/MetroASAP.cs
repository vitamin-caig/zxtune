/*
 * MetroASAP.cs - Windows 8 Metro Interface version of ASAP
 *
 * Copyright (C) 2012  Piotr Fusik
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
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using System.Threading.Tasks;
using Windows.ApplicationModel.Activation;
using Windows.Foundation;
using Windows.Media;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.Storage.Streams;
using Windows.UI;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

using Sf.Asap;

class ASAPRandomAccessStream : IRandomAccessStream
{
	readonly ASAP asap;
	readonly byte[] wavHeader = new byte[44];
	readonly ulong size;
	ulong position;
	const int BufferSize = 8192; // must be power of two
	readonly byte[] samples = new byte[BufferSize];
	bool needSeek = false;

	public ASAPRandomAccessStream(ASAP asap, int duration)
	{
		this.asap = asap;
		asap.GetWavHeader(this.wavHeader, ASAPSampleFormat.S16LE, false);
		this.size = 8 + BitConverter.ToUInt32(wavHeader, 4);
		this.position = 0;
	}

	uint Read(IBuffer buffer, uint offset, uint count)
	{
		if (this.needSeek) {
			if (this.position < 44)
				asap.Seek(0);
			else {
				ulong bytes = this.position - 44 & ~(BufferSize - 1UL);
				asap.SeekSample((int) (bytes / wavHeader[32]));
			}
			this.needSeek = false;
		}

		ulong left = this.size - this.position;
		if (count > left)
			count = (uint) left;
		if (this.position < 44UL) {
			uint n = Math.Min(44 - (uint) this.position, count);
			this.wavHeader.CopyTo((int) this.position, buffer, offset, (int) n);
			this.position += n;
			offset += n;
			count -= n;
		}
		while (count > 0) {
			int i = (int) this.position - 44 & BufferSize - 1;
			if (i == 0)
				asap.Generate(this.samples, BufferSize, ASAPSampleFormat.S16LE);
			uint n = Math.Min((uint) BufferSize - (uint) i, count);
			this.samples.CopyTo(i, buffer, offset, (int) n);
			this.position += n;
			offset += n;
			count -= n;
		}
		return offset;
	}

	IAsyncOperationWithProgress<IBuffer, uint> IInputStream.ReadAsync(IBuffer buffer, uint count, InputStreamOptions options)
	{
#if true
		return AsyncInfo.Run(async delegate (CancellationToken cancel, IProgress<uint> progress) {
			cancel.ThrowIfCancellationRequested();
			buffer.Length = Read(buffer, buffer.Length, count);
			progress.Report(buffer.Length);
			return buffer;
		});
#else
		return AsyncInfo.Run((CancellationToken cancel, IProgress<uint> progress) => {
			var task = new Task<IBuffer>(() => {
				cancel.ThrowIfCancellationRequested();
				buffer.Length = Read(buffer, buffer.Length, count);
				progress.Report(buffer.Length);
				return buffer;
			}, cancel);
			task.Start();
			return task;
		});
#endif
	}

	void IRandomAccessStream.Seek(ulong position)
	{
		if (position != this.position) {
			this.position = position;
			this.needSeek = true;
		}
	}

	bool IRandomAccessStream.CanRead
	{
		get
		{
			return true;
		}
	}

	bool IRandomAccessStream.CanWrite
	{
		get
		{
			return false;
		}
	}

	ulong IRandomAccessStream.Position
	{
		get
		{
			return this.position;
		}
	}

	ulong IRandomAccessStream.Size
	{
		get
		{
			return this.size;
		}
		set
		{
			throw new NotSupportedException();
		}
	}

	IRandomAccessStream IRandomAccessStream.CloneStream()
	{
		throw new NotImplementedException();
	}

	IInputStream IRandomAccessStream.GetInputStreamAt(ulong position)
	{
		throw new NotImplementedException();
	}

	IOutputStream IRandomAccessStream.GetOutputStreamAt(ulong position)
	{
		throw new NotSupportedException();
	}

	IAsyncOperationWithProgress<uint, uint> IOutputStream.WriteAsync(IBuffer buffer)
	{
		throw new NotSupportedException();
	}

	IAsyncOperation<bool> IOutputStream.FlushAsync()
	{
		throw new NotSupportedException();
	}

	public void Dispose()
	{
	}
}

class MetroASAP : Application
{
	MediaElement MyMedia;

	static IAsyncOperation<StorageFile> PickFile()
	{
		FileOpenPicker fop = new FileOpenPicker {
			FileTypeFilter = { ".sap", ".cmc", ".cm3", ".cmr", ".cms", ".dmc", ".dlt", ".mpt", ".mpd", ".rmt", ".tmc", ".tm8", ".tm2", ".fc" },
			SuggestedStartLocation = PickerLocationId.ComputerFolder // Downloads, Desktop, MusicLibrary?
		};
		return fop.PickSingleFileAsync();
	}

	async Task PlayFile(StorageFile sf)
	{
		if (sf == null) {
			Exit();
			return;
		}
		byte[] module = new byte[ASAPInfo.MaxModuleLength];
		int moduleLen;
		using (IInputStream iis = await sf.OpenSequentialReadAsync()) {
			IBuffer buf = await iis.ReadAsync(module.AsBuffer(), (uint) ASAPInfo.MaxModuleLength, InputStreamOptions.None);
			moduleLen = (int) buf.Length;
		}

		ASAP asap = new ASAP();
		asap.Load(sf.Name, module, moduleLen);
		ASAPInfo info = asap.GetInfo();
		int song = info.GetDefaultSong();
		int duration = info.GetLoop(song) ? -1 : info.GetDuration(song);
		asap.PlaySong(song, duration);

		this.MyMedia = new MediaElement {
			AudioCategory = AudioCategory.BackgroundCapableMedia,
			Volume = 1,
			AutoPlay = true
		};
		Window.Current.Content = this.MyMedia;
		await Task.Yield();
		this.MyMedia.SetSource(new ASAPRandomAccessStream(asap, duration), "audio/x-wav");

		MediaControl.TrackName = info.GetTitleOrFilename();
		MediaControl.ArtistName = info.GetAuthor();
		MediaControl.PlayPressed += MediaControl_PlayPressed;
		MediaControl.PausePressed += MediaControl_PausePressed;
		MediaControl.PlayPauseTogglePressed += MediaControl_PlayPauseTogglePressed;
		MediaControl.StopPressed += MediaControl_StopPressed;
		MediaControl.IsPlaying = true;
	}

	static void DispatchMediaControl(Func<bool> func)
	{
		Window.Current.Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () => {
			MediaControl.IsPlaying = func();
		});
	}

	void MediaControl_PlayPressed(object sender, object e)
	{
		DispatchMediaControl(() => {
			this.MyMedia.Play();
			return true;
		});
	}

	void MediaControl_PausePressed(object sender, object e)
	{
		DispatchMediaControl(() => {
			this.MyMedia.Pause();
			return false;
		});
	}

	void MediaControl_PlayPauseTogglePressed(object sender, object e)
	{
		DispatchMediaControl(() => {
			if (this.MyMedia.CurrentState == MediaElementState.Paused) {
				this.MyMedia.Play();
				return true;
			}
			else {
				this.MyMedia.Pause();
				return false;
			}
		});
	}

	void MediaControl_StopPressed(object sender, object e)
	{
		DispatchMediaControl(() => {
			this.MyMedia.Stop();
			return false;
		});
	}

	protected override async void OnLaunched(LaunchActivatedEventArgs args)
	{
		Window.Current.Activate();
		StorageFile sf = await PickFile();
		await PlayFile(sf);
	}

	protected override async void OnFileActivated(FileActivatedEventArgs args)
	{
		StorageFile sf = args.Files[0] as StorageFile;
		await PlayFile(sf);
	}

	public static void Main(string[] args)
	{
		Application.Start(p => new MetroASAP());
	}
}
