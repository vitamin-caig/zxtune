/**
 * @file
 * @brief Implementation of VfsRoot over http://zxtunes.com catalogue
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

import android.content.Context
import android.net.Uri
import android.util.SparseIntArray
import androidx.core.util.forEach
import app.zxtune.R
import app.zxtune.TimeStamp
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.zxtunes.Author
import app.zxtune.fs.zxtunes.Catalog
import app.zxtune.fs.zxtunes.Identifier
import app.zxtune.fs.zxtunes.RemoteCatalog
import app.zxtune.fs.zxtunes.Track

class VfsRootZxtunes(
    override val parent: VfsObject, private val context: Context, http: MultisourceHttpProvider
) : StubObject(), VfsRoot {
    private val catalog = Catalog.create(context, http)
    private val groups = arrayOf<GroupingDir>(AuthorsDir())

    override val uri: Uri
        get() = Identifier.forRoot().build()
    override val name
        get() = context.getString(R.string.vfs_zxtunes_root_name)
    override val description
        get() = context.getString(R.string.vfs_zxtunes_root_description)

    override fun getExtension(id: String) = when (id) {
        //assume root will search by authors
        VfsExtensions.SEARCH_ENGINE -> if (catalog.searchSupported()) AuthorsSearchEngine() else null

        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_zxtunes
        else -> super.getExtension(id)
    }

    override fun enumerate(visitor: VfsDir.Visitor) = groups.forEach(visitor::onDir)

    override fun resolve(uri: Uri) = if (Identifier.isFromRoot(uri)) {
        resolve(uri, uri.pathSegments)
    } else {
        null
    }

    private fun resolve(uri: Uri, path: List<String>) =
        when (val category = Identifier.findCategory(path)) {
            null -> this
            Identifier.CATEGORY_IMAGES -> resolveImages(uri, path)
            else -> groups.find { it.path == category }?.resolve(uri, path)
        }

    private fun resolveImages(uri: Uri, path: List<String>) =
        Identifier.findAuthor(uri, path)?.let {
            ImageFile(it)
        }

    private abstract inner class GroupingDir(val path: String) : StubObject(), VfsDir {
        override val uri: Uri
            get() = Identifier.forCategory(path).build()

        abstract fun resolve(uri: Uri, path: List<String>): VfsObject?
    }

    private inner class AuthorsDir : GroupingDir(Identifier.CATEGORY_AUTHORS) {
        override val name
            get() = context.getString(R.string.vfs_zxtunes_authors_name)
        override val description
            get() = context.getString(R.string.vfs_zxtunes_authors_description)
        override val parent
            get() = this@VfsRootZxtunes

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.SEARCH_ENGINE -> if (catalog.searchSupported()) AuthorsSearchEngine()
            else null

            else -> super.getExtension(id)
        }

        override fun enumerate(visitor: VfsDir.Visitor) = catalog.queryAuthors { obj ->
            visitor.onDir(AuthorDir(obj))
        }

        // use plain resolving with most frequent cases first
        override fun resolve(uri: Uri, path: List<String>) = Identifier.findTrack(uri, path)?.let {
            TrackFile(uri, it)
        } ?: resolveDir(uri, path) ?: this

    }

    private fun resolveDir(uri: Uri, path: List<String>) =
        Identifier.findAuthor(uri, path)?.let { author ->
            Identifier.findDate(uri, path)?.let { date ->
                AuthorDateDir(author, date, 0)
            } ?: AuthorDir(author)
        }

    private inner class AuthorDir(private val author: Author) : StubObject(), VfsDir {
        override val uri: Uri
            get() = Identifier.forAuthor(author).build()
        override val name
            get() = author.nickname
        override val description
            get() = author.name
        override val parent
            get() = groups[0] // TODO

        override fun enumerate(visitor: VfsDir.Visitor) = SparseIntArray().apply {
            catalog.queryAuthorTracks(author) { obj ->
                if (obj.date == null || obj.date == 0) {
                    val uri = Identifier.forTrack(Identifier.forAuthor(author), obj).build()
                    visitor.onFile(TrackFile(uri, obj))
                } else {
                    put(obj.date, 1 + get(obj.date))
                }
            }
        }.forEach { key, value ->
            visitor.onDir(AuthorDateDir(author, key, value))
        }

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.COVER_ART_URI -> catalog.queryAuthor(author.id)
                ?.takeIf { true == it.hasPhoto }?.let { Identifier.forPhotoOf(it) }
            // assume called on enumerated object with full Author
            VfsExtensions.ICON_URI -> author.takeIf { true == it.hasPhoto }?.let {
                Identifier.forPhotoOf(it)
            }
            else -> super.getExtension(id)
        }
    }

    private inner class AuthorDateDir(
        private val author: Author, private val date: Int, private val count: Int
    ) : StubObject(), VfsDir {
        override val uri: Uri
            get() = Identifier.forAuthor(author, date).build()
        override val name
            get() = date.toString()
        override val description
            get() = context.resources.getQuantityString(R.plurals.tracks, count, count)
        override val parent
            get() = AuthorDir(author)

        override fun enumerate(visitor: VfsDir.Visitor) = catalog.queryAuthorTracks(author) { obj ->
            if (date == obj.date) {
                val uri = Identifier.forTrack(Identifier.forAuthor(author), obj).build()
                visitor.onFile(TrackFile(uri, obj))
            }
        }
    }

    private inner class TrackFile(override val uri: Uri, private val module: Track) : StubObject(),
        VfsFile {
        override val name
            get() = requireNotNull(uri.lastPathSegment)
        override val description
            get() = module.title
        override val parent
            get() = resolveDir(uri, uri.pathSegments)

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.CACHE_PATH -> module.id.toString()
            VfsExtensions.DOWNLOAD_URIS -> RemoteCatalog.getTrackUris(module.id)
            VfsExtensions.SHARE_URL -> shareUrl
            else -> super.getExtension(id)
        }

        override val size
            get() = module.duration?.let {
                FRAME_DURATION.multiplies(it.toLong()).toString()
            } ?: ""

        val shareUrl
            get() = Identifier.findAuthor(uri, uri.pathSegments)?.let { author ->
                "http://zxtunes.com/author.php?id=${author.id}&play=${module.id}"
            }
    }

    private class ImageFile(
        private val author: Author,
        override val parent: VfsObject? = null,
        override val size: String = ""
    ) : StubObject(), VfsFile {
        override val uri: Uri
            get() = Identifier.forPhotoOf(author)
        override val name
            get() = author.nickname

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.DOWNLOAD_URIS -> RemoteCatalog.getImageUris(author.id)
            else -> super.getExtension(id)
        }
    }

    private inner class AuthorsSearchEngine : VfsExtensions.SearchEngine {
        override fun find(query: String, visitor: VfsExtensions.SearchEngine.Visitor) =
            catalog.findTracks(query) { author, track ->
                val uri = Identifier.forTrack(Identifier.forAuthor(author), track).build()
                visitor.onFile(TrackFile(uri, track))
            }
    }

    companion object {
        private val FRAME_DURATION = TimeStamp.fromMilliseconds(20)
    }
}
