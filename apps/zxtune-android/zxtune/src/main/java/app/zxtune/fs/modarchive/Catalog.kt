/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.modarchive

import android.content.Context
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import java.io.IOException

interface Catalog {
    interface WithCountHint {
        fun setCountHint(count: Int) {}
    }

    fun interface AuthorsVisitor : WithCountHint {
        override fun setCountHint(count: Int) {} // TODO: remove after KT-41670 fix
        fun accept(obj: Author)
    }

    fun interface GenresVisitor : WithCountHint {
        override fun setCountHint(count: Int) {} // TODO: remove after KT-41670 fix
        fun accept(obj: Genre)
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
     * Query authors by handle filter
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthors(visitor: AuthorsVisitor, progress: ProgressCallback)

    /**
     * Query all genres
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryGenres(visitor: GenresVisitor)

    /**
     * Query authors's tracks
     * @param author scope
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryTracks(author: Author, visitor: TracksVisitor, progress: ProgressCallback)

    /**
     * Query genre's tracks
     * @param genre scope
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryTracks(genre: Genre, visitor: TracksVisitor, progress: ProgressCallback)

    /**
     * Find tracks by query substring
     * @param query string to search in filename/title
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun findTracks(query: String, visitor: FoundTracksVisitor)

    /**
     * Queries next random track
     * @throws IOException
     */
    @Throws(IOException::class)
    fun findRandomTracks(visitor: TracksVisitor)

    companion object {
        @JvmStatic
        fun create(
            context: Context,
            http: MultisourceHttpProvider,
            key: String
        ): CachingCatalog {
            val remote = RemoteCatalog(http, key)
            val db = Database(context)
            return CachingCatalog(remote, db)
        }
    }
}
