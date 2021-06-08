/**
 * @file
 * @brief Vfs root directory interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

import android.net.Uri
import java.io.IOException

internal interface VfsRoot : VfsDir {
    /**
     * Tries to resolve object by uri
     *
     * @param uri Identifier to resolve
     * @return Found object or null otherwise
     */
    @Throws(IOException::class)
    fun resolve(uri: Uri): VfsObject?
}
