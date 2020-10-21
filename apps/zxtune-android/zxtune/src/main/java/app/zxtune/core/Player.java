package app.zxtune.core;

import app.zxtune.Releaseable;
import app.zxtune.TimeStamp;

/**
 * Player interface
 */
public interface Player extends PropertiesModifier, Releaseable {

  /**
   * @return Position
   */
  TimeStamp getPosition();

  /**
   * @param levels Array of levels to store
   * @return Count of actually stored entries
   */
  int analyze(byte[] levels);

  /**
   * Render next result.length bytes of sound data
   *
   * @param result Buffer to put data
   * @return Is there more data to render
   */
  boolean render(short[] result);

  /**
   * @param pos Position
   */
  void setPosition(TimeStamp pos);

  /**
   * @return Rendering performance in percents
   */
  int getPerformance();

  /**
   * @return Playback progress in percents (may be >100!!!)
   */
  int getProgress();
}
