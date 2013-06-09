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
      return TimeStamp.createFrom(delegate.getPlaybackPosition(), TimeUnit.MILLISECONDS);
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
  public boolean isPlaying() {
    try {
      return delegate.isPlaying();
    } catch (RemoteException e) {
      return false;
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
  public void play() {
    try {
      delegate.play();
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
  
  @Override
  public void next() {
    try {
      delegate.next();
    } catch (RemoteException e) {
    }
  }
  
  @Override
  public void prev() {
    try {
      delegate.prev();
    } catch (RemoteException e) {
    }
  }
  
  @Override
  public void setPlaybackPosition(TimeStamp pos) {
    try {
      delegate.setPlaybackPosition(pos.convertTo(TimeUnit.MILLISECONDS));
    } catch (RemoteException e) {
    }
  }
}
