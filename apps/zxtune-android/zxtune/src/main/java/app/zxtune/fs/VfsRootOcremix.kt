package app.zxtune.fs

import android.content.Context
import android.net.Uri
import app.zxtune.R
import app.zxtune.Util
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.ocremix.Album
import app.zxtune.fs.ocremix.Catalog
import app.zxtune.fs.ocremix.FilePath
import app.zxtune.fs.ocremix.Game
import app.zxtune.fs.ocremix.Identifier
import app.zxtune.fs.ocremix.Organization
import app.zxtune.fs.ocremix.Remix
import app.zxtune.fs.ocremix.RemoteCatalog
import app.zxtune.fs.ocremix.System

class VfsRootOcremix(
    override val parent: VfsObject, private val context: Context, http: MultisourceHttpProvider
) : StubObject(), VfsRoot {
    private val catalog = Catalog.create(context, http)
    private val roots by lazy {
        HashMap<Identifier.AggregateType, VfsDir>().apply {
            Identifier.AggregateType.entries.forEach {
                put(it, AggregateDir(arrayOf(Identifier.AggregateElement(it)), it))
            }
        }
    }

    override val uri: Uri
        get() = Identifier.forRoot().build()
    override val name
        get() = context.getString(R.string.vfs_ocremix_root_name)
    override val description
        get() = context.getString(R.string.vfs_ocremix_root_description)

    override fun getExtension(id: String) = when (id) {
        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_ocremix
        else -> super.getExtension(id)
    }

    override fun enumerate(visitor: VfsDir.Visitor) = roots.forEach { entry ->
        visitor.onDir(entry.value)
    }

    override fun resolve(uri: Uri): VfsObject? {
        if (!Identifier.isFromRoot(uri)) {
            return null
        }
        Identifier.findImagePath(uri)?.let {
            return File(arrayOf(Identifier.FileElement(it)), it, "")
        }
        return resolveChain(Identifier.findElementsChain(uri))
    }

    private fun resolveChain(chain: Array<Identifier.PathElement>) = when {
        chain.isEmpty() -> this@VfsRootOcremix
        chain.size == 1 -> (chain.last() as? Identifier.AggregateElement)?.let {
            roots[it.aggregate]
        }

        else -> when (val tail = chain.last()) {
            is Identifier.AggregateElement -> AggregateDir(chain, tail.aggregate)
            is Identifier.RemixElement -> RemixFile(chain, tail.remix)
            is Identifier.FileElement -> File(chain, tail.path, "")
            else -> EntityDir(chain)
        }
    }

    private open inner class BaseObject(protected val chain: Array<Identifier.PathElement>) :
        StubObject() {
        init {
            check(chain.isNotEmpty())
        }

        protected val element
            get() = chain.last()
        override val uri: Uri
            get() = Identifier.forElementsChain(chain)
        override val name
            get() = element.title
        override val parent
            get() = resolveChain(chain.sliceArray(0..<chain.size - 1))
    }


    private inner class AggregateDir(
        chain: Array<Identifier.PathElement>,
        private val aggregate: Identifier.AggregateType,
    ) : BaseObject(chain), VfsDir {
        override val name
            get() = aggregate.name // TODO: localize

        override fun enumerate(visitor: VfsDir.Visitor) = when (aggregate) {
            Identifier.AggregateType.Systems -> querySystems(visitor)
            Identifier.AggregateType.Organizations -> queryOrganizations(visitor)
            Identifier.AggregateType.Games -> queryGames(visitor)
            Identifier.AggregateType.Remixes -> queryRemixes(visitor)
            Identifier.AggregateType.Albums -> queryAlbums(visitor)
        }

        private val scope
            get() = when (val ent = chain.getOrNull(chain.size - 2)) {
                is Identifier.SystemElement -> Catalog.SystemScope(ent.sys.id)
                is Identifier.OrganizationElement -> Catalog.OrganizationScope(ent.org.id)
                is Identifier.GameElement -> Catalog.GameScope(ent.game.id)
                else -> null
            }

        private fun child(entity: Identifier.PathElement, description: String = "") =
            EntityDir(chain + entity, description)

        private fun querySystems(visitor: VfsDir.Visitor) =
            catalog.querySystems(object : Catalog.Visitor<System> {
                override fun setCountHint(count: Int) = visitor.onItemsCount(count)
                override fun accept(obj: System) =
                    visitor.onDir(child(Identifier.SystemElement(obj)))
            })

        private fun queryOrganizations(visitor: VfsDir.Visitor) =
            catalog.queryOrganizations(object : Catalog.Visitor<Organization> {
                override fun setCountHint(count: Int) = visitor.onItemsCount(count)
                override fun accept(obj: Organization) =
                    visitor.onDir(child(Identifier.OrganizationElement(obj)))
            }, visitor)

        private fun queryGames(visitor: VfsDir.Visitor) =
            catalog.queryGames(scope, object : Catalog.GamesVisitor {
                override fun setCountHint(count: Int) = visitor.onItemsCount(count)
                override fun accept(
                    game: Game, system: System, organization: Organization?
                ) = visitor.onDir(
                    child(
                        Identifier.GameElement(game), "${system.title}/${organization?.title ?: ""}"
                    )
                )
            }, visitor)

        private fun queryRemixes(visitor: VfsDir.Visitor) =
            catalog.queryRemixes(scope, object : Catalog.RemixesVisitor {
                override fun setCountHint(count: Int) = visitor.onItemsCount(count)
                override fun accept(remix: Remix, game: Game) = visitor.onFile(
                    RemixFile(
                        chain + Identifier.RemixElement(remix), remix, game.title
                    )
                )
            }, visitor)

        private fun queryAlbums(visitor: VfsDir.Visitor) =
            catalog.queryAlbums(scope, object : Catalog.AlbumsVisitor {
                override fun setCountHint(count: Int) = visitor.onItemsCount(count)
                override fun accept(album: Album, image: FilePath?) =
                    visitor.onDir(child(Identifier.AlbumElement(album)))
            }, visitor)
    }

    private inner class EntityDir(
        chain: Array<Identifier.PathElement>, override val description: String = ""
    ) : BaseObject(chain), VfsDir {
        init {
            check(chain.size >= 2)
            check(chain[chain.size - 2] is Identifier.AggregateElement)
        }

        private val gameDetails by lazy {
            catalog.queryGameDetails((element as Identifier.GameElement).game.id)
        }

        override fun enumerate(visitor: VfsDir.Visitor) = when (val el = element) {
            is Identifier.SystemElement, is Identifier.OrganizationElement -> visitor.run {
                onDir(child(Identifier.AggregateType.Remixes))
                onDir(child(Identifier.AggregateType.Albums))
                onDir(child(Identifier.AggregateType.Games))
            }

            is Identifier.GameElement -> visitor.run {
                gameDetails.chiptunePath?.let { filePath ->
                    onFile(File(chain + Identifier.FileElement(filePath), filePath, ""))
                }
                onDir(child(Identifier.AggregateType.Remixes))
                onDir(child(Identifier.AggregateType.Albums))
            }

            is Identifier.AlbumElement -> catalog.queryAlbumTracks(
                el.album.id,
                object : Catalog.AlbumTracksVisitor {
                    override fun accept(filePath: FilePath, size: Long) = visitor.onFile(
                        File(
                            chain + Identifier.FileElement(filePath),
                            filePath,
                            Util.formatSize(size)
                        )
                    )
                })

            else -> Unit
        }

        private fun child(type: Identifier.AggregateType) =
            AggregateDir(chain + Identifier.AggregateElement(type), type)

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.COVER_ART_URI -> coverArtUri()
            else -> super.getExtension(id)
        }

        private fun coverArtUri() = when (val el = element) {
            is Identifier.GameElement -> gameDetails.image?.let { Identifier.forImage(it) }
            is Identifier.AlbumElement -> catalog.queryAlbumImage(el.album.id)?.let {
                Identifier.forImage(it)
            }

            else -> null
        }
    }

    private inner class RemixFile(
        chain: Array<Identifier.PathElement>,
        private val remix: Remix,
        override val description: String = "",
    ) : BaseObject(chain), VfsFile {
        private val path by lazy {
            catalog.findRemixPath(remix.id)
        }
        override val size = ""

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.DOWNLOAD_URIS -> path?.let {
                RemoteCatalog.getRemoteUris(it)
            }

            // do not imply findRemixPath for cache availability check
            VfsExtensions.CACHE_PATH -> remix.id.value
            else -> super.getExtension(id)
        }
    }

    private inner class File(
        chain: Array<Identifier.PathElement>, private val path: FilePath, override val size: String
    ) : BaseObject(chain), VfsFile {
        override fun getExtension(id: String) = when (id) {
            VfsExtensions.DOWNLOAD_URIS -> RemoteCatalog.getRemoteUris(path)
            VfsExtensions.CACHE_PATH -> path.value
            VfsExtensions.COVER_ART_URI -> findCoverart()
            else -> super.getExtension(id)
        }

        private fun findCoverart(): Uri? {
            for (idx in chain.indices.reversed()) {
                when (val el = chain[idx]) {
                    is Identifier.GameElement -> catalog.queryGameDetails(
                        el.game.id
                    ).image?.let {
                        return Identifier.forImage(it)
                    }

                    is Identifier.AlbumElement -> catalog.queryAlbumImage(el.album.id)?.let {
                        return Identifier.forImage(it)
                    }

                    else -> Unit
                }
            }
            return null
        }
    }
}
