/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.amp

import app.zxtune.TimeStamp
import app.zxtune.fs.dbhelpers.CommandExecutor
import app.zxtune.fs.dbhelpers.QueryCommand

private val GROUPS_TTL = TimeStamp.fromDays(30)
private val AUTHORS_TTL = TimeStamp.fromDays(30)
private val TRACKS_TTL = TimeStamp.fromDays(30)
private val PICTURES_TTL = TimeStamp.fromDays(30)

class CachingCatalog internal constructor(
    private val remote: RemoteCatalog,
    private val db: Database
) : Catalog {
    private val executor = CommandExecutor("amp")

    override fun queryGroups(visitor: Catalog.GroupsVisitor) =
        executor.executeQuery("groups", object : QueryCommand {
            private val lifetime = db.getGroupsLifetime(GROUPS_TTL)
            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryGroups(db::addGroup)
                lifetime.update()
            }

            override fun queryFromCache() = db.queryGroups(visitor)
        })

    override fun queryAuthors(handleFilter: String, visitor: Catalog.AuthorsVisitor) =
        executor.executeQuery("authors", object : QueryCommand {
            private val lifetime = db.getAuthorsLifetime(handleFilter, AUTHORS_TTL)
            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryAuthors(handleFilter, db::addAuthor)
                lifetime.update()
            }


            override fun queryFromCache() = db.queryAuthors(handleFilter, visitor)
        })

    override fun queryAuthors(country: Country, visitor: Catalog.AuthorsVisitor) =
        executor.executeQuery("authors", object : QueryCommand {
            private val lifetime = db.getCountryLifetime(country, AUTHORS_TTL)
            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() {
                db.runInTransaction {
                    remote.queryAuthors(country) {
                        db.addAuthor(it)
                        db.addCountryAuthor(country, it)
                    }
                    lifetime.update()
                }
            }

            override fun queryFromCache() = db.queryAuthors(country, visitor)
        })

    override fun queryAuthors(group: Group, visitor: Catalog.AuthorsVisitor) =
        executor.executeQuery("authors", object : QueryCommand {
            private val lifetime = db.getGroupLifetime(group, AUTHORS_TTL)
            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryAuthors(group) {
                    db.addAuthor(it)
                    db.addGroupAuthor(group, it)
                }
                lifetime.update()
            }

            override fun queryFromCache() = db.queryAuthors(group, visitor)
        })

    override fun queryTracks(author: Author, visitor: Catalog.TracksVisitor) =
        executor.executeQuery("tracks", object : QueryCommand {
            private val lifetime = db.getAuthorTracksLifetime(author, TRACKS_TTL)
            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryTracks(author) {
                    db.addTrack(it)
                    db.addAuthorTrack(author, it)
                }
                lifetime.update()
            }


            override fun queryFromCache() = db.queryTracks(author, visitor)
        })

    override fun queryPictures(author: Author, visitor: Catalog.PicturesVisitor) = executor
        .executeQuery("pictures", object : QueryCommand {
            private val lifetime = db.getAuthorPicturesLifetime(author, PICTURES_TTL)
            override val isCacheExpired
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryPictures(author) {
                    db.addAuthorPicture(author, it)
                }
                lifetime.update()
            }

            override fun queryFromCache() = db.queryPictures(author, visitor)
        })

    override fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) =
        if (remote.searchSupported()) {
            remote.findTracks(query, visitor)
        } else {
            db.findTracks(query, visitor)
        }
}
