/**
 * @file
 * @brief Implementation of VfsRoot over local files
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.Manifest;
import android.content.Context;
import android.net.Uri;
import android.os.Build;
import android.text.TextUtils;

import androidx.annotation.Nullable;

import java.io.File;
import java.io.IOException;

import app.zxtune.R;
import app.zxtune.ResultActivity;
import app.zxtune.Util;
import app.zxtune.coverart.AlbumArt;
import app.zxtune.fs.local.Document;
import app.zxtune.fs.local.StoragesSource;

final class VfsRootLocal extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootLocal.class.getName();
  private static final String SCHEME = "file";

  private final Context context;
  private final StoragesSource source;

  VfsRootLocal(Context context) {
    this.context = context;
    this.source = StoragesSource.create(context);
  }

  @Override
  public Uri getUri() {
    return buildUri("");
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_local_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_local_root_description);
  }

  @Override
  @Nullable
  public VfsObject getParent() {
    return null;
  }

  @Nullable
  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.ICON.equals(id)) {
      return R.drawable.ic_browser_vfs_local;
    } else if (Build.VERSION.SDK_INT >= 23 && VfsExtensions.PERMISSION_QUERY_INTENT.equals(id)) {
      return ResultActivity.Companion.createPermissionsRequestIntent(context, Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE);
    } else {
      return super.getExtension(id);
    }
  }

  @Override
  public void enumerate(Visitor visitor) {
    source.getStorages((obj, description) -> visitor.onDir(buildDir(obj, description)));
  }

  @Override
  @Nullable
  public VfsObject resolve(Uri uri) {
    if (SCHEME.equals(uri.getScheme())) {
      final String path = uri.getPath();
      if (TextUtils.isEmpty(path)) {
        return this;
      } else {
        return resolvePath(path);
      }
    } else {
      return Document.tryResolve(context, uri);
    }
  }

  private static Uri buildUri(String path) {
    return new Uri.Builder().scheme(SCHEME).path(path).build();
  }

  @Nullable
  private VfsObject buildObject(File obj) {
    if (obj.isDirectory()) {
      return buildDir(obj);
    } else if (obj.isFile()) {
      return buildFile(obj);
    } else {
      return null;
    }
  }

  private VfsDir buildDir(File obj) {
    return new LocalDir(obj);
  }

  private VfsDir buildDir(File obj, String description) {
    return new LocalDir(obj, description);
  }

  private VfsFile buildFile(File obj) {
    return new LocalFile(obj);
  }

  @Nullable
  private VfsObject resolvePath(String path) {
    final File obj = new File(path);
    return buildObject(obj);
  }

  private class LocalObject extends StubObject implements VfsObject {

    final File object;
    final String name;

    LocalObject(File obj, String name) {
      this.object = obj;
      this.name = name;
    }

    @Override
    public Uri getUri() {
      return buildUri(object.getPath());
    }

    @Override
    public String getName() {
      return name;
    }

    @Override
    public VfsObject getParent() {
      final File parent = object.getParentFile();
      return parent != null ? new LocalDir(parent) : VfsRootLocal.this;
    }
  }

  private static String getDirName(File dir) {
    final String name = dir.getName();
    return 0 != name.length() ? name : dir.getAbsolutePath();
  }

  private class LocalDir extends LocalObject implements VfsDir {

    private final String description;

    LocalDir(File dir) {
      super(dir, getDirName(dir));
      this.description = "";
    }

    LocalDir(File dir, String description) {
      super(dir, getDirName(dir));
      this.description = description;
    }

    @Override
    public String getDescription() {
      return description;
    }

    @Override
    public void enumerate(Visitor visitor) throws IOException {
      final File[] files = object.listFiles();
      if (files == null) {
        throw new IOException("Failed to enumerate files at " + object.getAbsolutePath());
      }
      for (File file : files) {
        if (file.isDirectory()) {
          visitor.onDir(buildDir(file));
        } else if (file.isFile()) {
          visitor.onFile(buildFile(file));
        }
      }
    }

    @Nullable
    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.COVER_ART_URI.equals(id)) {
        return AlbumArt.forDir(this);
      } else {
        return super.getExtension(id);
      }
    }
  }

  private class LocalFile extends LocalObject implements VfsFile {

    LocalFile(File file) {
      super(file, file.getName());
    }

    @Override
    public String getSize() {
      return Util.formatSize(object.length());
    }

    @Nullable
    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.FILE.equals(id)) {
        return object;
      } else {
        return super.getExtension(id);
      }
    }
  }
}
