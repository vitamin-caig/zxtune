package app.zxtune.core;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.nio.ByteBuffer;

/**
 * Interface to additional files access
 */
public interface AdditionalFiles {
  /**
   * @return null if no additional files required, empty array if all the files are resolved,
   * else list of required filenames relative to current one
   */
  @Nullable
  String[] getAdditionalFiles() throws Exception;

  /**
   * @param name additional file name as returned by ListAdditionalFiles
   * @param data additional file content
   */
  void resolveAdditionalFile(@NonNull String name, @NonNull ByteBuffer data) throws Exception;
}