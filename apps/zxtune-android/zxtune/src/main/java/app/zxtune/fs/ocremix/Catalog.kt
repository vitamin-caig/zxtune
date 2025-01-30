package app.zxtune.fs.ocremix

import android.content.Context
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback

interface Catalog {
    interface WithCountHint {
        fun setCountHint(count: Int)
    }

    interface Visitor<T> : WithCountHint {
        fun accept(obj: T)
    }

    interface GamesVisitor : WithCountHint {
        fun accept(game: Game, system: System, organization: Organization?)
    }

    interface RemixesVisitor : WithCountHint {
        fun accept(remix: Remix, game: Game)
    }

    interface AlbumsVisitor : WithCountHint {
        fun accept(album: Album, image: FilePath?)
    }

    interface AlbumTracksVisitor {
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
        fun create(context: Context, httpProvider: MultisourceHttpProvider): Catalog =
            RemoteCatalog(httpProvider)
    }
}
