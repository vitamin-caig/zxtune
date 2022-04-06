package app.zxtune.fs.local

import android.net.Uri
import android.provider.DocumentsContract
import androidx.annotation.RequiresApi
import androidx.annotation.VisibleForTesting

@RequiresApi(24)
data class Identifier(val root: String, val path: String) {

    @VisibleForTesting
    val documentId = if (path.isEmpty()) {
        root
    } else {
        "$root$DOCID_DELIMITER$path"
    }

    val name
        get() = if (path.isEmpty()) {
            root
        } else {
            path.substringAfterLast(PATH_DELIMITER)
        }

    val parent
        get() = if (path.isEmpty()) {
            EMPTY
        } else {
            Identifier(root, path.substringBeforeLast(PATH_DELIMITER, ""))
        }

    val fsUri: Uri
        get() = Uri.Builder().scheme(SCHEME).authority(root).path(path).build()

    val rootUri: Uri
        get() = DocumentsContract.buildRootUri(PROVIDER_AUTHORITY, root)

    val documentUri: Uri
        get() = DocumentsContract.buildDocumentUri(PROVIDER_AUTHORITY, documentId)

    fun getDocumentUriUsingTree(treeId: Identifier): Uri =
        DocumentsContract.buildDocumentUriUsingTree(treeId.treeDocumentUri, documentId)

    fun getTreeChildDocumentUri(treeId: Identifier): Uri =
        DocumentsContract.buildChildDocumentsUriUsingTree(treeId.treeDocumentUri, documentId)

    @VisibleForTesting
    val treeDocumentUri: Uri
        get() = DocumentsContract.buildTreeDocumentUri(PROVIDER_AUTHORITY, documentId)

    companion object {
        private const val SCHEME = "file"

        // https://android.googlesource.com/platform/frameworks/base/+/master/packages/ExternalStorageProvider/src/com/android/externalstorage/ExternalStorageProvider.java
        private const val PROVIDER_AUTHORITY = "com.android.externalstorage.documents"
        private const val DOCID_DELIMITER = ':'
        const val PATH_DELIMITER = '/'

        val EMPTY = Identifier("", "")

        fun fromRoot(root: String) = Identifier(root, "")

        fun fromFsUri(uri: Uri) = uri.takeIf { SCHEME == it.scheme }?.let {
            Identifier(uri.authority.orEmpty(), uri.path?.substringAfter(PATH_DELIMITER).orEmpty())
        }

        fun fromTreeDocumentUri(uri: Uri) = fromDocumentId(DocumentsContract.getTreeDocumentId(uri))

        // Same as in ExternalStorageProvider
        fun fromDocumentId(id: String) = when (val delimiter = id.indexOf(DOCID_DELIMITER, 1)) {
            -1 -> Identifier(id, "")
            else -> Identifier(id.substring(0, delimiter), id.substring(delimiter + 1))
        }
    }
}