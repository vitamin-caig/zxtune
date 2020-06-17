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
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.fs.httpdir.Catalog;
import app.zxtune.fs.httpdir.HttpRootBase;
import app.zxtune.fs.scene.Path;

@Icon(R.drawable.ic_browser_vfs_scene)
final class VfsRootScene extends HttpRootBase implements VfsRoot {

  private final Context context;

  VfsRootScene(VfsObject parent, Context context, MultisourceHttpProvider http) {
    super(parent, Catalog.create(context, http, "scene"), Path.create());
    this.context = context;
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_scene_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_scene_root_description);
  }

  @Override
  @Nullable
  public VfsObject resolve(Uri uri) {
    return resolve(Path.parse(uri));
  }
}
