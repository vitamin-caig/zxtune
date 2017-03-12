/**
 *
 * @file
 *
 * @brief Implementation of VfsRoot over http://hvsc.c64.org collection
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

/**
 *
 * Paths:
 *
 * 1) aygor:/${FilePath} - direct access to files or folders starting from http://abrimaal.pro-e.pl/ayon/ dir
 */

import android.content.Context;
import android.net.Uri;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import app.zxtune.R;
import app.zxtune.fs.aygor.Catalog;

final class VfsRootAygor extends StubObject implements VfsRoot {

  private final static String TAG = VfsRootAygor.class.getName();

  private final static String SCHEME = "aygor";

  private final Context context;
  private final Catalog catalog;
  private final AyonMusicSubdir root;

  VfsRootAygor(Context context, HttpProvider http, VfsCache cache) {
    this.context = context;
    this.catalog = Catalog.create(http, cache);
    this.root = new AyonMusicSubdir(Collections.<String> emptyList());
  }

  @Override
  public Uri getUri() {
    return rootUri().build();
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_aygor_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_aygor_root_description);
  }

  @Override
  public VfsObject getParent() {
    return null;
  }

  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_aygor;
    } else {
      return super.getExtension(id);
    }
  }
  
  @Override
  public void enumerate(Visitor visitor) throws IOException {
    root.enumerate(visitor);
  }

  @Override
  public VfsObject resolve(Uri uri) throws IOException {
    if (SCHEME.equals(uri.getScheme())) {
      return resolvePath(uri);
    }
    return null;
  }

  private Uri.Builder rootUri() {
    return new Uri.Builder().scheme(SCHEME);
  }

  private VfsObject resolvePath(Uri uri) throws IOException {
    final List<String> path = uri.getPathSegments();
    if (path.isEmpty()) {
      return this;
    } else {
      return root.resolve(uri, path);
    }
  }
  
  private abstract class AyonMusicObject extends StubObject implements VfsObject {

    protected final List<String> path;
    
    AyonMusicObject(String path) {
      this.path = new ArrayList<String>();
      this.path.add(path);
    }
    
    AyonMusicObject(List<String> path) {
      this.path = path;
    }
    
    @Override
    public Uri getUri() {
      final Uri.Builder builder = rootUri();
      for (String dir : path) {
        builder.appendPath(dir);
      }
      return builder.build();
    }
    
    @Override
    public String getName() {
      return path.get(path.size() - 1);
    }

    @Override
    public VfsObject getParent() {
      final int depth = path.size();
      return 1 == depth
              ? VfsRootAygor.this//root folder
              : new AyonMusicSubdir(path.subList(0, depth - 1));
    }
  }
  
  //starting from ayon
  private class AyonMusicSubdir extends AyonMusicObject implements VfsDir {

    private ByteBuffer content;

    AyonMusicSubdir(String path) {
      super(path);
    }

    AyonMusicSubdir(List<String> path) {
      super(path);
    }

    AyonMusicSubdir(List<String> path, ByteBuffer content) {
      super(path);
      this.content = content;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final ByteBuffer content = getContent();
      visitor.onItemsCount(getApproxItemsCount(content.limit()));
      catalog.parseDir(content, new Catalog.DirVisitor() {
        @Override
        public void acceptDir(String name) {
          visitor.onDir(makeNestedDir(name));
        }

        @Override
        public void acceptFile(String name, String size) {
          visitor.onFile(makeNestedFile(name, size));
        }
      });
    }

    final VfsObject resolve(Uri uri, List<String> path) throws IOException {
      final ByteBuffer content = catalog.getFileContent(path);
      return catalog.isDirContent(content)
              ? new AyonMusicSubdir(path, content)
              : new AyonMusicFile(path, content);
    }

    private ByteBuffer getContent() throws IOException {
      if (content == null) {
        content = catalog.getFileContent(path);
      }
      return content;
    }

    private int getApproxItemsCount(int size) {
      /*
       *  musicians/Q with 1 entry is 888 bytes
       *  musicians/M with 30 entries is 7103 bytes
       *  ~210 bytes per entry, ~680 bytes constant part
       */
      final int ENTRY_SIZE = 210;
      final int AUX_SIZE = 680;
      return size > AUX_SIZE ? (size - AUX_SIZE) / ENTRY_SIZE : 0;
    }

    private VfsDir makeNestedDir(String name) {
      final ArrayList<String> nestedPath = createNestedPath(name);
      return new AyonMusicSubdir(nestedPath);
    }

    private VfsFile makeNestedFile(String name, String size) {
      final ArrayList<String> nestedPath = createNestedPath(name);
      return new AyonMusicFile(nestedPath, size);
    }

    private ArrayList<String> createNestedPath(String name) {
      final ArrayList<String> result = new ArrayList<String>(path.size() + 1);
      result.addAll(path);
      result.add(name);
      return result;
    }
  }
    
  private class AyonMusicFile extends AyonMusicObject implements VfsFile {

    private String size;
    private ByteBuffer content;

    AyonMusicFile(List<String> path, String size) {
      super(path);
      this.size = size;
    }

    AyonMusicFile(List<String> path, ByteBuffer content) {
      super(path);
      this.size = Integer.toString(content.limit());
      this.content = content;
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
