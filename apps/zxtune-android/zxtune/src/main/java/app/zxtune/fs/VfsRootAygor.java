/**
 * @file
 * @brief Implementation of VfsRoot over http://hvsc.c64.org collection
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;
import androidx.annotation.Nullable;

import java.io.IOException;

import app.zxtune.R;
import app.zxtune.fs.aygor.Path;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.httpdir.Catalog;
import app.zxtune.fs.httpdir.HttpRootBase;

final class VfsRootAygor extends HttpRootBase implements VfsRoot {

  private final Context context;

  VfsRootAygor(Context context, HttpProvider http, CacheDir cache) throws IOException {
    super(Catalog.create(context, http, cache, "aygor"), Path.create());
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
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_aygor;
    } else {
      return super.getExtension(id);
    }
  }

  @Override
  @Nullable
  public VfsObject resolve(Uri uri) throws IOException {
    return resolve(Path.parse(uri));
  }
}
