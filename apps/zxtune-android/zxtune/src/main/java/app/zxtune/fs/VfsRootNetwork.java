package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;

import java.io.IOException;

import app.zxtune.R;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.cache.CacheFactory;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;

@Icon(R.drawable.ic_browser_vfs_network)
class VfsRootNetwork extends VfsRootComposite {

  private final Context ctx;

  VfsRootNetwork(Context appContext) throws IOException {
    super("online");
    this.ctx = appContext;
    final HttpProvider http = HttpProviderFactory.createProvider(appContext);
    final CacheDir cache = CacheFactory.create(appContext);
    addSubroot(new VfsRootZxtunes(this, appContext, http, cache));
    addSubroot(new VfsRootModland(this, appContext, http, cache));
    addSubroot(new VfsRootHvsc(this, appContext, http, cache));
    addSubroot(new VfsRootZxart(this, appContext, http, cache));
    addSubroot(new VfsRootJoshw(this, appContext, http, cache));
    addSubroot(new VfsRootAmp(this, appContext, http, cache));
    addSubroot(new VfsRootAygor(this, appContext, http, cache));
    addSubroot(new VfsRootModarchive(this, appContext, http, cache));
    addSubroot(new VfsRootAsma(this, appContext, http, cache));
    addSubroot(new VfsRootAminet(this, appContext, http, cache));
  }

  @Override
  public String getName() {
    return ctx.getString(R.string.vfs_network_root_name);
  }
}
