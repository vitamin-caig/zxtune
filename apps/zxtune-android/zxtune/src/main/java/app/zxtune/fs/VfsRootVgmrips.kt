package app.zxtune.fs

import android.content.Context
import android.net.Uri
import androidx.annotation.DrawableRes
import androidx.annotation.StringRes
import app.zxtune.Logger
import app.zxtune.R
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.vgmrips.Catalog
import app.zxtune.fs.vgmrips.FilePath
import app.zxtune.fs.vgmrips.Group
import app.zxtune.fs.vgmrips.Identifier
import app.zxtune.fs.vgmrips.Pack
import app.zxtune.fs.vgmrips.RemoteCatalog

class VfsRootVgmrips(
    override val parent: VfsObject,
    private val context: Context,
    http: MultisourceHttpProvider,
) : StubObject(), VfsRoot {
    private val catalog = Catalog.create(context, http)
    private val randomTracks by lazy {
        RandomDir()
    }
    private val groupings by lazy {
        arrayOf<GroupingDir>(
            CompaniesDir(),
            ComposersDir(),
            ChipsDir(),
            SystemsDir(),
        )
    }

    override val uri: Uri
        get() = Identifier.forRoot().build()
    override val name
        get() = context.getString(R.string.vfs_vgmrips_root_name)
    override val description
        get() = context.getString(R.string.vfs_vgmrips_root_description)

    override fun getExtension(id: String) = when (id) {
        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_vgmrips
        else -> super.getExtension(id)
    }

    override fun enumerate(visitor: VfsDir.Visitor) = groupings.forEach { entry ->
        visitor.onDir(entry)
    }.also {
        visitor.onDir(randomTracks)
    }

    override fun resolve(uri: Uri) = if (Identifier.isFromRoot(uri)) {
        Identifier.findTrack(uri)?.run {
            TrackFile(uri, lazy { catalog.findPack(first) }, second)
        } ?: resolve(uri, uri.pathSegments)
    } else {
        null
    }

    private fun resolve(uri: Uri, path: List<String>): VfsObject? {
        val category = Identifier.findCategory(path) ?: return this
        // compatibility
        if (category == Identifier.CATEGORY_IMAGE) {
            return resolveImage(uri)
        }
        if (category == Identifier.CATEGORY_RANDOM) {
            return randomTracks
        }
        val grouping = groupings.find { it.category == category } ?: return null
        val group = Identifier.findGroup(uri, path) ?: return grouping
        val groupDir = grouping.makeChild(group)
        return Identifier.findPack(uri, path)?.let { pack ->
            PackFile(groupDir, pack, lazy { catalog.findPack(pack.id)?.image })
        } ?: groupDir
    }

    private fun resolveImage(uri: Uri) = Identifier.findImage(uri)?.let {
        ImageFile(it)
    }

    private interface ParentDir : VfsDir {
        fun makeUri(): Uri.Builder
    }

    private abstract inner class GroupingDir(
        val category: String,
        val grouping: Catalog.Grouping,
        @StringRes private val nameRes: Int,
        @DrawableRes private val iconRes: Int
    ) : StubObject(), ParentDir {
        override val uri: Uri
            get() = makeUri().build()
        override val name
            get() = context.getString(nameRes)
        override val parent
            get() = this@VfsRootVgmrips

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.ICON -> iconRes
            else -> super.getExtension(id)
        }

        override fun enumerate(visitor: VfsDir.Visitor) =
            grouping.query(Catalog.Visitor { obj -> visitor.onDir(makeChild(obj)) })

        fun makeChild(obj: Group) = GroupDir(this, obj)

        override fun makeUri() = Identifier.forCategory(category)
    }

    private inner class CompaniesDir : GroupingDir(
        Identifier.CATEGORY_COMPANY,
        catalog.companies(),
        R.string.vfs_vgmrips_companies_name,
        R.drawable.ic_browser_vfs_vgmrips_companies
    )

    private inner class ComposersDir : GroupingDir(
        Identifier.CATEGORY_COMPOSER,
        catalog.composers(),
        R.string.vfs_vgmrips_composers_name,
        R.drawable.ic_browser_vfs_vgmrips_composers
    )

    private inner class ChipsDir : GroupingDir(
        Identifier.CATEGORY_CHIP,
        catalog.chips(),
        R.string.vfs_vgmrips_chips_name,
        R.drawable.ic_browser_vfs_vgmrips_chips
    )

    private inner class SystemsDir : GroupingDir(
        Identifier.CATEGORY_SYSTEM,
        catalog.systems(),
        R.string.vfs_vgmrips_systems_name,
        R.drawable.ic_browser_vfs_vgmrips_systems
    )

    private inner class RandomDir : StubObject(), ParentDir {
        override val uri: Uri
            get() = makeUri().build()
        override val name
            get() = context.getString(R.string.vfs_vgmrips_random_name)
        override val parent
            get() = this@VfsRootVgmrips

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.ICON -> R.drawable.ic_browser_vfs_radio
            VfsExtensions.FEED -> FeedIterator()
            else -> super.getExtension(id)
        }

        override fun enumerate(visitor: VfsDir.Visitor) = Unit

        override fun makeUri() = Identifier.forCategory(Identifier.CATEGORY_RANDOM)
    }

    private inner class FeedIterator : Iterator<VfsFile> {
        private var track: Pair<Pack, FilePath>? = null

        override fun hasNext(): Boolean {
            if (track == null) {
                repeat(5) {
                    track = loadRandomTrack().getOrNull()
                    if (track != null) {
                        return true
                    }
                }
                LOG.d { "Failed to find next random track" }
            }
            return true
        }

        override fun next(): VfsFile =
            requireNotNull(track).also { track = null }.let { (pack, track) ->
                TrackFile(
                    Identifier.forRandomTrack(pack.id, track), lazyOf(pack), track
                )
            }

        fun loadRandomTrack() = runCatching {
            val tracks = ArrayList<FilePath>()
            catalog.findRandomPack(tracks::add)?.let { pack ->
                tracks.randomOrNull()?.let { track ->
                    pack to track
                }
            }
        }.onFailure { e ->
            LOG.w(e) { "Failed to get next track" }
        }
    }

    private inner class GroupDir(
        override val parent: GroupingDir,
        private val group: Group,
    ) : StubObject(), ParentDir {
        override val uri: Uri
            get() = makeUri().build()
        override val name
            get() = group.title
        override val description
            get() = context.resources.getQuantityString(R.plurals.packs, group.packs, group.packs)

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.ICON_URI -> group.image?.let {
                Identifier.forImage(it)
            }

            else -> super.getExtension(id)
        }

        override fun enumerate(visitor: VfsDir.Visitor) = parent.grouping.queryPacks(
            group.id,
            { obj -> visitor.onFile(PackFile(this@GroupDir, obj, lazyOf(obj.image))) },
            visitor
        )

        override fun makeUri() = Identifier.forGroup(parent.makeUri(), group)
    }

    private inner class PackFile(
        override val parent: ParentDir, private val pack: Pack, private val image: Lazy<FilePath?>
    ) : StubObject(), VfsFile {
        override val uri: Uri
            get() = makeUri().build()
        override val name
            get() = pack.title
        override val description
            get() = context.resources.getQuantityString(R.plurals.tracks, pack.songs, pack.songs)
        override val size
            get() = pack.size

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.COVER_ART_URI -> image.value?.let { Identifier.forImage(FilePath("images/large/${it.value}")) }
            VfsExtensions.ICON_URI -> image.value?.let { Identifier.forImage(FilePath("images/small/${it.value}")) }
            VfsExtensions.DOWNLOAD_URIS -> RemoteCatalog.getRemoteUris(pack.archive)
            VfsExtensions.CACHE_PATH -> pack.archive.value
            else -> super.getExtension(id)
        }

        fun makeUri() = Identifier.forPack(parent.makeUri(), pack)
    }

    private inner class TrackFile(
        override val uri: Uri, private val pack: Lazy<Pack?>, private val track: FilePath
    ) : StubObject(), VfsFile {
        override val name
            get() = track.displayName
        override val size
            get() = ""
        override val parent
            get() = resolve(uri, uri.pathSegments)

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.COVER_ART_URI -> pack.value?.image?.let { Identifier.forImage(FilePath("images/large/${it.value}")) }
            VfsExtensions.DOWNLOAD_URIS -> RemoteCatalog.getRemoteUris(track)
            VfsExtensions.CACHE_PATH -> track.value
            else -> super.getExtension(id)
        }
    }

    private class ImageFile(private val image: FilePath) : StubObject(), VfsFile {
        override val name
            get() = image.displayName
        override val uri: Uri
            get() = Identifier.forImage(image)
        override val size
            get() = ""
        override val parent
            get() = null

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.DOWNLOAD_URIS -> RemoteCatalog.getRemoteUris(image)
            else -> super.getExtension(id)
        }
    }

    companion object {
        private val LOG = Logger(VfsRootVgmrips::class.java.name)
    }
}
