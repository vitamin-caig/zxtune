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
   * Current state
   */
  public enum State {
    STOPPED,
    PLAYING,
    PAUSED
  }

  /**
   * Track playback mode
   */
  public enum TrackMode {
    /// Play track from start to end
    REGULAR,
    /// Loop track according to internal information
    LOOPED,
  }
  
  /**
   * Tracks collection playback mode
   */
  public enum SequenceMode {
    /// Play collection from start to end
    ORDERED,
    /// Loop collection (if possible)
    LOOPED,
    /// Random position
    SHUFFLE,
  }

  /*
   * Activate currently playing item
   */
  public void play();

  /*
   * Pause currently playing item
   */
  public void pause();
  
  /*
   * Stop currently playing item
   */
  public void stop();
  
  /*
   * @return current state
   */
  public State getState();
  
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
  
  /*
   * @return Sequence playback mode
   */
  public SequenceMode getSequenceMode();
  
  /*
   * Set sequence playback mode
   */
  public void setSequenceMode(SequenceMode mode);
}
