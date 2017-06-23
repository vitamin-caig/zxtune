/**
 * @file
 * @brief Implementation of VfsRoot over http://hvsc.c64.org collection
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

/**
 * Paths:
 * <p/>
 * 1) hvsc:/C64Music/${FilePath} - direct access to files or folders
 */

import android.content.Context;
import android.net.Uri;
import android.support.annotation.Nullable;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

import app.zxtune.R;
import app.zxtune.fs.hvsc.Catalog;

final class VfsRootHvsc extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootHvsc.class.getName();

  private static final String SCHEME = "hvsc";

  private static final int POS_CATEGORY = 0;

  private final Context context;
  private final Catalog catalog;
  private final GroupsDir groups[];

  VfsRootHvsc(Context context, HttpProvider http, VfsCache cache) {
    this.context = context;
    this.catalog = Catalog.create(http, cache);
    this.groups = new GroupsDir[]{
            new C64MusicRootDir()
    };
  }

  @Override
  public Uri getUri() {
    return rootUri().build();
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_hvsc_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_hvsc_root_description);
  }

  @Override
  @Nullable
  public VfsObject getParent() {
    return null;
  }

  @Override
  @Nullable
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_hvsc;
    } else {
      return super.getExtension(id);
    }
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (GroupsDir group : groups) {
      visitor.onDir(group);
    }
  }

  @Override
  @Nullable
  public VfsObject resolve(Uri uri) throws IOException {
    if (SCHEME.equals(uri.getScheme())) {
      return resolvePath(uri);
    }
    return null;
  }

  private static Uri.Builder rootUri() {
    return new Uri.Builder().scheme(SCHEME);
  }

  @Nullable
  private VfsObject resolvePath(Uri uri) throws IOException {
    final List<String> path = uri.getPathSegments();
    if (path.isEmpty()) {
      return this;
    } else {
      final String category = path.get(POS_CATEGORY);
      for (GroupsDir group : groups) {
        if (category.equals(group.getPath())) {
          return group.resolve(path);
        }
      }
      return null;
    }
  }

  private abstract class GroupsDir extends StubObject implements VfsDir {
    abstract String getPath();

    @Nullable
    abstract VfsObject resolve(List<String> path) throws IOException;
  }

  private class C64MusicRootDir extends GroupsDir {

    private final String[] SUBDIRS = {"DEMOS", "GAMES", "MUSICIANS"};
    private static final int POS_SUBDIR = POS_CATEGORY + 1;

    @Override
    public Uri getUri() {
      return pathsUri().build();
    }

    @Override
    public String getName() {
      return "C64Music";
    }

    @Override
    public VfsObject getParent() {
      return VfsRootHvsc.this;
    }

    @Override
    public void enumerate(Visitor visitor) throws IOException {
      for (String dir : SUBDIRS) {
        visitor.onDir(new C64MusicSubdir(dir));
      }
    }

    @Override
    public String getPath() {
      return getName();
    }

    @Override
    @Nullable
    public VfsObject resolve(List<String> path) throws IOException {
      final int lastPathComponent = path.size() - 1;
      if (POS_CATEGORY == lastPathComponent) {
        return this;
      } else {
        return new C64MusicSubdir(path.get(POS_SUBDIR)).resolve(path);
      }
    }

    final Uri.Builder pathsUri() {
      return rootUri().appendPath(getPath());
    }

    private abstract class C64MusicObject extends StubObject implements VfsObject {

      final List<String> path;

      C64MusicObject(String path) {
        this.path = new ArrayList<>();
        this.path.add(path);
      }

      C64MusicObject(List<String> path) {
        this.path = path;
      }

      @Override
      public Uri getUri() {
        final Uri.Builder builder = pathsUri();
        for (String dir : path) {
          builder.appendPath(dir);
        }
        return builder.build();
      }

      @Override
      public String getName() {
        return path.get(path.size() - 1);
      }
    }

    //starting from C64Music
    private class C64MusicSubdir extends C64MusicObject implements VfsDir {

      private ByteBuffer content;

      C64MusicSubdir(String path) {
        super(path);
      }

      C64MusicSubdir(List<String> path) {
        super(path);
      }

      C64MusicSubdir(List<String> path, ByteBuffer content) {
        super(path);
        this.content = content;
      }

      @Override
      public VfsObject getParent() {
        final int depth = path.size();
        return 1 == depth
                ? C64MusicRootDir.this//root folder
                : new C64MusicSubdir(path.subList(0, depth - 1));
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

      final VfsObject resolve(List<String> path) throws IOException {
        final int lastPathComponent = path.size() - 1;
        if (POS_SUBDIR == lastPathComponent) {
          return this;
        } else {
          final List<String> subpath = path.subList(POS_SUBDIR, path.size());
          final ByteBuffer content = catalog.getFileContent(subpath);
          return catalog.isDirContent(content)
                  ? new C64MusicSubdir(subpath, content)
                  : new C64MusicFile(subpath, content);
        }
      }

      private ByteBuffer getContent() throws IOException {
        if (content == null) {
          content = catalog.getFileContent(path);
        }
        return content;
      }

      private int getApproxItemsCount(int size) {
        /*
         *  MUSICIANS/Q with 1 entry is 1157 bytes
         *  MUSICIANS/M with 117 entries is 24074 bytes
         *  ~200 bytes per entry, ~520 bytes constant part
         */
        final int ENTRY_SIZE = 200;
        final int AUX_SIZE = 960;
        return size > AUX_SIZE ? (size - AUX_SIZE) / ENTRY_SIZE : 0;
      }

      private VfsDir makeNestedDir(String name) {
        final ArrayList<String> nestedPath = createNestedPath(name);
        return new C64MusicSubdir(nestedPath);
      }

      private VfsFile makeNestedFile(String name, String size) {
        final ArrayList<String> nestedPath = createNestedPath(name);
        return new C64MusicFile(nestedPath, size);
      }

      private ArrayList<String> createNestedPath(String name) {
        final ArrayList<String> result = new ArrayList<>(path.size() + 1);
        result.addAll(path);
        result.add(name);
        return result;
      }

      private class C64MusicFile extends C64MusicObject implements VfsFile {

        private final String size;
        private ByteBuffer content;

        C64MusicFile(List<String> path, String size) {
          super(path);
          this.size = size;
        }

        C64MusicFile(List<String> path, ByteBuffer content) {
          super(path);
          this.size = Integer.toString(content.limit());
          this.content = content;
        }

        @Override
        public VfsObject getParent() {
          return new C64MusicSubdir(path.subList(0, path.size() - 1));
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
  }
}
