/**
 * @file
 * @brief Implementation of VfsRoot over http://hvsc.c64.org collection
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;
import android.support.annotation.Nullable;

import java.io.IOException;

import app.zxtune.R;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.httpdir.Catalog;
import app.zxtune.fs.httpdir.HttpRootBase;
import app.zxtune.fs.hvsc.Path;

final class VfsRootHvsc extends HttpRootBase implements VfsRoot {

  private static final String[] SUBDIRS = {"DEMOS", "GAMES", "MUSICIANS"};

  private final Context context;

  VfsRootHvsc(Context context, HttpProvider http, CacheDir cache) throws IOException {
    super(Catalog.create(context, http, cache, "hvsc"), Path.create());
    this.context = context;
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
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_hvsc;
    } else {
      return super.getExtension(id);
    }
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (String dir : SUBDIRS) {
      visitor.onDir(makeDir(rootPath.getChild(dir), ""));
    }
  }

  @Override
  @Nullable
  public VfsObject resolve(Uri uri) {
    return resolve(Path.parse(uri));
  }
}
