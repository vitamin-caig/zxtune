package app.zxtune.fs.khinsider

import androidx.core.util.Consumer
import app.zxtune.TimeStamp.Companion.fromDays
import app.zxtune.fs.dbhelpers.CommandExecutor
import app.zxtune.fs.dbhelpers.QueryCommand
import app.zxtune.utils.ProgressCallback

private val SCOPES_TTL = fromDays(30)
private val ALBUMS_TTL = fromDays(1)
private val ALBUM_TTL = fromDays(2)

class CachingCatalog(private val remote: RemoteCatalog, private val db: Database) : Catalog {
    private val executor = CommandExecutor("khinsider")

    override fun queryScopes(type: Catalog.ScopeType, visitor: Consumer<Scope>) =
        executor.executeQuery(type.name.lowercase(), object : QueryCommand {
            private val lifetime = db.getLifetime(type.name.lowercase(), SCOPES_TTL)

            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryScopes(type) { db.addScope(type, it) }
                lifetime.update()
            }

            override fun queryFromCache() = db.queryScopes(type, visitor)
        })

    override fun queryAlbums(
        scope: Scope.Id, visitor: Consumer<AlbumAndDetails>, progress: ProgressCallback
    ) = executor.executeQuery("albums", object : QueryCommand {
        private val lifetime = db.getLifetime(scope.value, ALBUMS_TTL)

        override val isCacheExpired
            get() = lifetime.isExpired

        override fun updateCache() = db.runInTransaction {
            db.cleanupScope(scope)
            remote.queryAlbums(scope, {
                db.addAlbum(scope, it)
            }, progress)
            lifetime.update()
        }

        override fun queryFromCache() = db.queryAlbums(scope, visitor)
    })

    override fun queryAlbumDetails(
        album: Album.Id, visitor: Consumer<TrackAndDetails>
    ) = if (album == Catalog.RANDOM_ALBUM) {
        queryRandomAlbum(visitor)
    } else {
        queryRegularAlbum(album, visitor)
    }

    private fun queryRandomAlbum(visitor: Consumer<TrackAndDetails>) =
        if (remote.isNetworkAvailable()) {
            queryRemoteRandomAlbum(visitor)
        } else {
            db.queryRandomAlbum()?.apply {
                db.queryTracks(album, visitor)
            }
        }

    private fun queryRemoteRandomAlbum(visitor: Consumer<TrackAndDetails>): AlbumAndDetails? {
        var result: AlbumAndDetails? = null
        db.runInTransaction {
            var albumToCleanup: Album.Id? = null
            result = remote.queryAlbumDetails(Catalog.RANDOM_ALBUM) { track ->
                if (albumToCleanup == null) {
                    albumToCleanup = track.album.id.also {
                        db.cleanupAlbum(it)
                    }
                }
                db.addTrack(track)
                visitor.accept(track)
            }?.also {
                db.addAlbum(it)
                updateLifetime(it.album.id)
            }
        }
        return result
    }


    private fun updateLifetime(album: Album.Id) = db.getLifetime(album.value, ALBUM_TTL).update()

    private fun queryRegularAlbum(
        album: Album.Id, visitor: Consumer<TrackAndDetails>
    ): AlbumAndDetails? {
        var result: AlbumAndDetails? = null
        executor.executeQuery("album", object : QueryCommand {
            private val lifetime = db.getLifetime(album.value, ALBUM_TTL)

            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                db.cleanupAlbum(album)
                var realAlbum: Album.Id? = null
                remote.queryAlbumDetails(album) { track ->
                    if (track.album.id != album && realAlbum == null) {
                        realAlbum = track.album.id.also {
                            db.cleanupAlbum(it)
                            db.addAlbumAlias(album, it)
                        }
                    }
                    db.addTrack(track)
                }?.let {
                    db.addAlbum(it)
                }
                lifetime.update()
                realAlbum?.let {
                    updateLifetime(it)
                }
            }

            override fun queryFromCache() = true == db.queryAlbum(album).also { result = it }
                ?.let { db.queryTracks(it.album, visitor) }
        })
        return result
    }

    override fun findTrackLocation(album: Album.Id, track: Track.Id) =
        remote.findTrackLocation(album, track)
}
