package app.zxtune.fs.local

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import androidx.documentfile.provider.DocumentFile
import app.zxtune.Util
import app.zxtune.fs.StubObject
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.VfsFile
import app.zxtune.fs.VfsObject

object Document {
    @JvmStatic
    fun tryResolve(context: Context, uri: Uri): VfsFile? =
        if (DocumentFile.isDocumentUri(context, uri)) {
            DocumentFile.fromSingleUri(context, uri)?.takeIf { it.isFile }?.let {
                VfsDocumentFile(context.contentResolver, it)
            }
        } else {
            null
        }

    private class VfsDocumentFile(
        private val resolver: ContentResolver,
        private val doc: DocumentFile
    ) : StubObject(), VfsFile {
        override val uri: Uri
            get() = doc.uri
        override val name by lazy {
            doc.name.orEmpty()
        }
        override val size: String by lazy {
            Util.formatSize(doc.length())
        }
        override val parent: VfsObject?
            get() = null

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.FILE_DESCRIPTOR -> resolver.openFileDescriptor(
                doc.uri,
                "r"
            )?.fileDescriptor
            else -> super.getExtension(id)
        }
    }
}
