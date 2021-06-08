/**
 * @file
 * @brief Base Vfs object interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

import android.net.Uri

interface VfsObject {
    /**
     * @return Public unique identifier
     */
    val uri: Uri

    /**
     * @return Display name
     */
    val name: String

    /**
     * @return Optional description
     */
    val description: String

    /**
     * Retrieve parent object or null
     */
    val parent: VfsObject?

    /**
     * Retrieve extension by specified ID
     *
     * @return null if not supported
     */
    fun getExtension(id: String): Any?
}
