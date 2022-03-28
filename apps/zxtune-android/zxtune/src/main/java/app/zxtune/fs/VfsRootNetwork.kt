package app.zxtune.fs

import android.content.Context
import android.text.TextUtils
import app.zxtune.BuildConfig
import app.zxtune.R
import app.zxtune.fs.http.MultisourceHttpProvider

internal class VfsRootNetwork(private val ctx: Context, http: MultisourceHttpProvider) :
    VfsRootComposite("online") {

    init {
        addSubroot(VfsRootZxtunes(this, ctx, http))
        addSubroot(VfsRootModland(this, ctx, http))
        addSubroot(VfsRootHvsc(this, ctx, http))
        addSubroot(VfsRootZxart(this, ctx, http))
        addSubroot(VfsRootJoshw(this, ctx, http))
        addSubroot(VfsRootAmp(this, ctx, http))
        addSubroot(VfsRootAygor(this, ctx, http))
        if (!TextUtils.isEmpty(BuildConfig.MODARCHIVE_KEY)) {
            addSubroot(VfsRootModarchive(this, ctx, http, BuildConfig.MODARCHIVE_KEY))
        }
        addSubroot(VfsRootAsma(this, ctx, http))
        addSubroot(VfsRootAminet(this, ctx, http))
        addSubroot(VfsRootVgmrips(this, ctx, http))
        addSubroot(VfsRootScene(this, ctx, http))
    }

    override val name: String
        get() = ctx.getString(R.string.vfs_network_root_name)

    override fun getExtension(id: String) = when (id) {
        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_network
        else -> super.getExtension(id)
    }
}
