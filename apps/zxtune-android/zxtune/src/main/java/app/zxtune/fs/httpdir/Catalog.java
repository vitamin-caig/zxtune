/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.httpdir;

import android.content.Context;

import java.io.IOException;

import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.http.MultisourceHttpProvider;

public abstract class Catalog {

  public interface DirVisitor {
    void acceptDir(String name, String description);

    void acceptFile(String name, String description, String size);
  }

  /**
   *
   * @param path resource location
   * @param visitor result receiver
   */
  public abstract void parseDir(Path path, DirVisitor visitor) throws IOException;

  public static CachingCatalog create(Context ctx, MultisourceHttpProvider http, CacheDir cache, String id) {
    final RemoteCatalog remote = new RemoteCatalog(http);
    return create(ctx, remote, cache, id);
  }

  public static CachingCatalog create(Context ctx, RemoteCatalog remote, CacheDir cache,
                                     String id) {
    return new CachingCatalog(ctx, remote, cache, id);
  }
}
