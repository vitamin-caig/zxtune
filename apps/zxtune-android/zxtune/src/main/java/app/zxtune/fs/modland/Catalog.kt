/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modland

import android.content.Context
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import java.io.IOException

interface Catalog {

    interface WithCountHint {
        fun setCountHint(count: Int) {}
    }

    fun interface GroupsVisitor : WithCountHint {
        override fun setCountHint(count: Int) {} // TODO: remove after KT-41670 fix
        fun accept(obj: Group)
    }

    fun interface TracksVisitor : WithCountHint {
        override fun setCountHint(count: Int) {} // TODO: remove after KT-41670 fix
        //too many tracks possible, so enable breaking
        fun accept(obj: Track): Boolean
    }

    interface Grouping {

        /**
         * Query set of group objects by filter
         * @param filter letter(s) or '#' for non-letter entries
         * @param visitor result receiver
         */
        @Throws(IOException::class)
        fun queryGroups(filter: String, visitor: GroupsVisitor, progress: ProgressCallback)

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
        fun queryTracks(id: Int, visitor: TracksVisitor, progress: ProgressCallback)

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
