/**
 * @file
 * @brief Implementation of VfsRoot over http://hvsc.c64.org collection
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;

import androidx.annotation.Nullable;

import app.zxtune.R;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.fs.httpdir.Catalog;
import app.zxtune.fs.httpdir.HttpRootBase;
import app.zxtune.fs.hvsc.Path;

@Icon(R.drawable.ic_browser_vfs_hvsc)
final class VfsRootHvsc extends HttpRootBase implements VfsRoot {

  private static final String[] SUBDIRS = {"DEMOS/", "GAMES/", "MUSICIANS/"};

  private final Context context;

  VfsRootHvsc(VfsObject parent, Context context, MultisourceHttpProvider http) {
    super(parent, Catalog.create(context, http, "hvsc"), Path.create());
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
