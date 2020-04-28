/**
 * @file
 * @brief Implementation of VfsRoot over http://asma.atari.org collection
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;

import androidx.annotation.Nullable;

import app.zxtune.R;
import app.zxtune.fs.asma.Path;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.fs.httpdir.Catalog;
import app.zxtune.fs.httpdir.HttpRootBase;

@Icon(R.drawable.ic_browser_vfs_asma)
final class VfsRootAsma extends HttpRootBase implements VfsRoot {

  private final Context context;

  VfsRootAsma(VfsObject parent, Context context, MultisourceHttpProvider http) {
    super(parent, Catalog.create(context, http, "asma"), Path.create());
    this.context = context;
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_asma_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_asma_root_description);
  }

  @Override
  @Nullable
  public VfsObject resolve(Uri uri) {
    return resolve(Path.parse(uri));
  }
}
