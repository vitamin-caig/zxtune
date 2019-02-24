/**
 * @file
 * @brief Playback controller interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playback;

public interface PlaybackControl {

  /**
   * Current state
   */
  enum State {
    STOPPED,
    PLAYING,
  }

  /**
   * Track playback mode
   */
  enum TrackMode {
    /// Play track from start to end
    REGULAR,
    /// Loop track according to internal information
    LOOPED,
  }

  /**
   * Tracks collection playback mode
   */
  enum SequenceMode {
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
  void play();


  /*
   * Stop currently playing item
   */
  void stop();

  /*
   * Toggle play/stop state
   */
  void togglePlayStop();

  /*
   * Play next item in sequence
   */
  void next();

  /*
   * Play previous item in sequence
   */
  void prev();

  /*
   * @return Track playback mode
   */
  TrackMode getTrackMode();

  /*
   * Set track playback mode
   */
  void setTrackMode(TrackMode mode);

  /*
   * @return Sequence playback mode
   */
  SequenceMode getSequenceMode();

  /*
   * Set sequence playback mode
   */
  void setSequenceMode(SequenceMode mode);
}
