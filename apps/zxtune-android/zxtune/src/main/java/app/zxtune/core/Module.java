package app.zxtune.core;

import app.zxtune.Releaseable;

/**
 * Module interface
 */
public interface Module extends PropertiesAccessor, AdditionalFiles, Releaseable {

  /**
   * @return Module's duration in milliseconds
   */
  long getDurationInMs();

  /**
   * Creates new player object
   *
   * @throws RuntimeException in case of error
   */
  Player createPlayer();
}