package app.zxtune.core;

import app.zxtune.Releaseable;

/**
 * Player interface
 */
public interface Player extends Releaseable, PropertiesAccessor, PropertiesModifier {

  /**
   * @return Index of next rendered frame
   */
  int getPosition() throws Exception;

  /**
   * @param bands Array of bands to store
   * @param levels Array of levels to store
   * @return Count of actually stored entries
   */
  int analyze(int bands[], int levels[]) throws Exception;

  /**
   * Render next result.length bytes of sound data
   * @param result Buffer to put data
   * @return Is there more data to render
   */
  boolean render(short[] result) throws Exception;

  /**
   * @param pos Index of next rendered frame
   */
  void setPosition(int pos) throws Exception;

  /**
   * @return rendering performance in percents
   */
  int getPlaybackPerformance() throws Exception;
}
