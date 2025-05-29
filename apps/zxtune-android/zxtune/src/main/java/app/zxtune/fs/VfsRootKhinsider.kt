package app.zxtune.fs

import android.content.Context
import android.net.Uri
import androidx.annotation.StringRes
import app.zxtune.Logger
import app.zxtune.R
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.khinsider.AlbumAndDetails
import app.zxtune.fs.khinsider.Catalog
import app.zxtune.fs.khinsider.FilePath
import app.zxtune.fs.khinsider.Identifier
import app.zxtune.fs.khinsider.RemoteCatalog
import app.zxtune.fs.khinsider.Scope
import app.zxtune.fs.khinsider.TrackAndDetails

class VfsRootKhinsider(
    override val parent: VfsObject, private val context: Context, http: MultisourceHttpProvider
) : StubObject(), VfsRoot {
    private val catalog = Catalog.create(context, http)

    override val uri
        get() = Identifier().toUri()
    override val name
        get() = context.getString(R.string.vfs_khinsider_root_name)
    override val description
        get() = context.getString(R.string.vfs_khinsider_root_description)

    override fun resolve(uri: Uri) = Identifier.find(uri)?.let {
        resolve(it)
    }

    private fun resolve(id: Identifier) = when {
        id.location != null && true == id.category?.isImage -> ImageFile(id)
        id.track != null && id.album != null -> TrackFile(id, null)
        id.album != null -> AlbumDir(id, null)
        id.scope != null -> ScopeDir(id)
        id.category != null -> CategoryDir(id)
        else -> this@VfsRootKhinsider
    }

    override fun enumerate(visitor: VfsDir.Visitor) = Identifier.Category.entries.forEach {
        if (!it.isImage) {
            visitor.onDir(CategoryDir(Identifier(category = it)))
        }
    }

    override fun getExtension(id: String) = when (id) {
        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_khinsider
        else -> super.getExtension(id)
    }

    abstract inner class BaseObject(protected var id: Identifier) : StubObject() {
        override val uri
            get() = id.toUri()
        override val parent
            get() = when {
                id.track != null -> id.copy(track = null)
                id.album != null -> id.copy(album = null)
                id.scope != null -> id.copy(scope = null)
                else -> Identifier()
            }.let {
                resolve(it)
            }
        override val name
            get() = id.location?.run {
                value.lastPathSegment
            } ?: id.track?.run {
                title
            } ?: id.album?.run {
                title
            } ?: id.scope?.run {
                title
            } ?: id.category?.run {
                if (localized != 0) context.getString(localized) else name
            } ?: ""
    }

    inner class CategoryDir(id: Identifier) : BaseObject(id), VfsDir {
        private val category
            get() = requireNotNull(id.category)
        private val isRandom
            get() = Identifier.Category.Random == category

        override fun enumerate(visitor: VfsDir.Visitor) = when (category) {
            Identifier.Category.Top -> enumerateTops(visitor)
            Identifier.Category.Series -> enumerateScopes(Catalog.ScopeType.SERIES, visitor)
            Identifier.Category.Platforms -> enumerateScopes(Catalog.ScopeType.PLATFORMS, visitor)
            Identifier.Category.Types -> enumerateScopes(Catalog.ScopeType.TYPES, visitor)
            Identifier.Category.AlbumsByLetter -> enumerateAlbumLetters(visitor)
            Identifier.Category.AlbumsByYear -> enumerateScopes(Catalog.ScopeType.YEARS, visitor)
            else -> Unit
        }

        private fun enumerateTops(visitor: VfsDir.Visitor) {
            visitor.onDir(makeChild(Catalog.NEW100, R.string.vfs_khinsider_top_new100_name))
            visitor.onDir(makeChild(Catalog.WEEKLYTOP40, R.string.vfs_khinsider_top_40_name))
            visitor.onDir(makeChild(Catalog.FAVORITES, R.string.vfs_khinsider_top_favorites_name))
        }

        private fun enumerateScopes(type: Catalog.ScopeType, visitor: VfsDir.Visitor) =
            catalog.queryScopes(type) { visitor.onDir(makeChild(it)) }

        private fun enumerateAlbumLetters(visitor: VfsDir.Visitor) {
            for (letter in 'A'..'Z') {
                visitor.onDir(makeChild(Catalog.letterScope(letter)))
            }
            visitor.onDir(makeChild(Catalog.nonLetterScope()))
        }

        private fun makeChild(scope: Scope) = ScopeDir(id.copy(scope = scope))
        private fun makeChild(id: Scope.Id, @StringRes name: Int) =
            makeChild(Scope(id, context.getString(name)))

        override fun getExtension(id: String) = if (isRandom) {
            when (id) {
                VfsExtensions.FEED -> FeedIterator(this.id)
                VfsExtensions.ICON -> R.drawable.ic_browser_vfs_radio
                else -> super.getExtension(id)
            }
        } else {
            super.getExtension(id)
        }
    }

    private inner class FeedIterator(private val parentId: Identifier) : Iterator<VfsFile> {
        private var track: TrackAndDetails? = null

        override fun hasNext(): Boolean {
            if (track == null) {
                repeat(5) {
                    track = loadRandomTracks().getOrNull()
                    if (track != null) {
                        return true
                    }
                }
                LOG.d { "Failed to find next random track after all tries" }
            }
            return true
        }

        override fun next(): VfsFile = requireNotNull(track).also { track = null }.run {
            TrackFile(parentId.copy(album = album, track = track), this)
        }

        private fun loadRandomTracks() = runCatching {
            ArrayList<TrackAndDetails>().apply {
                catalog.queryAlbumDetails(Catalog.RANDOM_ALBUM, this::add)
            }.randomOrNull()
        }.onFailure { err ->
            LOG.w(err) { "Failed to get next track" }
        }
    }

    private inner class ScopeDir(id: Identifier) : BaseObject(id), VfsDir {
        private val scope
            get() = requireNotNull(id.scope)

        override fun enumerate(visitor: VfsDir.Visitor) = catalog.queryAlbums(scope.id, { album ->
            visitor.onDir(AlbumDir(id.copy(album = album.album), album))
        }, visitor)
    }

    private inner class AlbumDir(id: Identifier, private var details: AlbumAndDetails?) :
        BaseObject(id), VfsDir {
        private val album
            get() = requireNotNull(id.album)

        override val description
            get() = details?.details ?: ""

        override fun enumerate(visitor: VfsDir.Visitor) {
            details = catalog.queryAlbumDetails(album.id) { track ->
                visitor.onFile(TrackFile(id.copy(album = track.album, track = track.track), track))
            }
            checkAlbumAlias()
        }

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.COVER_ART_URI -> imageUriFor(Identifier.Category.Image)
            VfsExtensions.ICON_URI -> imageUriFor(Identifier.Category.Thumb)
            VfsExtensions.COMPARATOR -> TracksComparator
            else -> super.getExtension(id)
        }

        private fun imageUriFor(category: Identifier.Category) =
            (details ?: catalog.queryAlbumDetails(album.id) {}.also { details = it })?.image?.let {
                checkAlbumAlias()
                Identifier(category = category, location = it).toUri()
            }

        private fun checkAlbumAlias() = details?.album?.let { realAlbum ->
            if (realAlbum.id != album.id) {
                LOG.d { "Detected alias $album -> $realAlbum" }
                id = id.copy(album = realAlbum)
            }
        }
    }

    private object TracksComparator : Comparator<VfsObject> {
        override fun compare(lh: VfsObject, rh: VfsObject): Int {
            val lhIdx = (lh as TrackFile).details?.index ?: -2
            val rhIdx = (rh as TrackFile).details?.index ?: -1
            return lhIdx.compareTo(rhIdx)
        }
    }

    private inner class TrackFile(
        id: Identifier, val details: TrackAndDetails? = null
    ) : BaseObject(id), VfsFile {
        private val album
            get() = requireNotNull(id.album)
        private val track
            get() = requireNotNull(id.track)
        override val description
            get() = details?.size ?: ""
        override val size
            get() = details?.duration ?: ""

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.DOWNLOAD_URIS -> downloadUris
            VfsExtensions.CACHE_PATH -> "${album.id.value}/${track.id.value}"
            else -> super.getExtension(id)
        }

        private val downloadUris
            get() = iterator {
                yield(RemoteCatalog.getRemoteUri(album.id, track.id))
                catalog.findTrackLocation(album.id, track.id)?.let {
                    yield(it)
                }
            }
    }

    private inner class ImageFile(id: Identifier) : BaseObject(id), VfsFile {
        private val location = requireNotNull(id.location)
        override val size
            get() = ""

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.DOWNLOAD_URIS -> downloadUris
            else -> super.getExtension(id)
        }

        private val downloadUris
            get() = when (id.category) {
                Identifier.Category.Image -> RemoteCatalog.getRemoteUris(location)

                Identifier.Category.Thumb -> location.thumbUri?.let {
                    RemoteCatalog.getRemoteUris(FilePath(it))
                }

                else -> null
            }
    }

    companion object {
        private val LOG = Logger(VfsRootKhinsider::class.java.name)
    }
}
