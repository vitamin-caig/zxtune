package app.zxtune.core;

import app.zxtune.Releaseable;

/**
 * Module interface
 */
public interface Module extends Releaseable, PropertiesAccessor, AdditionalFiles {

  /**
   * @return Module's duration in frames
   */
  int getDuration() throws Exception;

  /**
   * Creates new player object
   *
   * @throws Exception in case of error
   */
  Player createPlayer() throws Exception;
}