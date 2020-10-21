package app.zxtune.core;

import app.zxtune.Releaseable;
import app.zxtune.TimeStamp;

/**
 * Module interface
 */
public interface Module extends PropertiesAccessor, AdditionalFiles, Releaseable {

  /**
   * @return Module's duration in milliseconds
   */
  TimeStamp getDuration();

  /**
   * Creates new player object
   *
   * @throws RuntimeException in case of error
   */
  Player createPlayer(int samplerate);
}