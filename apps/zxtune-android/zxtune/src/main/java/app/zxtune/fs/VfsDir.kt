/**
 * @file
 * @brief Vfs directory object interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

import app.zxtune.utils.ProgressCallback
import java.io.IOException

interface VfsDir : VfsObject {
    /**
     * Directory content enumerating callback
     */
    // TODO: make interface
    abstract class Visitor : ProgressCallback {
        /**
         * Called on visited directory
         */
        abstract fun onDir(dir: VfsDir)

        /**
         * Called on visited file
         */
        abstract fun onFile(file: VfsFile)

        /**
         * Called when items count is known (at any moment, maybe approximate)
         */
        open fun onItemsCount(count: Int) {}

        override fun onProgressUpdate(done: Int, total: Int) {}
    }

    /**
     * Enumerate directory content
     *
     * @param visitor Callback
     */
    @Throws(IOException::class)
    fun enumerate(visitor: Visitor)
}
