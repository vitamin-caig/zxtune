/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxtunes

import android.content.Context
import app.zxtune.fs.http.MultisourceHttpProvider
import java.io.IOException

interface Catalog {

    interface WithCountHint {
        fun setCountHint(count: Int) {}
    }

    fun interface AuthorsVisitor : WithCountHint {
        override fun setCountHint(count: Int) {} // TODO: remove after KT-41670 fix
        fun accept(obj: Author)
    }

    fun interface TracksVisitor : WithCountHint {
        override fun setCountHint(count: Int) {} // TODO: remove after KT-41670 fix
        fun accept(obj: Track)
    }

    fun interface FoundTracksVisitor : WithCountHint {
        override fun setCountHint(count: Int) {} // TODO: remove after KT-41670 fix
        fun accept(author: Author, track: Track)
    }

    /**
     * Query authors object
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthors(visitor: AuthorsVisitor)

    /**
     * Query tracks objects
     * @param author scope
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthorTracks(author: Author, visitor: TracksVisitor)

    /**
     * Checks whether tracks can be found directly from catalogue instead of scanning
     */
    fun searchSupported(): Boolean

    /**
     * Find tracks by query substring
     * @param query string to search in filename/title
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun findTracks(query: String, visitor: FoundTracksVisitor)

    companion object {
        @JvmStatic
        fun create(context: Context, http: MultisourceHttpProvider): CachingCatalog {
            val remote = RemoteCatalog(http)
            val db = Database(context)
            return CachingCatalog(remote, db)
        }
    }
}
