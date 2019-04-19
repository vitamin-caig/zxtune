package app.zxtune.core;

import android.support.annotation.NonNull;

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
  @NonNull
  Player createPlayer() throws Exception;
}