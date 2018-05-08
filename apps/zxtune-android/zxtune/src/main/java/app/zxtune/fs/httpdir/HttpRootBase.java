/**
 * @file
 * @brief Base implementation of all http-based catalogues
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.httpdir;

import android.net.Uri;
import android.support.annotation.Nullable;

import java.io.IOException;
import java.nio.ByteBuffer;

import app.zxtune.fs.StubObject;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

public abstract class HttpRootBase extends StubObject implements VfsDir {

  private final Catalog catalog;
  protected final Path rootPath;

  protected HttpRootBase(Catalog catalog, Path path) {
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
    return null;
  }

  @Override
  public void enumerate(Visitor visitor) throws IOException {
    makeDir(rootPath).enumerate(visitor);
  }

  @Nullable
  protected VfsObject resolve(@Nullable Path path) {
    if (path == null) {
      return null;
    } else if (path.isEmpty()) {
      return this;
    } else if (path.isFile()) {
      return makeFile(path, "");
    } else {
      return makeDir(path);
    }
  }

  @Nullable
  protected VfsDir makeDir(Path path) {
    return new HttpDir(path);
  }

  @Nullable
  protected VfsFile makeFile(Path path, String size) {
    return new HttpFile(path, size);
  }

  private abstract class HttpObject extends StubObject {

    protected final Path path;

    HttpObject(Path path) {
      this.path = path;
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
    public VfsObject getParent() {
      return resolve(path.getParent());
    }
  }

  public class HttpDir extends HttpObject implements VfsDir {

    public HttpDir(Path path) {
      super(path);
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.parseDir(path, new Catalog.DirVisitor() {
        @Override
        public void acceptDir(String name) {
          visitor.onDir(makeDir(path.getChild(name)));
        }

        @Override
        public void acceptFile(String name, String size) {
          visitor.onFile(makeFile(path.getChild(name), size));
        }
      });
    }
  }

  public class HttpFile extends HttpObject implements VfsFile {

    private final String size;
    private ByteBuffer content;

    public HttpFile(Path path, String size) {
      super(path);
      this.size = size;
    }

    @Override
    public String getSize() {
      return size;
    }

    @Override
    public ByteBuffer getContent() throws IOException {
      if (content == null) {
        content = catalog.getFileContent(path);
      }
      return content;
    }
  }
}
