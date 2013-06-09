/*
 * @file
 * @brief Remote server stub for Playback.Control
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import java.util.concurrent.TimeUnit;

import android.net.Uri;
import app.zxtune.TimeStamp;
import app.zxtune.playback.Control;

public class PlaybackControlServer extends IPlaybackControl.Stub {

  private final Control delegate;

  public PlaybackControlServer(Control delegate) {
    this.delegate = delegate;
  }

  @Override
  public ParcelablePlaybackItem getItem() {
    return ParcelablePlaybackItem.create(delegate.getItem());
  }

  @Override
  public long getPlaybackPosition() {
    return delegate.getPlaybackPosition().convertTo(TimeUnit.MILLISECONDS);
  }
  
  @Override
  public int[] getSpectrumAnalysis() {
    return delegate.getSpectrumAnalysis();
  }
  
  @Override
  public boolean isPlaying() {
    return delegate.isPlaying();
  }
  
  @Override
  public void playItem(Uri item) {
    delegate.play(item);
  }

  @Override
  public void play() {
    delegate.play();
  }
  
  @Override
  public void stop() {
    delegate.stop();
  }
  
  @Override
  public void next() {
    delegate.next();
  }
  
  @Override
  public void prev() {
    delegate.prev();
  }
  
  @Override
  public void setPlaybackPosition(long ms) {
    delegate.setPlaybackPosition(TimeStamp.createFrom(ms, TimeUnit.MILLISECONDS));
  }
}
