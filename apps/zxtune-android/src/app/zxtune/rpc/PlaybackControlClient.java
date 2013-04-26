/*
 * @file
 * @brief Remote stub for Playback.Control
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import java.util.concurrent.TimeUnit;

import android.net.Uri;
import android.os.RemoteException;
import app.zxtune.TimeStamp;
import app.zxtune.playback.Control;
import app.zxtune.playback.Item;
import app.zxtune.playback.Status;

final class PlaybackControlClient implements Control {

  private final IPlaybackControl delegate;

  PlaybackControlClient(IPlaybackControl delegate) {
    this.delegate = delegate;
  }
  
  @Override
  public Item getItem() {
    try {
      return delegate.getItem();
    } catch (RemoteException e) {
      return null;
    }
  }
  
  @Override
  public TimeStamp getPlaybackPosition() {
    try {
      return new TimeStamp(delegate.getPlaybackPosition(), TimeUnit.MILLISECONDS);
    } catch (RemoteException e) {
      return null;
    }
  }
  
  @Override
  public int[] getSpectrumAnalysis() {
    try {
      return delegate.getSpectrumAnalysis();
    } catch (RemoteException e) {
      return null;
    }
  }

  @Override
  public Status getStatus() {
    try {
      return Status.valueOf(delegate.getStatus());
    } catch (RemoteException e) {
      return null;
    }
  }

  @Override
  public void play(Uri item) {
    try {
      delegate.playItem(item);
    } catch (RemoteException e) {
    }
  }

  @Override
  public void playPause() {
    try {
      delegate.playPause();
    } catch (RemoteException e) {
    }
  }

  @Override
  public void stop() {
    try {
      delegate.stop();
    } catch (RemoteException e) {
    }
  }
}
