/*
 * MediaButtonEventReceiver.java - ASAP for Android
 *
 * Copyright (C) 2013  Piotr Fusik
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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.view.KeyEvent;

public class MediaButtonEventReceiver extends BroadcastReceiver
{
	@Override
	public void onReceive(Context context, Intent intent)
	{
		IBinder binder = peekService(context, new Intent(context, PlayerService.class));
		if (binder == null)
			return;
		PlayerService service = ((PlayerService.LocalBinder) binder).getService();

		KeyEvent event = (KeyEvent) intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
		if (event.getAction() != KeyEvent.ACTION_DOWN)
			return;
		switch (event.getKeyCode()) {
		case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
		case KeyEvent.KEYCODE_HEADSETHOOK:
			service.togglePause();
			break;
		case KeyEvent.KEYCODE_MEDIA_PAUSE:
			service.pause();
			break;
		case KeyEvent.KEYCODE_MEDIA_PLAY:
			service.start();
			break;
		case KeyEvent.KEYCODE_MEDIA_NEXT:
			service.playNextSong();
			break;
		case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
			service.playPreviousSong();
			break;
		default:
			break;
		}
	}
}
