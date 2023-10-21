package app.zxtune.fs

import android.content.Context
import android.net.Uri
import android.os.storage.StorageManager
import android.provider.DocumentsContract
import androidx.annotation.RequiresApi
import androidx.core.content.getSystemService
import androidx.core.database.getStringOrNull
import androidx.documentfile.provider.DocumentFile
import app.zxtune.R
import app.zxtune.ResultActivity
import app.zxtune.Util
import app.zxtune.fs.local.Document
import app.zxtune.fs.local.Identifier
import app.zxtune.fs.local.PersistablePermissions
import app.zxtune.fs.local.Utils.isMounted
import app.zxtune.fs.local.Utils.mountPoint
import app.zxtune.fs.local.Utils.rootId
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

    override fun enumerate(visitor: VfsDir.Visitor) =
        manager.storageVolumes.also { visitor.onItemsCount(it.size) }.forEach {
            if (it.isMounted()) {
                StorageRoot(Identifier.fromRoot(it.rootId()), it.getDescription(context)).run {
                    visitor.onDir(this)
                }
            }
        }

    override fun resolve(uri: Uri): VfsObject? {
        val id = Identifier.fromFsUri(uri) ?: return Document.tryResolve(context, uri)
        return resolve(id)
    }

    private fun resolve(id: Identifier): VfsObject? = when {
        id == Identifier.EMPTY -> this
        id.root.isEmpty() -> convertLegacyIdentifier(id)?.let {
            resolve(it)
        }

        id.path.isEmpty() -> StorageRoot(id)
        else -> accessibleDirectories.findAncestor(id)?.let { treeId ->
            DocumentFile.fromTreeUri(context, id.getDocumentUriUsingTree(treeId))?.run {
                if (isDirectory) LocalDir(id, treeId) else LocalFile(id, treeId)
            }
        } ?: if (accessibleDirectories.getDirectChildrenOf(id.parent).contains(id)) {
            // Phantom dir
            LocalDir(id)
        } else {
            null
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
            VfsExtensions.PERMISSION_QUERY_INTENT -> ResultActivity.createStoragePermissionRequestIntent(
                context, this.id.rootUri
            )

            else -> super.getExtension(id)
        }
    }

    private inner class LocalDir(
        id: Identifier, treeId: Identifier? = null, override val description: String = ""
    ) : BaseObject(id), VfsDir {

        private val treeId by lazy {
            treeId ?: accessibleDirectories.findAncestor(id)
        }

        override fun enumerate(visitor: VfsDir.Visitor) = treeId?.let { treeId ->
            getDocumentChildren(treeId, visitor)
        } ?: getPhantomDirs(id, visitor)

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.PERMISSION_QUERY_INTENT -> this.id.documentUri.takeIf { treeId == null }
                ?.let { uri ->
                    ResultActivity.createStoragePermissionRequestIntent(context, uri)
                }

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
                    visitor.onFile(LocalFile(subId, treeId, description, Util.formatSize(size)))
                }
            }
        } ?: Unit
    }

    private fun getPhantomDirs(id: Identifier, visitor: VfsDir.Visitor) =
        accessibleDirectories.getDirectChildrenOf(id).also { visitor.onItemsCount(it.size) }
            .forEach {
                visitor.onDir(LocalDir(it))
            }

    private inner class LocalFile(
        id: Identifier,
        treeId: Identifier? = null,
        override val description: String = "",
        override val size: String = "",
    ) : BaseObject(id), VfsFile {

        private val treeId by lazy {
            treeId ?: accessibleDirectories.findAncestor(id)
        }

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.FILE_DESCRIPTOR -> resolver.openFileDescriptor(
                fileUri, "r"
            )?.fileDescriptor

            else -> super.getExtension(id)
        }

        // Return something to give explicit error
        private val fileUri
            get() = treeId?.let { id.getDocumentUriUsingTree(it) } ?: id.documentUri
    }

    private fun convertLegacyIdentifier(id: Identifier): Identifier? {
        val fullPath = "${File.separatorChar}${id.path}"
        manager.storageVolumes.forEach { vol ->
            val volPath = vol.mountPoint()?.absolutePath ?: return@forEach
            fullPath.extractRelativePath(volPath)?.let {
                return Identifier(vol.rootId(), it)
            }
        }
        return null
    }

    companion object {
        private fun String.extractRelativePath(subpath: String) = when {
            this == subpath -> ""
            startsWith(subpath) && getOrNull(subpath.length) == File.separatorChar -> drop(subpath.length + 1)
            else -> null
        }
    }
}
