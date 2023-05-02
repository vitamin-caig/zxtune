package app.zxtune.fs.vgmrips

import app.zxtune.TimeStamp.Companion.fromDays
import app.zxtune.fs.dbhelpers.CommandExecutor
import app.zxtune.fs.dbhelpers.QueryCommand
import app.zxtune.utils.ProgressCallback

private val GROUPS_TTL = fromDays(1)
private val GROUP_PACKS_TTL = fromDays(1)
private val PACK_TRACKS_TTL = fromDays(30)

internal class CachingCatalog(private val remote: RemoteCatalog, private val db: Database) :
    Catalog {

    private val executor: CommandExecutor = CommandExecutor("vgmrips")

    // TODO: lazy
    override fun companies(): Catalog.Grouping =
        CachedGrouping(Database.TYPE_COMPANY, "company", remote.companies())

    override fun composers(): Catalog.Grouping =
        CachedGrouping(Database.TYPE_COMPOSER, "composer", remote.composers())

    override fun chips(): Catalog.Grouping =
        CachedGrouping(Database.TYPE_CHIP, "chip", remote.chips())

    override fun systems(): Catalog.Grouping =
        CachedGrouping(Database.TYPE_SYSTEM, "system", remote.systems())

    override fun findPack(id: String, visitor: Catalog.Visitor<Track>): Pack? {
        var result: Pack? = null
        executor.executeQuery("pack", object : QueryCommand {
            private val lifetime = db.getLifetime(id, PACK_TRACKS_TTL)

            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                result = remote.findPack(id) { obj ->
                    db.addPackTrack(id, obj)
                }?.also {
                    db.addPack(it)
                }
                lifetime.update()
            }

            // TODO: improve tests to not ignore remote query result if cache doesn't store anything
            override fun queryFromCache(): Boolean {
                result = db.queryPack(id)
                return result?.run {
                    db.queryPackTracks(id, visitor)
                } ?: false
            }
        })
        return result
    }

    override fun findRandomPack(visitor: Catalog.Visitor<Track>) = if (remote.isAvailable()) {
        findRandomPackAndCache(visitor)
    } else {
        findRandomPackFromCache(visitor)
    }

    // TODO: simplify - transaction after remote query
    // TODO: return null if no tracks?
    private fun findRandomPackAndCache(visitor: Catalog.Visitor<Track>): Pack? {
        var res: Pack? = null
        db.runInTransaction {
            val tracks = ArrayList<Track>()
            remote.findRandomPack(tracks::add)?.let {
                res = it
                db.addPack(it)
                for (tr in tracks) {
                    db.addPackTrack(it.id, tr)
                    visitor.accept(tr)
                }
            }
        }
        return res
    }

    private fun findRandomPackFromCache(visitor: Catalog.Visitor<Track>) =
        db.queryRandomPack()?.apply {
            db.queryPackTracks(id, visitor)
        }

    private inner class CachedGrouping constructor(
        @Database.Type private val type: Int,
        private val scope: String,
        private val remote: Catalog.Grouping
    ) : Catalog.Grouping {
        override fun query(visitor: Catalog.Visitor<Group>) =
            executor.executeQuery(scope + "s", object : QueryCommand {
                private val lifetime = db.getLifetime(scope, GROUPS_TTL)

                override val isCacheExpired: Boolean
                    get() = lifetime.isExpired

                override fun updateCache() = db.runInTransaction {
                    remote.query { obj ->
                        db.addGroup(type, obj)
                    }
                    lifetime.update()
                }

                override fun queryFromCache() = db.queryGroups(type, visitor)
            })

        override fun queryPacks(
            id: String,
            visitor: Catalog.Visitor<Pack>,
            progress: ProgressCallback
        ) = executor.executeQuery(scope, object : QueryCommand {
            private val lifetime = db.getLifetime(id, GROUP_PACKS_TTL)

            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() = db.runInTransaction {
                remote.queryPacks(id, { obj ->
                    db.addGroupPack(id, obj)
                }, progress)
                lifetime.update()
            }

            override fun queryFromCache() = db.queryGroupPacks(id, visitor)
        })
    }
}
