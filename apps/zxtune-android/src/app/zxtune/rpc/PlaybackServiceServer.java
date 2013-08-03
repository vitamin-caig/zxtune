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
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.SeekControl;
import app.zxtune.playback.Visualizer;

public class PlaybackServiceServer extends IRemotePlaybackService.Stub {

  private final PlaybackService delegate;
  private final PlaybackControl control;
  private final SeekControl seek;
  private final Visualizer visualizer;

  public PlaybackServiceServer(PlaybackService delegate) {
    this.delegate = delegate;
    this.control = delegate.getPlaybackControl();
    this.seek = delegate.getSeekControl();
    this.visualizer = delegate.getVisualizer();
  }

  @Override
  public ParcelablePlaybackItem getNowPlaying() {
    return ParcelablePlaybackItem.create(delegate.getNowPlaying());
  }
  
  @Override
  public void setNowPlaying(Uri uri) {
    delegate.setNowPlaying(uri);
  }

  @Override
  public void play() {
    control.play();
  }
  
  @Override
  public void stop() {
    control.stop();
  }
  
  @Override
  public boolean isPlaying() {
    return control.isPlaying();
  }
    
  @Override
  public void next() {
    control.next();
  }
  
  @Override
  public void prev() {
    control.prev();
  }
  
  @Override
  public long getDuration() {
    return seek.getDuration().convertTo(TimeUnit.MILLISECONDS);
  }
  
  @Override
  public long getPosition() {
    return seek.getPosition().convertTo(TimeUnit.MILLISECONDS);
  }
  
  @Override
  public void setPosition(long ms) {
    seek.setPosition(TimeStamp.createFrom(ms, TimeUnit.MILLISECONDS));
  }
  
  @Override
  public int[] getSpectrum() {
    final int[] bands = new int[16];
    final int[] levels = new int[16];
    final int chans = visualizer.getSpectrum(bands, levels);
    final int[] res = new int[chans];
    for (int idx = 0; idx != chans; ++idx) {
      res[idx] = (levels[idx] << 8) | bands[idx];
    }
    return res;
  }
}
