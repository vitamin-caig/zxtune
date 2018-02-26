/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.joshw;

import android.support.annotation.NonNull;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;

import app.zxtune.fs.HttpProvider;
import app.zxtune.fs.cache.CacheDir;

public abstract class Catalog {

  public interface DirVisitor {
    void acceptDir(String name);

    void acceptFile(String name, String size);
  }

  /**
   * Get file content
   * @param path path components
   * @return content
   */
  @NonNull
  public abstract ByteBuffer getFileContent(List<String> path) throws IOException;

  /**
   * Check if buffer contains directory page 
   * @param buffer html page content
   * @return true if so
   */
  public abstract boolean isDirContent(ByteBuffer buffer);

  /**
   *
   * @param data html page content
   * @param visitor result receiver
   */
  public abstract void parseDir(ByteBuffer data, DirVisitor visitor) throws IOException;

  public static Catalog create(HttpProvider http, CacheDir cache) {
    final Catalog remote = new RemoteCatalog(http);
    return new CachingCatalog(remote, cache);
  }
}
