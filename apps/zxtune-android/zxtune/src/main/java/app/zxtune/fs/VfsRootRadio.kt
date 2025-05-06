package app.zxtune.fs

import android.content.Context
import android.net.Uri
import androidx.annotation.DrawableRes
import androidx.annotation.StringRes
import androidx.core.net.toUri
import app.zxtune.Logger
import app.zxtune.R

internal class VfsRootRadio(private val ctx: Context) : StubObject(), VfsRoot {
    override val uri: Uri = Uri.fromParts("radio", "", "")
    private val children by lazy {
        ArrayList<VfsDir>().apply {
            maybeAdd(
                this,
                "modarchive:/Random",
                R.string.vfs_modarchive_root_name,
                R.drawable.ic_browser_vfs_modarchive
            )
            maybeAdd(
                this,
                "vgmrips:/Random",
                R.string.vfs_vgmrips_root_name,
                R.drawable.ic_browser_vfs_vgmrips
            )
            maybeAdd(
                this,
                "khinsider:/Random",
                R.string.vfs_khinsider_root_name,
                R.drawable.ic_browser_vfs_khinsider
            )
        }
    }

    override val name: String
        get() = ctx.getString(R.string.vfs_radio_root_name)
    override val description: String
        get() = ctx.getString(R.string.vfs_radio_root_description)
    override val parent: VfsObject?
        get() = null

    override fun getExtension(id: String) = when (id) {
        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_radio
        else -> super.getExtension(id)
    }

    override fun resolve(uri: Uri): VfsObject? = this.takeIf { it.uri == uri }

    override fun enumerate(visitor: VfsDir.Visitor) = children.forEach(visitor::onDir)

    private fun maybeAdd(
        list: ArrayList<VfsDir>, uri: String, @StringRes description: Int, @DrawableRes icon: Int
    ) = runCatching {
        list.add(BaseEntry(uri, description, icon))
    }.onFailure { e ->
        LOG.w(e) { "Failed to add radio source" }
    }

    private inner class BaseEntry(
        private val delegate: VfsDir,
        @StringRes private val descr: Int,
        @DrawableRes private val icon: Int
    ) : StubObject(), VfsDir by delegate {

        constructor(
            uri: String, @StringRes descr: Int, @DrawableRes icon: Int
        ) : this(Vfs.resolve(uri.toUri()) as VfsDir, descr, icon)

        override val description
            get() = ctx.getString(descr)

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.ICON -> icon
            else -> delegate.getExtension(id)
        }
    }

    companion object {
        private val LOG = Logger(VfsRootRadio::class.java.name)
    }
}
