package app.zxtune.fs.ocremix

import android.content.Context
import androidx.core.util.Consumer
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback

interface Catalog {
    fun interface Visitor<T> : Consumer<T>

    fun interface GamesVisitor {
        fun accept(game: Game, system: System, organization: Organization?, image: FilePath?)
    }

    fun interface RemixesVisitor {
        fun accept(remix: Remix, game: Game)
    }

    fun interface AlbumsVisitor {
        fun accept(album: Album, image: FilePath?)
    }

    fun interface AlbumTracksVisitor {
        fun accept(filePath: FilePath, size: Long)
    }

    sealed interface Scope

    @JvmInline
    value class SystemScope(val id: System.Id) : Scope

    @JvmInline
    value class OrganizationScope(val id: Organization.Id) : Scope

    // only for remixes and albums
    @JvmInline
    value class GameScope(val id: Game.Id) : Scope

    fun querySystems(visitor: Visitor<System>)
    fun queryOrganizations(visitor: Visitor<Organization>, progress: ProgressCallback)
    fun queryGames(scope: Scope?, visitor: GamesVisitor, progress: ProgressCallback)
    fun queryRemixes(scope: Scope?, visitor: RemixesVisitor, progress: ProgressCallback)
    fun queryAlbums(scope: Scope?, visitor: AlbumsVisitor, progress: ProgressCallback)

    fun findRemixPath(id: Remix.Id): FilePath?
    fun queryAlbumTracks(id: Album.Id, visitor: AlbumTracksVisitor)
    fun queryGameDetails(id: Game.Id): Game.Details
    fun queryAlbumImage(id: Album.Id): FilePath?

    companion object {
        fun create(context: Context, httpProvider: MultisourceHttpProvider): Catalog {
            val remote = RemoteCatalog(httpProvider)
            val db = Database(context)
            return CachingCatalog(remote, db)
        }
    }
}
