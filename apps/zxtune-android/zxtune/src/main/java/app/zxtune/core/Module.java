package app.zxtune.core;

/**
 * Module interface
 */
public interface Module extends PropertiesAccessor, AdditionalFiles {

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