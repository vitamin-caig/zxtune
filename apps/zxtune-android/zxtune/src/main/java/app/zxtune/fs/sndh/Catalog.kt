package app.zxtune.fs.sndh

import android.content.Context
import androidx.core.util.Consumer
import app.zxtune.fs.http.MultisourceHttpProvider

interface Catalog {
    fun interface FoundTracksVisitor {
        fun accept(author: Author, track: Track)
    }

    fun queryAuthors(visitor: Consumer<Author>)

    fun queryTracks(author: Author, visitor: Consumer<Track>)

    fun findTracks(query: String, visitor: FoundTracksVisitor)

    fun findRandomTrack(): Track?

    companion object {
        @JvmStatic
        fun create(context: Context, http: MultisourceHttpProvider): Catalog {
            return RemoteCatalog(http)
        }
    }
}
