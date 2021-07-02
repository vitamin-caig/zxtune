package app.zxtune.fs;

import android.content.Context;
import android.text.TextUtils;

import app.zxtune.BuildConfig;
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
    if (!TextUtils.isEmpty(BuildConfig.MODARCHIVE_KEY)) {
      addSubroot(new VfsRootModarchive(this, appContext, http, BuildConfig.MODARCHIVE_KEY));
    }
    addSubroot(new VfsRootAsma(this, appContext, http));
    addSubroot(new VfsRootAminet(this, appContext, http));
    addSubroot(new VfsRootVgmrips(this, appContext, http));
    addSubroot(new VfsRootScene(this, appContext, http));
  }

  @Override
  public String getName() {
    return ctx.getString(R.string.vfs_network_root_name);
  }
}
