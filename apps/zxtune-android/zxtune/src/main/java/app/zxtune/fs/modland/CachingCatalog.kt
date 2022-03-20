/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.modland

import app.zxtune.TimeStamp.Companion.fromDays
import app.zxtune.fs.dbhelpers.CommandExecutor
import app.zxtune.fs.dbhelpers.FetchCommand
import app.zxtune.fs.dbhelpers.QueryCommand
import app.zxtune.utils.ProgressCallback
import app.zxtune.utils.StubProgressCallback
import java.io.IOException

private val GROUPS_TTL = fromDays(30)
private val GROUP_TRACKS_TTL = fromDays(14)

class CachingCatalog internal constructor(remote: RemoteCatalog, private val db: Database) :
    Catalog {
    // TODO: lazy
    private val authors = CachedGrouping(Database.Tables.Authors.NAME, remote.getAuthors())
    private val collections =
        CachedGrouping(Database.Tables.Collections.NAME, remote.getCollections())
    private val formats = CachedGrouping(Database.Tables.Formats.NAME, remote.getFormats())
    private val executor = CommandExecutor("modland")

    override fun getAuthors(): Catalog.Grouping = authors

    override fun getCollections(): Catalog.Grouping = collections

    override fun getFormats(): Catalog.Grouping = formats

    private inner class CachedGrouping(
        private val category: String,
        private val remote: Catalog.Grouping
    ) : Catalog.Grouping {

        override fun queryGroups(
            filter: String,
            visitor: Catalog.GroupsVisitor,
            progress: ProgressCallback
        ) = executor.executeQuery(category, object : QueryCommand {
            private val lifetime = db.getGroupsLifetime(category, filter, GROUPS_TTL)
            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryGroups(
                    filter, { obj -> db.addGroup(category, obj) },
                    progress
                )
                lifetime.update()
            }

            override fun queryFromCache() = db.queryGroups(category, filter, visitor)
        })

        // It's impossible to fill all the cache, so query/update for specified group
        override fun getGroup(id: Int): Group =
            executor.executeFetchCommand(category.dropLast(1), object : FetchCommand<Group> {
                override fun fetchFromCache() = db.queryGroup(category, id)

                override fun updateCache() = remote.getGroup(id).also { db.addGroup(category, it) }
            })

        override fun queryTracks(
            id: Int,
            visitor: Catalog.TracksVisitor,
            progress: ProgressCallback
        ) = executor.executeQuery("tracks", object : QueryCommand {
            private val lifetime = db.getGroupTracksLifetime(category, id, GROUP_TRACKS_TTL)
            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryTracks(id, { obj ->
                    db.addTrack(obj)
                    db.addGroupTrack(category, id, obj)
                    true
                }, progress)
                lifetime.update()
            }

            override fun queryFromCache() = db.queryTracks(category, id, visitor)
        })

        // Just query all the category tracks and store found one
        override fun getTrack(id: Int, filename: String): Track =
            executor.executeFetchCommand("track", object : FetchCommand<Track> {
                override fun fetchFromCache() = db.findTrack(category, id, filename)

                override fun updateCache(): Track {
                    var found: Track? = null
                    queryTracks(id, { obj ->
                        if (obj.filename == filename) {
                            found = obj
                        }
                        true
                    }, StubProgressCallback.instance())
                    return found
                        ?: throw IOException("Failed to get track '${filename}' from group=${id}")
                }
            })
    }
}
