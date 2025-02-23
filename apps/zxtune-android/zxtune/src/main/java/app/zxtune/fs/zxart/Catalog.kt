/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxart

import android.content.Context
import androidx.core.util.Consumer
import app.zxtune.fs.http.MultisourceHttpProvider
import java.io.IOException

interface Catalog {
    fun interface Visitor<T> : Consumer<T>

    fun interface FoundTracksVisitor {
        fun accept(author: Author, track: Track)
    }

    /**
     * Query authors object
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthors(visitor: Visitor<Author>)

    /**
     * Query tracks objects
     * @param author tracks owner
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthorTracks(author: Author, visitor: Visitor<Track>)

    /**
     * Query parties object
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryParties(visitor: Visitor<Party>)

    /**
     * Query tracks objects
     * @param party filter by party
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryPartyTracks(party: Party, visitor: Visitor<Track>)

    /**
     * Query top tracks (not cached
     * @param limit count
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryTopTracks(limit: Int, visitor: Visitor<Track>)

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
