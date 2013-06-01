/**
 * @file
 * @brief Sound player interface
 * @version $Id:$
 * @author
 */
package app.zxtune.sound;

import app.zxtune.TimeStamp;

/**
 * Base interface for low-level sound player
 */
public interface Player {

  public void startPlayback();

  public void stopPlayback();
  
  public void setPosition(TimeStamp position);
  
  public boolean isStarted();
  
  public TimeStamp getPosition();

  public void release();
}
