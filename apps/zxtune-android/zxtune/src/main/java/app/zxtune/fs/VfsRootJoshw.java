/**
 * @file
 * @brief Implementation of VfsRoot over http://hvsc.c64.org collection
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

/**
 * Paths:
 * <p/>
 * 1) joshw:/${Catalogue}/${FilePath} - direct access to files or folders
 */

import android.content.Context;
import android.net.Uri;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.text.format.Formatter;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.TreeSet;

import app.zxtune.R;
import app.zxtune.fs.joshw.Catalog;

final class VfsRootJoshw extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootJoshw.class.getName();

  private static final String SCHEME = "joshw";

  private static final int POS_BASE_ID = 0;

  private final Context context;
  private final Catalog catalog;
  private final AudiobaseDir bases[];

  VfsRootJoshw(Context context, HttpProvider http, VfsCache cache) {
    this.context = context;
    this.catalog = Catalog.create(http, cache);
    this.bases = new AudiobaseDir[]{
            new AudiobaseDir("gbs", R.string.vfs_joshw_gbs_name, R.string.vfs_joshw_gbs_description),
            new AudiobaseDir("hes", R.string.vfs_joshw_hes_name, R.string.vfs_joshw_hes_description),
            new AudiobaseDir("nsf", R.string.vfs_joshw_nsf_name, R.string.vfs_joshw_nsf_description),
            new AudiobaseDir("smd", R.string.vfs_joshw_smd_name, R.string.vfs_joshw_smd_description),
            new AudiobaseDir("spc", R.string.vfs_joshw_spc_name, R.string.vfs_joshw_spc_description),
            new AudiobaseDir("kss", R.string.vfs_joshw_kss_name, R.string.vfs_joshw_kss_description),
            new AudiobaseDir("psf", R.string.vfs_joshw_psf_name, R.string.vfs_joshw_psf_description),
            new AudiobaseDir("usf", R.string.vfs_joshw_usf_name, R.string.vfs_joshw_usf_description),
    };
  }

  @Override
  public Uri getUri() {
    return rootUri().build();
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_joshw_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_joshw_root_description);
  }

  @Override
  @Nullable
  public VfsObject getParent() {
    return null;
  }

  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_joshw;
    } else {
      return super.getExtension(id);
    }
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (AudiobaseDir base : bases) {
      visitor.onDir(base);
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
      final String id = path.get(POS_BASE_ID);
      for (AudiobaseDir dir : bases) {
        if (id.equals(dir.getBaseId())) {
          return dir.resolve(path);
        }
      }
      return null;
    }
  }

  private class AudiobaseDir extends StubObject implements VfsDir {

    private final String id;
    private final int name;
    private final int description;

    AudiobaseDir(String id, int nameRes, int descRes) {
      this.id = id;
      this.name = nameRes;
      this.description = descRes;
    }

    @Override
    public Uri getUri() {
      return rootUri().appendPath(getBaseId()).build();
    }

    @Override
    public String getName() {
      return context.getString(name);
    }

    @Override
    public String getDescription() {
      return context.getString(description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootJoshw.this;
    }

    @Override
    public void enumerate(Visitor visitor) throws IOException {
      new MusicSubdir(id).enumerate(visitor);
    }

    public String getBaseId() {
      return id;
    }

    public VfsObject resolve(List<String> path) throws IOException {
      final int lastPathComponent = path.size() - 1;
      if (POS_BASE_ID == lastPathComponent) {
        // joshw://xxx
        return this;
      } else {
        // joshw://xxx/yyy
        final ByteBuffer content = catalog.getFileContent(path);
        return catalog.isDirContent(content)
                ? new MusicSubdir(path, content)
                : new MusicFile(path, content);
      }
    }

    private abstract class MusicObject extends StubObject implements VfsObject {

      final List<String> path;
      ByteBuffer content;

      MusicObject(String path) {
        this.path = new ArrayList<String>();
        this.path.add(path);
      }

      MusicObject(List<String> path) {
        this.path = path;
      }

      MusicObject(List<String> path, ByteBuffer content) {
        this.path = path;
        this.content = content;
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
      public VfsObject getParent() {
        final int parentDepth = path.size() - 1;
        return parentDepth == 1
                ? AudiobaseDir.this//root folder
                : new MusicSubdir(path.subList(0, parentDepth));
      }

      ByteBuffer getContent() throws IOException {
        if (content == null) {
          content = catalog.getFileContent(path);
        }
        return content;
      }
    }

    private class MusicSubdir extends MusicObject implements VfsDir {

      MusicSubdir(String path) {
        super(path);
      }

      MusicSubdir(List<String> path) {
        super(path);
      }

      MusicSubdir(List<String> path, ByteBuffer content) {
        super(path, content);
      }

      @Override
      public String getName() {
        return Uri.decode(path.get(path.size() - 1));
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

      private int getApproxItemsCount(int size) {
        /*
         *  at gbs.joshw.info
         *    0-9 with 6 entries is 2777 bytes
         *    s with 168 entries is 55251 bytes
         *  ~324 bytes per entry, ~835 bytes constant part
         */
        final int ENTRY_SIZE = 324;
        final int AUX_SIZE = 835;
        return size > AUX_SIZE ? (size - AUX_SIZE) / ENTRY_SIZE : 0;
      }

      private VfsDir makeNestedDir(String name) {
        final ArrayList<String> nestedPath = createNestedPath(name);
        return new MusicSubdir(nestedPath);
      }

      private VfsFile makeNestedFile(String name, String size) {
        final ArrayList<String> nestedPath = createNestedPath(name);
        return new MusicFile(nestedPath, size);
      }

      private ArrayList<String> createNestedPath(String name) {
        final ArrayList<String> result = new ArrayList<String>(path.size() + 1);
        result.addAll(path);
        result.add(name);
        return result;
      }
    }

    private class MusicFile extends MusicObject implements VfsFile {

      //${title} (\[${alt title}\])?(\(${tag}\))*
      private String name;
      private String description;
      private final String size;

      MusicFile(List<String> path, String size) {
        super(path);
        loadMetainfo();
        this.size = size;
      }

      MusicFile(List<String> path, ByteBuffer content) {
        super(path, content);
        loadMetainfo();
        this.size = Formatter.formatShortFileSize(context, content.limit());
      }

      @Override
      public String getName() {
        return name;
      }

      @Override
      public String getDescription() {
        return description;
      }

      @Override
      public String getSize() {
        return size;
      }

      @Override
      public ByteBuffer getContent() throws IOException {
        return super.getContent();
      }

      private void loadMetainfo() {
        final String filename = Uri.decode(path.get(path.size() - 1));
        int endOfName = filename.indexOf('(');
        if (endOfName == -1) {
          endOfName = filename.lastIndexOf('.');
        }
        name = filename.substring(0, endOfName - 1).trim();
        final TreeSet<String> meta = new TreeSet<String>();
        for (int prevMetaPos = endOfName; ; ) {
          final int metaPos = filename.indexOf('(', prevMetaPos);
          if (metaPos == -1) {
            break;
          }
          final int metaPosEnd = filename.indexOf(')', metaPos);
          if (metaPosEnd == -1) {
            break;
          }
          final String field = filename.substring(metaPos + 1, metaPosEnd);
          meta.add(field);
          prevMetaPos = metaPosEnd;
        }
        description = TextUtils.join(" - ", meta);
      }
    }
  }
}
