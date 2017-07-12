/**
 *
 * @file
 *
 * @brief Player events listener interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.sound;

/**
 * All calls are synchronous and may be performed from background thread
 */
public interface PlayerEventsListener {

  /**
   * Called when playback is started or resumed
   */
  public void onStart();

  /**
   * Called when played stream come to an end
   */
  public void onFinish();

  /**
   * Called when playback stopped (also called after onFinish)
   */
  public void onStop();

  /**
   * Called when playback is paused
   */
  public void onPause();
  
  /**
   * Called on unexpected error occurred
   * @param e Exception happened
   */
  public void onError(Exception e);
}
