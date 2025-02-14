/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.archives

import android.text.TextUtils
import app.zxtune.core.Identifier

class DirEntry private constructor(
    @JvmField val path: Identifier, @JvmField val parent: Identifier, @JvmField val filename: String
) {
    val isRoot: Boolean
        get() = filename.isEmpty()

    companion object {
        @JvmStatic
        fun create(id: Identifier): DirEntry {
            val subpath = id.subPath
            if (TextUtils.isEmpty(subpath)) {/*
        * TODO: fix model to avoid conflicts in the next case:
        *
        * file.ext - module at the beginning
        * file.exe?+100 - module at the offset
        */
                return DirEntry(id, id, "")
            } else {
                var parentAndFilename = subpath.splitRight('/')
                while (parentAndFilename.first.isNotEmpty()) {
                    val (parent, filename) = parentAndFilename
                    if (!isPacked(filename)) {
                        return DirEntry(id, Identifier(id.dataLocation, parent), filename)
                    }
                    parentAndFilename = parent.splitRight('/')
                }
                assert(parentAndFilename.first.isEmpty())
                return DirEntry(id,
                    Identifier(id.dataLocation, ""),
                    parentAndFilename.second.takeUnless { isPacked(it) }
                        ?: id.dataLocation.lastPathSegment.orEmpty())
            }
        }
    }
}

// TODO: reuse PACKED_DATA_PREFIX
private fun isPacked(name: String) = name.startsWith("+un")

private fun String.splitRight(delimiter: Char) = lastIndexOf(delimiter).takeIf { it != -1 }?.let {
    substring(0, it) to substring(it + 1)
} ?: ("" to this)
