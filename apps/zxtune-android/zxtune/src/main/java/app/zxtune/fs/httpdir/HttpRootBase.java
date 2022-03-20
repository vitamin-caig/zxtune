/**
 * @file
 * @brief Base implementation of all http-based catalogues
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.httpdir;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.io.IOException;

import app.zxtune.fs.StubObject;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

public abstract class HttpRootBase extends StubObject implements VfsDir {

  private final VfsObject parent;
  private final Catalog catalog;
  protected final Path rootPath;

  protected HttpRootBase(VfsObject parent, Catalog catalog, Path path) {
    this.parent = parent;
    this.catalog = catalog;
    this.rootPath = path;
  }

  @Override
  public Uri getUri() {
    return rootPath.getUri();
  }

  @Override
  @Nullable
  public VfsObject getParent() {
    return parent;
  }

  @Override
  public void enumerate(Visitor visitor) throws IOException {
    makeDir(rootPath, "").enumerate(visitor);
  }

  @Nullable
  protected VfsObject resolve(@Nullable Path path) {
    if (path == null) {
      return null;
    } else if (path.isEmpty()) {
      return this;
    } else if (path.isFile()) {
      return makeFile(path, "", "");
    } else {
      return makeDir(path, "");
    }
  }

  protected VfsDir makeDir(Path path, String descr) {
    return new HttpDir(path, descr);
  }

  protected VfsFile makeFile(Path path, String descr, String size) {
    return new HttpFile(path, descr, size);
  }

  private abstract class HttpObject extends StubObject {

    protected final Path path;
    private final String descr;

    HttpObject(Path path, String descr) {
      this.path = path;
      this.descr = descr;
    }

    @Override
    public Uri getUri() {
      return path.getUri();
    }

    @Override
    public String getName() {
      return path.getName();
    }

    @Override
    public String getDescription() {
      return descr;
    }

    @Override
    public VfsObject getParent() {
      return resolve(path.getParent());
    }
  }

  protected class HttpDir extends HttpObject implements VfsDir {

    public HttpDir(Path path, String descr) {
      super(path, descr);
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.parseDir(path, new Catalog.DirVisitor() {
        @Override
        public void acceptDir(String name, String descr) {
          if (!name.isEmpty()) {
            visitor.onDir(makeDir(path.getChild(name + "/"), descr));
          }
        }

        @Override
        public void acceptFile(String name, String descr, String size) {
          if (!name.isEmpty()) {
            visitor.onFile(makeFile(path.getChild(name), descr, size));
          }
        }
      });
    }
  }

  protected class HttpFile extends HttpObject implements VfsFile {

    private final String size;

    public HttpFile(Path path, String descr, String size) {
      super(path, descr);
      this.size = size;
    }

    @Override
    public String getSize() {
      return size;
    }

    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.CACHE_PATH.equals(id)) {
        return path.getLocalId();
      } else if (VfsExtensions.DOWNLOAD_URIS.equals(id)) {
        return path.getRemoteUris();
      } else {
        return super.getExtension(id);
      }
    }
  }
}
