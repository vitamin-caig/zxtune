package app.zxtune.core;

import android.support.annotation.NonNull;
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
   * @throws Exception in case of error
   */
  @NonNull
  Player createPlayer();
}