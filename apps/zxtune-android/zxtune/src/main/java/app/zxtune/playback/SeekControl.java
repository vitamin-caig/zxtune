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
  
  public TimeStamp getDuration() throws Exception;

  public TimeStamp getPosition() throws Exception;

  public void setPosition(TimeStamp position) throws Exception;
}
