package app.zxtune.fs

import android.content.Context
import android.net.Uri
import android.os.storage.StorageManager
import android.provider.DocumentsContract
import androidx.annotation.RequiresApi
import androidx.core.content.getSystemService
import androidx.core.database.getStringOrNull
import app.zxtune.R
import app.zxtune.Util
import app.zxtune.fs.local.*
import java.io.File

@RequiresApi(29)
class VfsRootLocalStorageAccessFramework(private val context: Context) : StubObject(), VfsRoot {
    private val manager by lazy {
        requireNotNull(context.getSystemService<StorageManager>())
    }

    private val accessibleDirectories by lazy {
        PersistablePermissions(resolver)
    }

    private val resolver by lazy {
        context.contentResolver
    }

    override val uri: Uri
        get() = Identifier.EMPTY.fsUri
    override val name
        get() = context.getString(R.string.vfs_local_root_name)
    override val description
        get() = context.getString(R.string.vfs_local_root_description)
    override val parent: VfsObject?
        get() = null

    override fun getExtension(id: String) = when (id) {
        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_local
        else -> super.getExtension(id)
    }

    override fun enumerate(visitor: VfsDir.Visitor) = manager.storageVolumes
        .also { visitor.onItemsCount(it.size) }
        .forEach {
            if (it.isMounted()) {
                StorageRoot(Identifier.fromRoot(it.rootId()), it.getDescription(context)).run {
                    visitor.onDir(this)
                }
            }
        }

    override fun resolve(uri: Uri) = Identifier.fromFsUri(uri)?.let {
        resolve(it)
    }

    private fun resolve(id: Identifier) = when {
        id == Identifier.EMPTY -> this
        id.root.isEmpty() -> LegacyFile(id)
        id.path.isEmpty() -> StorageRoot(id)
        else -> when (resolver.getType(id.documentUri)) {
            null -> null
            DocumentsContract.Document.MIME_TYPE_DIR -> LocalDir(id)
            else -> LocalFile(id)
        }
    }

    private fun query(uri: Uri, vararg columns: String) = resolver.query(uri, columns, null, null)

    private abstract inner class BaseObject(
        protected val id: Identifier,
    ) : StubObject() {

        override val uri: Uri
            get() = id.fsUri
        override val name: String
            get() = id.name
        override val parent: VfsObject?
            get() = resolve(id.parent)
    }

    private inner class StorageRoot(
        id: Identifier,
        override val description: String = "",
    ) : BaseObject(id), VfsDir {

        override fun enumerate(visitor: VfsDir.Visitor) = getPhantomDirs(id, visitor)

        override val parent = this@VfsRootLocalStorageAccessFramework

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.PERMISSION_QUERY_URI -> this.id.rootUri
            else -> super.getExtension(id)
        }
    }

    private inner class LocalDir(
        id: Identifier,
        treeId: Identifier? = null,
        override val description: String = ""
    ) : BaseObject(id), VfsDir {

        private val treeId by lazy {
            treeId ?: accessibleDirectories.findAncestor(id)
        }

        override fun enumerate(visitor: VfsDir.Visitor) = treeId?.let { treeId ->
            getDocumentChildren(treeId, visitor)
        } ?: getPhantomDirs(id, visitor)

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.PERMISSION_QUERY_URI -> this.id.documentUri.takeIf { treeId == null }
            else -> super.getExtension(id)
        }

        private fun getDocumentChildren(
            treeId: Identifier,
            visitor: VfsDir.Visitor,
        ) = query(
            id.getTreeChildDocumentUri(treeId),
            DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_MIME_TYPE,
            DocumentsContract.Document.COLUMN_SUMMARY,
            DocumentsContract.Document.COLUMN_SIZE,
        )?.use { cursor ->
            visitor.onItemsCount(cursor.count)
            while (cursor.moveToNext()) {
                val subId = Identifier.fromDocumentId(cursor.getString(0))
                val type = cursor.getString(1)
                val description = cursor.getStringOrNull(2).orEmpty()
                if (DocumentsContract.Document.MIME_TYPE_DIR == type) {
                    visitor.onDir(LocalDir(subId, treeId, description))
                } else {
                    val size = cursor.getLong(3)
                    visitor.onFile(LocalFile(subId, description, Util.formatSize(size)))
                }
            }
        } ?: Unit
    }

    private fun getPhantomDirs(id: Identifier, visitor: VfsDir.Visitor) =
        accessibleDirectories.getDirectChildrenOf(id)
            .also { visitor.onItemsCount(it.size) }
            .forEach {
                visitor.onDir(LocalDir(it))
            }

    private inner class LocalFile(
        id: Identifier,
        override val description: String = "",
        override val size: String = "",
    ) : BaseObject(id), VfsFile {

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.FILE -> findMountPoint()?.let {
                File(it, this.id.path)
            }
            else -> super.getExtension(id)
        }

        private fun findMountPoint() =
            manager.storageVolumes.find { this.id.root == it.rootId() }?.mountPoint()
    }

    private inner class LegacyFile(
        id: Identifier,
        override val size: String = "",
    ) : BaseObject(id), VfsFile {

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.FILE -> File("/${this.id.path}")
            else -> super.getExtension(id)
        }
    }

    companion object {
        const val REQUIRED_SDK_LEVEL = 30
    }
}
