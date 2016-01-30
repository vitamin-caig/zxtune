/**
 *
 * @file
 *
 * @brief Seek controller interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import app.zxtune.TimeStamp;

public interface SeekControl {
  
  public TimeStamp getDuration();

  public TimeStamp getPosition();

  public void setPosition(TimeStamp position);
}
