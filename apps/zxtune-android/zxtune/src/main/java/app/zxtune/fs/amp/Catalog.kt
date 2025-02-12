/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.amp

import android.content.Context
import app.zxtune.fs.http.MultisourceHttpProvider
import java.io.IOException

interface Catalog {

    interface WithCountHint {
        fun setCountHint(count: Int) {}
    }

    // TODO: generalize visitor
    fun interface GroupsVisitor : WithCountHint {
        override fun setCountHint(count: Int) {} // TODO: remove after KT-41670 fix
        fun accept(obj: Group)
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
     * Query all groups
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryGroups(visitor: GroupsVisitor)

    /**
     * Query authors by handle filter
     * @param handleFilter letter(s) or '0-9' for non-letter entries
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthors(handleFilter: String, visitor: AuthorsVisitor)

    /**
     * Query authors by country id
     * @param country scope
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthors(country: Country, visitor: AuthorsVisitor)

    /**
     * Query authors by group id
     * @param group scope
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthors(group: Group, visitor: AuthorsVisitor)

    /**
     * Query authors's tracks
     * @param author scope
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryTracks(author: Author, visitor: TracksVisitor)

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
