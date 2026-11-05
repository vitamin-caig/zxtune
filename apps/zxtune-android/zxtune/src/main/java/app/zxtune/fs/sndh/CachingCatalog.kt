package app.zxtune.fs.sndh

import androidx.core.util.Consumer
import app.zxtune.TimeStamp.Companion.fromDays
import app.zxtune.fs.dbhelpers.CommandExecutor
import app.zxtune.fs.dbhelpers.QueryCommand
import java.io.IOException

private val AUTHORS_TTL = fromDays(30)
private val TRACKS_TTL = fromDays(2)

class CachingCatalog(private val remote: RemoteCatalog, private val db: Database) : Catalog {
    private val executor = CommandExecutor("sndh")

    override fun queryAuthors(visitor: Consumer<Author>) = executor.executeQuery("authors", object : QueryCommand {
        private val lifetime = db.getLifetime("authors", AUTHORS_TTL)

        override val isCacheExpired
            get() = lifetime.isExpired

        override fun updateCache() = db.runInTransaction {
            db.cleanupAuthors()
            remote.queryAuthors({ author ->
                db.addAuthor(author)
            })
            lifetime.update()
        }

        override fun queryFromCache() = db.queryAuthors(visitor)
    })

    override fun queryTracks(
        author: Author,
        visitor: Consumer<Track>,
    ) = executor.executeQuery("tracks", object : QueryCommand {
        private val lifetime = db.getLifetime("tracks:${author.name}", TRACKS_TTL)

        override val isCacheExpired
            get() = lifetime.isExpired

        override fun updateCache() = db.runInTransaction {
            db.cleanupTracks(author)
            remote.queryTracks(author, { track ->
                db.addTrack(author, track)
            })
            lifetime.update()
        }

        override fun queryFromCache() = db.queryTracks(author, visitor)
    })

    override fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) = if (remote.searchSupported()) {
        remote.findTracks(query, visitor)
    } else {
        db.findTracks(query, visitor)
    }

    override fun findRandomTrack(): Track? = try {
        remote.findRandomTrack()
    } catch (e: IOException) {
        null
    } ?: db.queryRandomTrack()
}
