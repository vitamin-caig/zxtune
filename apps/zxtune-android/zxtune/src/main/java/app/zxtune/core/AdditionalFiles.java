package app.zxtune.core;

import java.nio.ByteBuffer;

/**
 * Interface to additional files access
 */
public interface AdditionalFiles {
  /**
   * @return null if no additional files required, empty array if all the files are resolved,
   * else list of required filenames relative to current one
   */
  String[] getAdditionalFiles() throws Exception;

  /**
   * @param name additional file name as returned by ListAdditionalFiles
   * @param data additional file content
   */
  void resolveAdditionalFile(String name, ByteBuffer data) throws Exception;
}