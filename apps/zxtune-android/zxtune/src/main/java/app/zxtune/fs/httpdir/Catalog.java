/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.httpdir;

import android.content.Context;

import java.io.IOException;

import app.zxtune.fs.http.MultisourceHttpProvider;

public abstract class Catalog {

  public interface DirVisitor {
    void acceptDir(String name, String description);

    void acceptFile(String name, String description, String size);
  }

  /**
   * @param path    resource location
   * @param visitor result receiver
   */
  public abstract void parseDir(Path path, DirVisitor visitor) throws IOException;

  public static CachingCatalog create(Context ctx, MultisourceHttpProvider http, String id) {
    final RemoteCatalog remote = new RemoteCatalog(http);
    return create(ctx, remote, id);
  }

  public static CachingCatalog create(Context ctx, RemoteCatalog remote, String id) {
    return new CachingCatalog(ctx, remote, id);
  }
}
