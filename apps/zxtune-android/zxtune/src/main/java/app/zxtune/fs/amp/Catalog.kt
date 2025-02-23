/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.amp

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
     * Query all groups
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryGroups(visitor: Visitor<Group>)

    /**
     * Query authors by handle filter
     * @param handleFilter letter(s) or '0-9' for non-letter entries
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthors(handleFilter: String, visitor: Visitor<Author>)

    /**
     * Query authors by country id
     * @param country scope
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthors(country: Country, visitor: Visitor<Author>)

    /**
     * Query authors by group id
     * @param group scope
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthors(group: Group, visitor: Visitor<Author>)

    /**
     * Query authors's tracks
     * @param author scope
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryTracks(author: Author, visitor: Visitor<Track>)

    /**
     * Find tracks by query substring
     * @param query string to search in filename/title
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun findTracks(query: String, visitor: FoundTracksVisitor)

    companion object {
        const val NON_LETTER_FILTER = "0-9"

        @JvmStatic
        fun create(context: Context, http: MultisourceHttpProvider): CachingCatalog {
            val remote = RemoteCatalog(http)
            val db = Database(context)
            return CachingCatalog(remote, db)
        }
    }
}
