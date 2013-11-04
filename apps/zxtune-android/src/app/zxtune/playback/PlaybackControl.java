/**
 *
 * @file
 *
 * @brief Playback controller interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

public interface PlaybackControl {
  
  /**
   * Track playback mode
   */
  public enum TrackMode {
    /// Play track from start to end
    REGULAR,
    /// Loop track according to internal information
    LOOPED,
  }

  /*
   * Activate currently playing item
   */
  public void play();
  
  /*
   * Stop currently playing item
   */
  public void stop();
  
  /*
   * @return true if something is played now
   */
  public boolean isPlaying();
  
  /*
   * Play next item in sequence
   */
  public void next();
  
  /*
   * Play previous item in sequence
   */
  public void prev();
  
  /*
   * @return Track playback mode
   */
  public TrackMode getTrackMode();
  
  /*
   * Set track playback mode
   */
  public void setTrackMode(TrackMode mode);
}
