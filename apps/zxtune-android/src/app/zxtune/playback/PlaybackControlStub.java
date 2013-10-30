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

/**
 * 
 */
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
  public boolean isLooped() {
    return false;
  }
  
  public void setLooped(boolean looped) {
  }
  
  public static PlaybackControl instance() {
    return Holder.INSTANCE;
  }
  
  private static class Holder {
    public final static PlaybackControl INSTANCE = new PlaybackControlStub();
  }
}
