/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.httpdir;

import android.content.Context;
import android.net.Uri;
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
   * @param path resource location
   * @return content
   */
  @NonNull
  public abstract ByteBuffer getFileContent(Path path) throws IOException;

  /**
   *
   * @param path resource location
   * @param visitor result receiver
   */
  public abstract void parseDir(Path path, DirVisitor visitor) throws IOException;

  public static Catalog create(Context ctx, HttpProvider http, CacheDir cache, String id) throws IOException {
    final RemoteCatalog remote = new RemoteCatalog(http);
    return new CachingCatalog(ctx, remote, cache, id);
  }
}
