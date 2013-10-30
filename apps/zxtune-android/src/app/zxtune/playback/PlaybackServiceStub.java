/**
 *
 * @file
 *
 * @brief Stub singleton implementation of PlaybackService
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import android.net.Uri;

public class PlaybackServiceStub implements PlaybackService {

  private PlaybackServiceStub() {
  }

  @Override
  public Item getNowPlaying() {
    return ItemStub.instance();
  }

  @Override
  public void setNowPlaying(Uri[] uris) {
  }

  @Override
  public PlaylistControl getPlaylistControl() {
    return PlaylistControlStub.instance();
  }

  @Override
  public PlaybackControl getPlaybackControl() {
    return PlaybackControlStub.instance();
  }

  @Override
  public SeekControl getSeekControl() {
    return SeekControlStub.instance();
  }

  @Override
  public Visualizer getVisualizer() {
    return VisualizerStub.instance();
  }

  @Override
  public void subscribe(Callback cb) {
  }

  @Override
  public void unsubscribe(Callback cb) {
  }
  
  public static PlaybackService instance() {
    return Holder.INSTANCE;
  }
  
  //onDemand holder idiom
  private static class Holder {
    public static final PlaybackService INSTANCE = new PlaybackServiceStub();
  }  
}
