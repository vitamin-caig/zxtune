/**
 *
 * @file
 *
 * @brief Stub singleton implementation of PlaybackService
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback.stubs;

import app.zxtune.playback.Callback;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.SeekControl;
import app.zxtune.playback.Visualizer;

public class PlaybackServiceStub implements PlaybackService {

  private PlaybackServiceStub() {
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
    static final PlaybackService INSTANCE = new PlaybackServiceStub();
  }  
}
