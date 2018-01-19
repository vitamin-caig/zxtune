/*
 * PlayerService.java - ASAP for Android
 *
 * Copyright (C) 2010-2014  Piotr Fusik
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

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.net.Uri;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.widget.MediaController;
import android.widget.Toast;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;

public class PlayerService extends Service implements Runnable, MediaController.MediaPlayerControl
{
	// User interface -----------------------------------------------------------------------------------------

	private NotificationManager notMan;
	private static final int NOTIFICATION_ID = 1;

	@Override
	public void onCreate()
	{
		notMan = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
	}

	private void startForegroundCompat(int id, Notification notification)
	{
		if (!Util.invokeMethod(this, "startForeground", id, notification)) {
			// Fall back on the old API.
			Util.invokeMethod(this, "setForeground", true);
			notMan.notify(id, notification);
		}
	}

	private void stopForegroundCompat(int id)
	{
		if (!Util.invokeMethod(this, "stopForeground", true)) {
			// Fall back on the old API.
			// Cancel before changing the foreground state, since we could be killed at that point.
			notMan.cancel(id);
			Util.invokeMethod(this, "setForeground", false);
		}
	}

	private final Handler toastHandler = new Handler();

	private void showError(final int messageId)
	{
		toastHandler.post(new Runnable() {
			public void run() {
				Toast.makeText(PlayerService.this, messageId, Toast.LENGTH_SHORT).show();
			}
		});
	}

	private void showInfo()
	{
		sendBroadcast(new Intent(Player.ACTION_SHOW_INFO));
	}

	private void showNotification()
	{
		PendingIntent contentIntent = PendingIntent.getActivity(this, 0, new Intent(this, Player.class), 0);
		String title = info.getTitleOrFilename();
		Notification notification = new Notification(R.drawable.icon, title, System.currentTimeMillis());
		notification.flags |= Notification.FLAG_ONGOING_EVENT;
		notification.setLatestEventInfo(this, title, info.getAuthor(), contentIntent);
		startForegroundCompat(NOTIFICATION_ID, notification);
	}


	// Playlist -----------------------------------------------------------------------------------------------

	static final String EXTRA_PLAYLIST = "asap.intent.extra.PLAYLIST";

	private final ArrayList<Uri> playlist = new ArrayList<Uri>();

	private void setPlaylist(final Uri uri, boolean shuffle)
	{
		playlist.clear();
		FileContainer container = new FileContainer() {
				@Override
				protected void onSongFile(String name, InputStream is) {
					playlist.add(Util.buildUri(uri, name));
				}
			};
		try {
			container.list(uri, false, true);
			if (shuffle)
				Collections.shuffle(playlist);
			else if (!Util.endsWithIgnoreCase(uri.toString(), ".m3u"))
				Collections.sort(playlist);
		}
		catch (IOException ex) {
			// playlist is not essential
		}
	}

	private int getPlaylistIndex()
	{
		return playlist.indexOf(uri);
	}

	private boolean loadFromPlaylist(int playlistIndex, int song)
	{
		return load(playlist.get(playlistIndex), song);
	}


	// Playback -----------------------------------------------------------------------------------------------

	private Uri uri;
	int song;
	private static final int SONG_DEFAULT = -1;
	private static final int SONG_LAST = -2;
	private final ASAP asap = new ASAP();
	ASAPInfo info;
	private Thread thread;
	private boolean paused;
	private int seekPosition;

	private void stop()
	{
		Thread t;
		synchronized (this) {
			t = thread;
			if (t == null || t == Thread.currentThread())
				return;
			thread = null;
			start();
		}
		try {
			t.join();
		}
		catch (InterruptedException ex) {
		}
	}

	private boolean load(Uri uri, int song)
	{
		// read file
		if (!"file".equals(uri.getScheme())) {
			showError(R.string.error_reading_file);
			return false;
		}
		String filename = uri.getPath();
		byte[] module = new byte[ASAPInfo.MAX_MODULE_LENGTH];
		int moduleLen;
		try {
			if (Util.endsWithIgnoreCase(filename, ".zip")) {
				String zipFilename = filename;
				filename = uri.getFragment();
				moduleLen = Util.readAndClose(new ZipInputStream(zipFilename, filename), module);
			}
			else if (Util.endsWithIgnoreCase(filename, ".atr")) {
				AATRStream stream = new AATRStream(filename);
				filename = uri.getFragment();
				try {
					moduleLen = stream.open().readFile(filename, module, module.length);
				}
				finally {
					stream.close();
				}
				if (moduleLen < 0)
					throw new FileNotFoundException(filename);
			}
			else {
				moduleLen = Util.readAndClose(new FileInputStream(filename), module);
			}
		}
		catch (IOException ex) {
			showError(R.string.error_reading_file);
			return false;
		}

		// stop current playback
		stop();

		// No need to worry about synchronization, because at this point we are either:
		// a. on the UI thread with no playback thread
		// b. on the playback thread switching to the next song

		// load into ASAP
		try {
			asap.load(filename, module, moduleLen);
			info = asap.getInfo();
			switch (song) {
			case SONG_DEFAULT:
				song = info.getDefaultSong();
				break;
			case SONG_LAST:
				song = info.getSongs() - 1;
				break;
			default:
				break;
			}
			asap.playSong(song, info.getDuration(song));
		}
		catch (Exception ex) {
			showError(R.string.invalid_file);
			return false;
		}

		// start playback
		this.uri = uri;
		this.song = song;
		paused = false;
		seekPosition = -1;
		showInfo();
		if (thread == null) {
			// we're on the UI thread - start the playback thread
			thread = new Thread(this);
			thread.start();
		}
		return true;
	}

	boolean playNextSong()
	{
		if (song + 1 < info.getSongs())
			return load(uri, song + 1);
		int playlistIndex = getPlaylistIndex();
		if (playlistIndex < 0)
			return false;
		if (++playlistIndex >= playlist.size())
			playlistIndex = 0;
		return loadFromPlaylist(playlistIndex, 0);
	}

	boolean playPreviousSong()
	{
		if (song > 0)
			return load(uri, song - 1);
		int playlistIndex = getPlaylistIndex();
		if (playlistIndex < 0)
			return false;
		if (playlistIndex == 0)
			playlistIndex = playlist.size();
		return loadFromPlaylist(playlistIndex - 1, SONG_LAST);
	}

	public void run()
	{
		do {
			showNotification();

			int channelConfig = info.getChannels() == 1 ? AudioFormat.CHANNEL_CONFIGURATION_MONO : AudioFormat.CHANNEL_CONFIGURATION_STEREO;
			int bufferLen = AudioTrack.getMinBufferSize(ASAP.SAMPLE_RATE, channelConfig, AudioFormat.ENCODING_PCM_16BIT) >> 1;
			if (bufferLen < 16384)
				bufferLen = 16384;
			byte[] byteBuffer = new byte[bufferLen << 1];
			short[] shortBuffer = new short[bufferLen];
			AudioTrack audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, ASAP.SAMPLE_RATE, channelConfig, AudioFormat.ENCODING_PCM_16BIT, bufferLen << 1, AudioTrack.MODE_STREAM);
			audioTrack.play();

			do {
				synchronized (this) {
					if (paused) {
						audioTrack.pause();
						try {
							wait();
						}
						catch (InterruptedException ex) {
						}
						audioTrack.play();
					}

					if (thread == null) {
						audioTrack.stop();
						stopForegroundCompat(NOTIFICATION_ID);
						return;
					}

					int pos = seekPosition;
					if (pos >= 0) {
						seekPosition = -1;
						try {
							asap.seek(pos);
						}
						catch (Exception ex) {
						}
					}
				}

				bufferLen = asap.generate(byteBuffer, byteBuffer.length, ASAPSampleFormat.S16_L_E) >> 1;
				for (int i = 0; i < bufferLen; i++)
					shortBuffer[i] = (short) ((byteBuffer[i << 1] & 0xff) | byteBuffer[i << 1 | 1] << 8);
				audioTrack.write(shortBuffer, 0, bufferLen);
			} while (bufferLen == shortBuffer.length);

			audioTrack.stop();
		} while (playNextSong());

		stopForegroundCompat(NOTIFICATION_ID);
		thread = null;
	}

	private boolean isPaused()
	{
		return paused;
	}

	public boolean isPlaying()
	{
		return !paused;
	}

	public boolean canPause()
	{
		return true;
	}

	public boolean canSeekBackward()
	{
		return false;
	}

	public boolean canSeekForward()
	{
		return false;
	}

	public int getBufferPercentage()
	{
		return 100;
	}

	public synchronized void pause()
	{
		paused = true;
	}

	public synchronized void start()
	{
		paused = false;
		notify();
	}

	synchronized void togglePause()
	{
		if (paused)
			start();
		else
			pause();
	}

	public int getDuration()
	{
		if (song < 0)
			return -1;
		return info.getDuration(song);
	}

	public int getCurrentPosition()
	{
		return asap.getPosition();
	}

	public synchronized void seekTo(int pos)
	{
		seekPosition = pos;
		start();
	}

	public int getAudioSessionId()
	{
		// API 9: return audioTrack != null ? audioTrack.getAudioSessionId() : 0;
		return 0;
	}

	private final BroadcastReceiver headsetReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent)
		{
			if (intent.getIntExtra("state", -1) == 0) {
				pause();
				showInfo(); // just to update the MediaController
			}
		}
	};

	private void registerMediaButtonEventReceiver(String methodName)
	{
		Object audioManager = getSystemService(AUDIO_SERVICE);
		ComponentName eventReceiver = new ComponentName(getPackageName(), MediaButtonEventReceiver.class.getName());
		Util.invokeMethod(audioManager, methodName, eventReceiver);
	}

	@Override
	public void onStart(Intent intent, int startId)
	{
		super.onStart(intent, startId);

		registerReceiver(headsetReceiver, new IntentFilter(Intent.ACTION_HEADSET_PLUG));
		registerMediaButtonEventReceiver("registerMediaButtonEventReceiver");

		TelephonyManager telephony = (TelephonyManager) getSystemService(TELEPHONY_SERVICE);
		telephony.listen(new PhoneStateListener() {
				public void onCallStateChanged(int state, String incomingNumber) {
					if (state == TelephonyManager.CALL_STATE_RINGING)
						pause();
				}
			}, PhoneStateListener.LISTEN_CALL_STATE);

		Uri uri = intent.getData();
		String playlistUri = intent.getStringExtra(EXTRA_PLAYLIST);
		if (playlistUri != null)
			setPlaylist(Uri.parse(playlistUri), false);
		else if ("file".equals(uri.getScheme())) {
			if (ASAPInfo.isOurFile(uri.toString()))
				setPlaylist(Util.getParent(uri), false);
			else {
				setPlaylist(uri, true);
				uri = playlist.get(0);
			}
		}
		load(uri, SONG_DEFAULT);
	}

	@Override
	public void onDestroy()
	{
		super.onDestroy();
		stop();
		registerMediaButtonEventReceiver("unregisterMediaButtonEventReceiver");
		unregisterReceiver(headsetReceiver);
	}


	// Player.java interface ----------------------------------------------------------------------------------

	class LocalBinder extends Binder
	{
		PlayerService getService()
		{
			return PlayerService.this;
		}
	}

	private final IBinder binder = new LocalBinder();

	@Override
	public IBinder onBind(Intent intent)
	{
		return binder;
	}
}
