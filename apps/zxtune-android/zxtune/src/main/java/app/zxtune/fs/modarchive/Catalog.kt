/**
 * @file
 * @brief Catalog interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.modarchive

import android.content.Context
import androidx.core.util.Consumer
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import java.io.IOException

interface Catalog {
    fun interface Visitor<T> : Consumer<T>

    fun interface FoundTracksVisitor {
        fun accept(author: Author, track: Track)
    }

    /**
     * Query authors by handle filter
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryAuthors(visitor: Visitor<Author>, progress: ProgressCallback)

    /**
     * Query all genres
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryGenres(visitor: Visitor<Genre>)

    /**
     * Query authors's tracks
     * @param author scope
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryTracks(author: Author, visitor: Visitor<Track>, progress: ProgressCallback)

    /**
     * Query genre's tracks
     * @param genre scope
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun queryTracks(genre: Genre, visitor: Visitor<Track>, progress: ProgressCallback)

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
    fun findRandomTracks(visitor: Visitor<Track>)

    companion object {
        @JvmStatic
        fun create(
            context: Context, http: MultisourceHttpProvider, key: String
        ): CachingCatalog {
            val remote = RemoteCatalog(http, key)
            val db = Database(context)
            return CachingCatalog(remote, db)
        }
    }
}
