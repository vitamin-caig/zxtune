package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;
import androidx.annotation.Nullable;
import app.zxtune.R;
import app.zxtune.fs.aminet.Path;
import app.zxtune.fs.aminet.RemoteCatalog;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.httpdir.Catalog;
import app.zxtune.fs.httpdir.HttpRootBase;

import java.io.IOException;

final class VfsRootAminet extends HttpRootBase implements VfsRoot {

  private final Context context;
  private final RemoteCatalog remote;

  VfsRootAminet(Context context, HttpProvider http, CacheDir cache) throws IOException {
    this(context, new RemoteCatalog(http), cache);
  }

  private VfsRootAminet(Context context, RemoteCatalog remote, CacheDir cache) throws IOException {
    super(Catalog.create(context, remote, cache, "aminet"), Path.create());
    this.context = context;
    this.remote = remote;
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_aminet_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_aminet_root_description);
  }

  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_aminet;
    } else if (VfsExtensions.SEARCH_ENGINE.equals(id) && remote.searchSupported()) {
      return new SearchEngine();
    } else {
      return super.getExtension(id);
    }
  }

  @Override
  @Nullable
  public VfsObject resolve(Uri uri) {
    return resolve(Path.parse(uri));
  }

  private class SearchEngine implements VfsExtensions.SearchEngine {

    @Override
    public void find(String query, final Visitor visitor) throws IOException {
      remote.find(query, new Catalog.DirVisitor() {

        @Override
        public void acceptDir(String name, String description) {
        }

        @Override
        public void acceptFile(String name, String description, String size) {
          visitor.onFile(makeFile(rootPath.getChild(name), description, size));
        }
      });
    }
  }
}
