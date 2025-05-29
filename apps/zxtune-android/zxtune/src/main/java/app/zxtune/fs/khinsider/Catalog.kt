package app.zxtune.fs.khinsider

import android.content.Context
import android.net.Uri
import androidx.core.util.Consumer
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback

interface Catalog {
    enum class ScopeType {
        SERIES, PLATFORMS, TYPES, YEARS,
    }

    fun queryScopes(type: ScopeType, visitor: Consumer<Scope>)
    fun queryAlbums(scope: Scope.Id, visitor: Consumer<AlbumAndDetails>, progress: ProgressCallback)
    fun queryAlbumDetails(album: Album.Id, visitor: Consumer<TrackAndDetails>): AlbumAndDetails?
    fun findTrackLocation(album: Album.Id, track: Track.Id): Uri?

    companion object {
        val WEEKLYTOP40 = Scope.Id("top40")
        val NEW100 = Scope.Id("top-100-newly-added")
        val FAVORITES = Scope.Id("most-favorites")
        val RANDOM_ALBUM = Album.Id("*")
        fun nonLetterScope() = Scope(Scope.Id("game-soundtracks/browse/0-9"), "#")
        fun letterScope(letter: Char) =
            Scope(Scope.Id("game-soundtracks/browse/$letter"), letter.toString())

        fun create(context: Context, httpProvider: MultisourceHttpProvider): Catalog {
            val remote = RemoteCatalog(httpProvider)
            val db = Database(context)
            return CachingCatalog(remote, db)
        }
    }
}
