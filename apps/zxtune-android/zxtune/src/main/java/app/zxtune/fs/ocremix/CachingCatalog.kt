package app.zxtune.fs.ocremix

import app.zxtune.TimeStamp.Companion.fromDays
import app.zxtune.fs.dbhelpers.CommandExecutor
import app.zxtune.fs.dbhelpers.QueryCommand
import app.zxtune.utils.ProgressCallback

private val SYSTEMS_TTL = fromDays(30)
private val ORGANIZATIONS_TTL = fromDays(30)
private val GAMES_TTL = fromDays(7)
private val REMIXES_TTL = fromDays(2)
private val ALBUMS_TTL = fromDays(7)
private val OBJECTS_TTL = fromDays(14)

class CachingCatalog(private val remote: RemoteCatalog, private val db: Database) : Catalog {
    private val executor = CommandExecutor("ocremix")

    override fun querySystems(visitor: Catalog.Visitor<System>) =
        executor.executeQuery("systems", object : QueryCommand {
            private val lifetime = db.getLifetime("systems", SYSTEMS_TTL)

            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.querySystems(db::addSystem)
                lifetime.update()
            }

            override fun queryFromCache() = db.querySystems(visitor)
        })

    override fun queryOrganizations(
        visitor: Catalog.Visitor<Organization>, progress: ProgressCallback
    ) = executor.executeQuery("organizations", object : QueryCommand {
        private val lifetime = db.getLifetime("organizations", ORGANIZATIONS_TTL)

        override val isCacheExpired
            get() = lifetime.isExpired

        override fun updateCache() = db.runInTransaction {
            remote.queryOrganizations(db::addOrganization, progress)
            lifetime.update()
        }

        override fun queryFromCache() = db.queryOrganizations(visitor)
    })

    override fun queryGames(
        scope: Catalog.Scope?, visitor: Catalog.GamesVisitor, progress: ProgressCallback
    ) = executor.executeQuery("games", object : QueryCommand {
        private val lifetime = db.getLifetime("games@${scope.id}", GAMES_TTL)

        override val isCacheExpired
            get() = lifetime.isExpired

        override fun updateCache() = db.runInTransaction {
            remote.queryGames(scope, db::addGame, progress)
            lifetime.update()
        }

        override fun queryFromCache() = db.queryGames(scope, visitor)
    })

    override fun queryRemixes(
        scope: Catalog.Scope?, visitor: Catalog.RemixesVisitor, progress: ProgressCallback
    ) = executor.executeQuery("remixes", object : QueryCommand {
        private val lifetime = db.getLifetime("remixes@${scope.id}", REMIXES_TTL)

        override val isCacheExpired
            get() = lifetime.isExpired

        override fun updateCache() = db.runInTransaction {
            remote.queryRemixes(scope, { remix, game -> db.addRemix(scope, remix, game) }, progress)
            lifetime.update()
        }

        override fun queryFromCache() = db.queryRemixes(scope, visitor)
    })

    override fun queryAlbums(
        scope: Catalog.Scope?, visitor: Catalog.AlbumsVisitor, progress: ProgressCallback
    ) = executor.executeQuery("albums", object : QueryCommand {
        private val lifetime = db.getLifetime("albums@${scope.id}", ALBUMS_TTL)

        override val isCacheExpired
            get() = lifetime.isExpired

        override fun updateCache() = db.runInTransaction {
            remote.queryAlbums(
                scope, { album, image -> db.addAlbum(scope, album, image) }, progress
            )
            lifetime.update()
        }

        override fun queryFromCache() = db.queryAlbums(scope, visitor)
    })

    override fun findRemixPath(id: Remix.Id): FilePath? {
        var result: FilePath? = null
        executor.executeQuery("mp3", object : QueryCommand {
            private val lifetime = db.getLifetime("mp3@${id.value}", OBJECTS_TTL)

            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                result = remote.findRemixPath(id)?.also {
                    db.run {
                        deleteMusicFiles(id.value)
                        addMusicFile(id.value, it)
                    }
                    lifetime.update()
                }
            }

            override fun queryFromCache() = db.queryMusicFiles(id.value).firstOrNull().also {
                result = it?.path
            } != null
        })
        return result
    }

    override fun queryAlbumTracks(id: Album.Id, visitor: Catalog.AlbumTracksVisitor) =
        executor.executeQuery("mp3", object : QueryCommand {
            private val lifetime = db.getLifetime("mp3@${id.value}", OBJECTS_TTL)

            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                var hasContent = false
                remote.queryAlbumTracks(id, object : Catalog.AlbumTracksVisitor {
                    override fun accept(filePath: FilePath, size: Long) {
                        if (!hasContent) {
                            db.deleteMusicFiles(id.value)
                            hasContent = true
                        }
                        db.addMusicFile(id.value, filePath, size)
                    }
                })
                if (hasContent) {
                    lifetime.update()
                }
            }

            override fun queryFromCache() = db.queryMusicFiles(id.value).onEach {
                visitor.accept(it.path, it.size ?: 0)
            }.isNotEmpty()
        })

    override fun queryGameDetails(id: Game.Id): Game.Details {
        var result: Game.Details? = null
        executor.executeQuery("game", object : QueryCommand {
            private val lifetime = db.getLifetime(id.value, OBJECTS_TTL)

            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryGameDetails(id).run {
                    image?.let {
                        db.addImage(id.value, it)
                    }
                    chiptunePath?.let {
                        db.deleteMusicFiles(id.value)
                        db.addMusicFile(id.value, it)
                    }
                    if (image != null || chiptunePath != null) {
                        lifetime.update()
                    }
                }
            }

            override fun queryFromCache() = with(db) {
                Game.Details(queryMusicFiles(id.value).firstOrNull()?.path, queryImage(id.value))
            }.also {
                result = it
            }.run {
                chiptunePath != null || image != null
            }
        })
        return result ?: Game.Details(null, null)
    }

    override fun queryAlbumImage(id: Album.Id): FilePath? {
        var result: FilePath? = null
        executor.executeQuery("album", object : QueryCommand {
            private val lifetime = db.getLifetime(id.value, OBJECTS_TTL)

            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryAlbumImage(id)?.let {
                    db.addImage(id.value, it)
                    lifetime.update()
                }
            }

            override fun queryFromCache() = db.queryImage(id.value).also {
                result = it
            } != null
        })
        return result
    }
}

private val Catalog.Scope?.id
    get() = when (this) {
        is Catalog.SystemScope -> id.value
        is Catalog.OrganizationScope -> id.value
        is Catalog.GameScope -> id.value
        else -> ""
    }
