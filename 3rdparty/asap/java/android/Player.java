/*
 * Player.java - ASAP for Android
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

package net.sf.asap;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.net.Uri;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.MediaController;
import android.widget.TextView;

public class Player extends Activity
{
	private PlayerService service;
	private final ServiceConnection connection = new ServiceConnection() {
			public void onServiceConnected(ComponentName className, IBinder service)
			{
				Player.this.service = ((PlayerService.LocalBinder) service).getService();
				showInfo();
			}
			public void onServiceDisconnected(ComponentName className)
			{
				Player.this.service = null;
			}
		};

	private MediaController mediaController;

	private View getContentView()
	{
		return findViewById(android.R.id.content);
	}

	private void setTag(int controlId, String value)
	{
		TextView control = (TextView) findViewById(controlId);
		if (value.length() == 0)
			control.setVisibility(View.GONE);
		else {
			control.setText(value);
			control.setVisibility(View.VISIBLE);
		}
	}

	private void showInfo()
	{
		final PlayerService service = this.service;
		if (service == null)
			return;
		ASAPInfo info = service.info;
		if (info == null)
			return;
		setTag(R.id.name, info.getTitleOrFilename());
		setTag(R.id.author, info.getAuthor());
		setTag(R.id.date, info.getDate());
		int songs = info.getSongs();
		if (songs > 1)
			setTag(R.id.song, getString(R.string.song_format, service.song + 1, songs));
		else
			setTag(R.id.song, "");

		mediaController.setMediaPlayer(service);
		mediaController.setPrevNextListeners(new OnClickListener() {
				public void onClick(View v) { service.playNextSong(); }
			},
			new OnClickListener() {
				public void onClick(View v) { service.playPreviousSong(); }
			});
	}

	static final String ACTION_SHOW_INFO = "net.sf.asap.action.SHOW_INFO";

	private BroadcastReceiver receiver = new BroadcastReceiver() {
			@Override
			public void onReceive(Context context, Intent intent) {
				showInfo();
			}
		};

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setTitle(R.string.playing_title);
		setContentView(R.layout.playing);
		mediaController = new MediaController(this, false);
		mediaController.setAnchorView(getContentView());

		Uri uri = getIntent().getData();
		Intent intent = new Intent(Intent.ACTION_VIEW, uri, this, PlayerService.class);
		if (uri != null)
			startService(intent);
		bindService(intent, connection, Context.BIND_AUTO_CREATE);

		findViewById(R.id.stop_button).setOnClickListener(new OnClickListener() {
				public void onClick(View v) {
					if (service != null)
						service.stopSelf();
					finish();
				}
			});

		registerReceiver(receiver, new IntentFilter(ACTION_SHOW_INFO));
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		if (service != null)
			mediaController.show(0);
	}

	@Override
	public boolean onTouchEvent(MotionEvent event)
	{
		if (service != null)
			mediaController.show(0);
		return true;
	}

	@Override
	protected void onDestroy()
	{
		super.onDestroy();
		unregisterReceiver(receiver);
		unbindService(connection);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		getMenuInflater().inflate(R.menu.playing, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch (item.getItemId()) {
		case R.id.menu_about:
			Util.showAbout(this);
			return true;
		default:
			return false;
		}
	}
}
