/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxart

import app.zxtune.TimeStamp
import app.zxtune.fs.dbhelpers.CommandExecutor
import app.zxtune.fs.dbhelpers.QueryCommand

private val AUTHORS_TTL = TimeStamp.fromDays(7)
private val PARTIES_TTL = TimeStamp.fromDays(7)
private val TRACKS_TTL = TimeStamp.fromDays(1)

class CachingCatalog internal constructor(
    private val remote: RemoteCatalog,
    private val db: Database
) : Catalog {
    private val executor: CommandExecutor = CommandExecutor("zxart")

    override fun queryAuthors(visitor: Catalog.Visitor<Author>) =
        executor.executeQuery("authors", object : QueryCommand {
            private val lifetime = db.getAuthorsLifetime(AUTHORS_TTL)

            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryAuthors(db::addAuthor)
                lifetime.update()
            }

            override fun queryFromCache() = db.queryAuthors(visitor)
        })

    override fun queryAuthorTracks(author: Author, visitor: Catalog.Visitor<Track>) =
        executor.executeQuery("tracks", object : QueryCommand {
            private val lifetime = db.getAuthorTracksLifetime(author, TRACKS_TTL)

            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryAuthorTracks(author) { obj ->
                    db.addTrack(obj)
                    db.addAuthorTrack(author, obj)
                }
                lifetime.update()
            }

            override fun queryFromCache() = db.queryAuthorTracks(author, visitor)
        })

    override fun queryParties(visitor: Catalog.Visitor<Party>) =
        executor.executeQuery("parties", object : QueryCommand {
            private val lifetime = db.getPartiesLifetime(PARTIES_TTL)

            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryParties(db::addParty)
                lifetime.update()
            }

            override fun queryFromCache() = db.queryParties(visitor)
        })

    override fun queryPartyTracks(party: Party, visitor: Catalog.Visitor<Track>) =
        executor.executeQuery("tracks", object : QueryCommand {
            private val lifetime = db.getPartyTracksLifetime(party, TRACKS_TTL)

            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryPartyTracks(party) { obj ->
                    db.addTrack(obj)
                    db.addPartyTrack(party, obj)
                }
                lifetime.update()
            }

            override fun queryFromCache() = db.queryPartyTracks(party, visitor)
        })

    override fun queryTopTracks(limit: Int, visitor: Catalog.Visitor<Track>) =
        executor.executeQuery("tracks", object : QueryCommand {
            private val lifetime = db.getTopLifetime(TRACKS_TTL)

            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryTopTracks(limit, db::addTrack)
                lifetime.update()
            }

            override fun queryFromCache() = db.queryTopTracks(limit, visitor)
        })

    override fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) =
        if (remote.searchSupported()) {
            remote.findTracks(query, visitor)
        } else {
            db.findTracks(query, visitor)
        }
}
