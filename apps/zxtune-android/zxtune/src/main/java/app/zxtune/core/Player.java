package app.zxtune.core;

/**
 * Player interface
 */
public interface Player extends PropertiesAccessor, PropertiesModifier {

  /**
   * @return Index of next rendered frame
   */
  int getPosition() throws Exception;

  /**
   * @param levels Array of levels to store
   * @return Count of actually stored entries
   */
  int analyze(byte levels[]) throws Exception;

  /**
   * Render next result.length bytes of sound data
   *
   * @param result Buffer to put data
   * @return Is there more data to render
   */
  boolean render(short[] result) throws Exception;

  /**
   * @param pos Index of next rendered frame
   */
  void setPosition(int pos) throws Exception;
}
