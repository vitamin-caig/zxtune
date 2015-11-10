/**
 *
 * @file
 *
 * @brief Catalog interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.hvsc;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;

import android.content.Context;
import app.zxtune.fs.HttpProvider;
import app.zxtune.fs.VfsCache;

public abstract class Catalog {
  
  public interface DirVisitor {
    void acceptDir(String name);
    void acceptFile(String name, String size);
  }

  /**
   * Get file content
   * @param path path to module starting from C64Music/..
   * @return content
   * @throws IOException
   */
  public abstract ByteBuffer getFileContent(List<String> path) throws IOException;
  
  /**
   * Check if buffer contains directory page 
   * @param buffer
   * @return true if so
   */
  public abstract boolean isDirContent(ByteBuffer buffer);

  /**
   * 
   * @param data
   * @param visitor
   * @throws IOException
   */
  public abstract void parseDir(ByteBuffer data, DirVisitor visitor) throws IOException;
  
  public static Catalog create(Context context, HttpProvider http) {
    final Catalog remote = new RemoteCatalog(http);
    final VfsCache cacheDir = VfsCache.create(context, "hvsc");
    return new CachingCatalog(remote, cacheDir);
  }
}
