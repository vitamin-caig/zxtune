/**
 * @file
 * @brief Sound player interface
 * @version $Id:$
 * @author
 */
package app.zxtune.sound;

/**
 * Base interface for low-level sound player
 */
public interface Player {

  public void play();

  public void stop();

  public void release();
}
