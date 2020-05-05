package app.zxtune.fs;

import android.content.Context;

import app.zxtune.R;
import app.zxtune.fs.http.MultisourceHttpProvider;

@Icon(R.drawable.ic_browser_vfs_network)
class VfsRootNetwork extends VfsRootComposite {

  private final Context ctx;

  VfsRootNetwork(Context appContext, MultisourceHttpProvider http) {
    super("online");
    this.ctx = appContext;
    addSubroot(new VfsRootZxtunes(this, appContext, http));
    addSubroot(new VfsRootModland(this, appContext, http));
    addSubroot(new VfsRootHvsc(this, appContext, http));
    addSubroot(new VfsRootZxart(this, appContext, http));
    addSubroot(new VfsRootJoshw(this, appContext, http));
    addSubroot(new VfsRootAmp(this, appContext, http));
    addSubroot(new VfsRootAygor(this, appContext, http));
    addSubroot(new VfsRootModarchive(this, appContext, http));
    addSubroot(new VfsRootAsma(this, appContext, http));
    addSubroot(new VfsRootAminet(this, appContext, http));
  }

  @Override
  public String getName() {
    return ctx.getString(R.string.vfs_network_root_name);
  }
}
