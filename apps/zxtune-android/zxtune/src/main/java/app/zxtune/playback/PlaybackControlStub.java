/**
 *
 * @file
 *
 * @brief Stub singleton implementation of PlaybackControl
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

public class PlaybackControlStub implements PlaybackControl {

  private PlaybackControlStub() {
  }

  @Override
  public void play() {
  }

  @Override
  public void stop() {
  }

  @Override
  public boolean isPlaying() {
    return false;
  }
  
  @Override
  public void prev() {
  }
  
  @Override
  public void next() {
  }

  @Override
  public TrackMode getTrackMode() {
    return TrackMode.REGULAR;
  }
  
  @Override
  public void setTrackMode(TrackMode mode) {
  }
  
  public SequenceMode getSequenceMode() {
    return SequenceMode.ORDERED;
  }
  
  public void setSequenceMode(SequenceMode mode) {
  }
  
  public static PlaybackControl instance() {
    return Holder.INSTANCE;
  }
  
  private static class Holder {
    public static final PlaybackControl INSTANCE = new PlaybackControlStub();
  }
}
