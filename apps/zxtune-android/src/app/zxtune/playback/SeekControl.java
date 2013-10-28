/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import app.zxtune.TimeStamp;

public interface SeekControl {
  
  public TimeStamp getDuration();

  public TimeStamp getPosition();

  public void setPosition(TimeStamp position);
}
