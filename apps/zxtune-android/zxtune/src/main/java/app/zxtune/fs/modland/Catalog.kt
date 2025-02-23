/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modland

import android.content.Context
import androidx.core.util.Consumer
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import java.io.IOException

interface Catalog {
    fun interface Visitor<T> : Consumer<T>

    interface Grouping {

        /**
         * Query set of group objects by filter
         * @param filter letter(s) or '#' for non-letter entries
         * @param visitor result receiver
         */
        @Throws(IOException::class)
        fun queryGroups(filter: String, visitor: Visitor<Group>, progress: ProgressCallback)

        /**
         * Query single group object
         * @param id object identifier
         */
        @Throws(IOException::class)
        fun getGroup(id: Int): Group

        /**
         * Query group's tracks
         * @param id object identifier
         * @param visitor result receiver
         */
        @Throws(IOException::class)
        fun queryTracks(id: Int, visitor: Visitor<Track>, progress: ProgressCallback)

        /**
         * Query track by name
         * @param id object identifier
         * @param filename track filename
         */
        @Throws(IOException::class)
        fun getTrack(id: Int, filename: String): Track
    }

    fun getAuthors(): Grouping

    fun getCollections(): Grouping

    fun getFormats(): Grouping

    companion object {
        @JvmStatic
        fun create(context: Context, http: MultisourceHttpProvider): CachingCatalog {
            val remote = RemoteCatalog(http)
            val db = Database(context)
            return CachingCatalog(remote, db)
        }
    }
}
