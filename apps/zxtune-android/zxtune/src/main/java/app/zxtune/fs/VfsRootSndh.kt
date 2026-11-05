package app.zxtune.fs

import android.content.Context
import android.net.Uri
import app.zxtune.Logger
import app.zxtune.R
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.sndh.Author
import app.zxtune.fs.sndh.Catalog
import app.zxtune.fs.sndh.Identifier
import app.zxtune.fs.sndh.RemoteCatalog
import app.zxtune.fs.sndh.Track
import java.io.IOException

internal class VfsRootSndh(
    override val parent: VfsObject,
    private val context: Context,
    http: MultisourceHttpProvider,
) : StubObject(),
    VfsRoot {
    private val catalog = Catalog.create(context, http)

    override val uri
        get() = Identifier.forRoot().build()
    override val name
        get() = context.getString(R.string.vfs_sndh_root_name)
    override val description
        get() = context.getString(R.string.vfs_sndh_root_description)

    override fun enumerate(visitor: VfsDir.Visitor) {
        visitor.onDir(AuthorsDir())
        visitor.onDir(RandomDir())
    }

    override fun resolve(uri: Uri): VfsObject? {
        if (!Identifier.isFromRoot(uri)) {
            return null
        }
        val path = uri.pathSegments
        return Identifier.findTrack(uri, path)?.let {
            TrackFile(uri, it)
        } ?: Identifier.findAuthor(uri, path)?.let {
            AuthorDir(it)
        } ?: when {
            path.isEmpty() -> this
            path.size != 1 -> null
            Identifier.findCategory(path) == Identifier.CATEGORY_AUTHORS -> AuthorsDir()
            Identifier.findCategory(path) == Identifier.CATEGORY_RANDOM -> RandomDir()
            else -> null
        }
    }

    override fun getExtension(id: String) = when (id) {
        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_sndh
        else -> super.getExtension(id)
    }

    private inner class AuthorsDir :
        StubObject(),
        VfsDir {
        override val uri
            get() = Identifier.forCategory(Identifier.CATEGORY_AUTHORS).build()
        override val name
            get() = context.getString(R.string.vfs_sndh_authors_name)
        override val parent
            get() = this@VfsRootSndh

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.SEARCH_ENGINE -> AuthorsSearchEngine()
            else -> super.getExtension(id)
        }

        override fun enumerate(visitor: VfsDir.Visitor) = catalog.queryAuthors({ author ->
            visitor.onDir(AuthorDir(author))
        })
    }

    private inner class AuthorsSearchEngine : VfsExtensions.SearchEngine {
        override fun find(query: String, visitor: VfsExtensions.SearchEngine.Visitor) = catalog.findTracks(query) { author, track ->
            val uri = Identifier.forTrack(Identifier.forAuthor(author), track).build()
            visitor.onFile(TrackFile(uri, track))
        }
    }

    private inner class AuthorDir(private val author: Author) :
        StubObject(),
        VfsDir {
        override val uri
            get() = Identifier.forAuthor(author).build()
        override val name
            get() = author.name
        override val description
            get() = if (author.files > 0) {
                context.resources.getQuantityString(
                    R.plurals.files,
                    author.files,
                    author.files
                )
            } else {
                ""
            }
        override val parent
            get() = AuthorsDir()

        override fun enumerate(visitor: VfsDir.Visitor) = catalog.queryTracks(author, { track ->
            visitor.onFile(
                TrackFile(
                    Identifier.forTrack(Identifier.forAuthor(author), track).build(),
                    track
                )
            )
        })
    }

    private inner class RandomDir :
        StubObject(),
        VfsDir {
        override val uri
            get() = Identifier.forCategory(Identifier.CATEGORY_RANDOM).build()
        override val name
            get() = context.getString(R.string.vfs_sndh_random_name)
        override val description
            get() = context.getString(R.string.vfs_sndh_random_description)
        override val parent
            get() = this@VfsRootSndh

        override fun enumerate(visitor: VfsDir.Visitor) = Unit

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.FEED -> FeedIterator()
            VfsExtensions.ICON -> R.drawable.ic_browser_vfs_radio
            else -> super.getExtension(id)
        }
    }

    private inner class FeedIterator : Iterator<VfsFile> {
        private var nextTrack: Track? = null
        private var lastId = -1
        private var repeats = 0

        override fun hasNext(): Boolean {
            nextTrack?.let {
                return true
            }
            repeat(MAX_TRIES) {
                val track = loadRandomTrack() ?: return@repeat
                if (track.id == lastId) {
                    if (++repeats >= MAX_REPEATS) {
                        LOG.d { "Break random feed loop" }
                        return false
                    }
                } else {
                    lastId = track.id
                    repeats = 0
                }
                nextTrack = track
                return true
            }
            return false
        }

        override fun next(): VfsFile {
            if (!hasNext()) {
                throw NoSuchElementException()
            }
            val track = requireNotNull(nextTrack)
            nextTrack = null
            return TrackFile(
                Identifier.forTrack(Identifier.forCategory(Identifier.CATEGORY_RANDOM), track)
                    .build(),
                track
            )
        }

        private fun loadRandomTrack(): Track? = try {
            catalog.findRandomTrack()
        } catch (e: IOException) {
            LOG.w(e) { "Failed to get random track" }
            null
        }
    }

    private inner class TrackFile(
        override val uri: Uri,
        private val track: Track,
    ) : StubObject(),
        VfsFile {
        override val name
            get() = track.title.takeIf { it.isNotEmpty() }?.let { "$it.sndh" }
                ?: "${track.id}.sndh"
        override val size
            get() = track.sizeText
        override val parent
            get() = uri.pathSegments.let { path ->
                when (Identifier.findCategory(path)) {
                    Identifier.CATEGORY_AUTHORS -> Identifier.findAuthor(uri, path)
                        ?.let { AuthorDir(it) }

                    Identifier.CATEGORY_RANDOM -> RandomDir()

                    else -> this@VfsRootSndh
                }
            }

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.CACHE_PATH -> track.id.toString()
            VfsExtensions.DOWNLOAD_URIS -> RemoteCatalog.getTrackUris(track.id)
            VfsExtensions.SHARE_URL -> "https://sndh.atari.org/?ID=${track.id}"
            else -> super.getExtension(id)
        }
    }

    companion object {
        private const val MAX_TRIES = 5
        private const val MAX_REPEATS = 10
        private val LOG = Logger(VfsRootSndh::class.java.name)
    }
}
