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
import app.zxtune.fs.aygor.Path;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.fs.httpdir.Catalog;
import app.zxtune.fs.httpdir.HttpRootBase;

@Icon(R.drawable.ic_browser_vfs_aygor)
final class VfsRootAygor extends HttpRootBase implements VfsRoot {

  private final Context context;

  VfsRootAygor(VfsObject parent, Context context, MultisourceHttpProvider http, CacheDir cache) {
    super(parent, Catalog.create(context, http, cache, "aygor"), Path.create());
    this.context = context;
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
  @Nullable
  public VfsObject resolve(Uri uri) {
    return resolve(Path.parse(uri));
  }
}
