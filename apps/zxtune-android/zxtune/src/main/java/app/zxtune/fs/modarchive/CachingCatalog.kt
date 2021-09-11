/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.modarchive

import app.zxtune.TimeStamp.Companion.fromDays
import app.zxtune.fs.dbhelpers.CommandExecutor
import app.zxtune.fs.dbhelpers.QueryCommand
import app.zxtune.fs.modarchive.Catalog.*
import app.zxtune.utils.ProgressCallback

private val AUTHORS_TTL = fromDays(2)
private val GENRES_TTL = fromDays(30)
private val TRACKS_TTL = fromDays(2)

class CachingCatalog internal constructor(
    private val remote: RemoteCatalog,
    private val db: Database
) : Catalog {
    private val executor: CommandExecutor = CommandExecutor("modarchive")

    override fun queryAuthors(visitor: AuthorsVisitor, progress: ProgressCallback) =
        executor.executeQuery("authors", object : QueryCommand {
            private val lifetime = db.getAuthorsLifetime(AUTHORS_TTL)
            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryAuthors(db::addAuthor, progress)
                lifetime.update()
            }

            override fun queryFromCache() = db.queryAuthors(visitor)
        })

    override fun queryGenres(visitor: GenresVisitor) =
        executor.executeQuery("genres", object : QueryCommand {
            private val lifetime = db.getGenresLifetime(GENRES_TTL)
            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryGenres(db::addGenre)
                lifetime.update()
            }

            override fun queryFromCache() = db.queryGenres(visitor)
        })

    override fun queryTracks(author: Author, visitor: TracksVisitor, progress: ProgressCallback) {
        executor.executeQuery("tracks", object : QueryCommand {
            private val lifetime = db.getAuthorTracksLifetime(author, TRACKS_TTL)
            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryTracks(author, { obj ->
                    db.addTrack(obj)
                    db.addAuthorTrack(author, obj)
                }, progress)
                lifetime.update()
            }

            override fun queryFromCache() = db.queryTracks(author, visitor)
        })
    }

    override fun queryTracks(genre: Genre, visitor: TracksVisitor, progress: ProgressCallback) =
        executor.executeQuery("tracks", object : QueryCommand {
            private val lifetime = db.getGenreTracksLifetime(genre, TRACKS_TTL)
            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryTracks(genre, { obj ->
                    db.addTrack(obj)
                    db.addGenreTrack(genre, obj)
                }, progress)
                lifetime.update()
            }

            override fun queryFromCache() = db.queryTracks(genre, visitor)
        })

    override fun findTracks(query: String, visitor: FoundTracksVisitor) =
        if (remote.searchSupported()) {
            remote.findTracks(query, visitor)
        } else {
            db.findTracks(query, visitor)
        }

    //TODO: another method?
    override fun findRandomTracks(visitor: TracksVisitor) = if (remote.searchSupported()) {
        remote.findRandomTracks(TracksCacher(visitor))
    } else {
        db.queryRandomTracks(visitor)
    }

    private inner class TracksCacher(private val delegate: TracksVisitor) : TracksVisitor {
        override fun accept(obj: Track) {
            db.addTrack(obj)
            delegate.accept(obj)
        }
    }
}
