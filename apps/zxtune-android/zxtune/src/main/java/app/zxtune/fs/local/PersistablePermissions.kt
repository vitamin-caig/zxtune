package app.zxtune.fs.local

import android.content.ContentResolver
import android.content.UriPermission
import androidx.annotation.RequiresApi

@RequiresApi(24)
internal class PersistablePermissions(private val resolver: ContentResolver) {
    fun getDirectChildrenOf(id: Identifier) = mutableSetOf<Identifier>().apply {
        identifiers.forEach { allowed ->
            allowed.path.takeIf { allowed.root == id.root }?.nextLevelSubPathTo(id.path)
                ?.let { path ->
                    add(Identifier(id.root, path))
                }
        }
    }

    fun findAncestor(id: Identifier) = identifiers.find { allowed ->
        allowed.root == id.root && (id.path == allowed.path || id.path.isSubPathTo(allowed.path))
    }

    private val identifiers
        get() = resolver.persistedUriPermissions
            .filter(UriPermission::isReadPermission)
            .map { Identifier.fromTreeDocumentUri(it.uri) }

    companion object {
        private fun String.isSubPathTo(rh: String) = rh.isEmpty() ||
                (startsWith(rh) && Identifier.PATH_DELIMITER == elementAtOrNull(rh.length))

        private fun String.nextLevelSubPathTo(rh: String) = if (isSubPathTo(rh)) {
            val end = indexOf(Identifier.PATH_DELIMITER, rh.length + 1)
            if (end == -1) {
                this
            } else {
                substring(0, end)
            }
        } else {
            null
        }
    }
}