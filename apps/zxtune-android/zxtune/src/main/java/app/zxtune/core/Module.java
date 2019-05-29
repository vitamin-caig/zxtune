package app.zxtune.core;

import android.support.annotation.NonNull;
import app.zxtune.Releaseable;

/**
 * Module interface
 */
public interface Module extends PropertiesAccessor, AdditionalFiles, Releaseable {

  /**
   * @return Module's duration in frames
   */
  int getDuration();

  /**
   * Creates new player object
   *
   * @throws Exception in case of error
   */
  @NonNull
  Player createPlayer();
}