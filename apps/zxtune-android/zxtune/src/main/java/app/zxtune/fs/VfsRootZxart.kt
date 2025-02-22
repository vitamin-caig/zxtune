/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

import android.content.Context
import android.net.Uri
import android.util.SparseIntArray
import androidx.annotation.StringRes
import androidx.core.util.keyIterator
import app.zxtune.R
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.zxart.Author
import app.zxtune.fs.zxart.Catalog
import app.zxtune.fs.zxart.Identifier
import app.zxtune.fs.zxart.Party
import app.zxtune.fs.zxart.RemoteCatalog
import app.zxtune.fs.zxart.Track

class VfsRootZxart(
    override val parent: VfsObject, private val context: Context, http: MultisourceHttpProvider
) : StubObject(), VfsRoot {
    private val catalog = Catalog.create(context, http)
    private val groups = arrayOf(
        AuthorsDir(), PartiesDir(), TopTracksDir()
    )

    override val name
        get() = context.getString(R.string.vfs_zxart_root_name)
    override val description
        get() = context.getString(R.string.vfs_zxart_root_description)
    override val uri: Uri
        get() = Identifier.forRoot().build()

    override fun getExtension(id: String) = when (id) {
        //assume search by authors from root
        VfsExtensions.SEARCH_ENGINE -> AuthorsSearchEngine()
        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_zxart
        else -> super.getExtension(id)
    }

    override fun enumerate(visitor: VfsDir.Visitor) = groups.forEach(visitor::onDir)

    override fun resolve(uri: Uri) = if (Identifier.isFromRoot(uri)) {
        resolve(uri, uri.pathSegments)
    } else {
        null
    }

    private fun resolve(uri: Uri, path: List<String>) =
        Identifier.findCategory(path)?.let { category ->
            groups.find { it.path == category }?.resolve(uri, path)
        } ?: this

    private abstract class GroupingDir(val path: String) : StubObject(), VfsDir {
        override val uri: Uri
            get() = id.build()

        abstract fun resolve(uri: Uri, path: List<String>): VfsObject?

        protected val id
            get() = Identifier.forCategory(path)
    }

    private inner class AuthorsDir : GroupingDir(Identifier.CATEGORY_AUTHORS) {
        override val name
            get() = context.getString(R.string.vfs_zxart_authors_name)
        override val parent
            get() = this@VfsRootZxart

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.SEARCH_ENGINE -> AuthorsSearchEngine()
            else -> super.getExtension(id)
        }

        override fun enumerate(visitor: VfsDir.Visitor) = catalog.queryAuthors { obj ->
            visitor.onDir(AuthorDir(obj))
        }

        // use plain resolving to avoid deep hierarchy
        // check most frequent cases first
        override fun resolve(uri: Uri, path: List<String>) = Identifier.findAuthorTrack(
            uri, path
        )?.let { track ->
            AuthorTrackFile(uri, track)
        } ?: resolveAuthorDir(uri, path) ?: this
    }

    private fun resolveAuthorDir(uri: Uri, path: List<String>) =
        Identifier.findAuthor(uri, path)?.let { author ->
            Identifier.findAuthorYear(uri, path)?.let { year ->
                AuthorYearDir(author, year)
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
            catalog.queryAuthorTracks(author) { obj -> put(obj.year, 1 + get(obj.year)) }
        }.keyIterator().forEach { year ->
            // handle count when required
            visitor.onDir(AuthorYearDir(author, year))
        }
    }

    private inner class AuthorYearDir(private val author: Author, private val year: Int) :
        StubObject(), VfsDir {
        private val id
            get() = Identifier.forAuthor(author, year)

        override val uri: Uri
            get() = id.build()
        override val name
            get() = if (year != 0) year.toString() else context.getString(R.string.vfs_zxart_unknown_year_name)
        override val parent
            get() = AuthorDir(author)

        override fun enumerate(visitor: VfsDir.Visitor) = catalog.queryAuthorTracks(author) { obj ->
            if (year == obj.year) {
                val uri = Identifier.forTrack(id, obj).build()
                visitor.onFile(AuthorTrackFile(uri, obj))
            }
        }
    }

    private inner class AuthorTrackFile(uri: Uri, track: Track) : BaseTrackFile(uri, track) {
        override val parent
            get() = resolveAuthorDir(uri, uri.pathSegments)
    }

    private inner class AuthorsSearchEngine : VfsExtensions.SearchEngine {
        override fun find(query: String, visitor: VfsExtensions.SearchEngine.Visitor) =
            catalog.findTracks(query) { author, track ->
                val uri =
                    Identifier.forTrack(Identifier.forAuthor(author, track.year), track).build()
                visitor.onFile(AuthorTrackFile(uri, track))
            }
    }

    private inner class PartiesDir : GroupingDir(Identifier.CATEGORY_PARTIES) {
        override val name
            get() = context.getString(R.string.vfs_zxart_parties_name)
        override val parent
            get() = this@VfsRootZxart

        override fun enumerate(visitor: VfsDir.Visitor) = SparseIntArray().apply {
            catalog.queryParties { obj -> put(obj.year, 1 + get(obj.year)) }
        }.keyIterator().forEach { year ->
            // handle count when required
            visitor.onDir(PartyYearDir(year))
        }

        override fun resolve(uri: Uri, path: List<String>) =
            Identifier.findPartyTrack(uri, path)?.let { track ->
                PartyTrackFile(uri, track)
            } ?: resolvePartyDir(uri, path) ?: this
    }

    private fun resolvePartyDir(uri: Uri, path: List<String>) =
        Identifier.findParty(uri, path)?.let { party ->
            Identifier.findPartyCompo(uri, path)?.let { compo ->
                PartyCompoDir(party, compo)
            } ?: PartyDir(party)
        } ?: Identifier.findPartiesYear(uri, path)?.let { year ->
            PartyYearDir(year)
        }

    private inner class PartyYearDir(private val year: Int) : StubObject(), VfsDir {
        override val uri: Uri
            get() = Identifier.forPartiesYear(year).build()
        override val name
            get() = year.toString()
        override val parent
            get() = groups[1] // TODO

        override fun enumerate(visitor: VfsDir.Visitor) = catalog.queryParties { obj ->
            if (obj.year == year) {
                visitor.onDir(PartyDir(obj))
            }
        }
    }

    private inner class PartyDir(private val party: Party) : StubObject(), VfsDir {
        override val uri: Uri
            get() = Identifier.forParty(party).build()
        override val name
            get() = party.name
        override val parent
            get() = PartyYearDir(party.year)

        override fun enumerate(visitor: VfsDir.Visitor) = HashSet<String>().apply {
            catalog.queryPartyTracks(party) { obj -> add(obj.compo) }
        }.forEach { compo ->
            visitor.onDir(PartyCompoDir(party, compo))
        }
    }

    enum class LocalizedCompoName(@StringRes val titleRes: Int) {
        UNKNOWN(R.string.vfs_zxart_compo_unknown), STANDARD(R.string.vfs_zxart_compo_standard), AY(R.string.vfs_zxart_compo_ay), BEEPER(
            R.string.vfs_zxart_compo_beeper
        ),
        COPYAY(R.string.vfs_zxart_compo_copyay), NOCOPYAY(R.string.vfs_zxart_compo_nocopyay), REALTIME(
            R.string.vfs_zxart_compo_realtime
        ),
        REALTIMEAY(R.string.vfs_zxart_compo_realtimeay), REALTIMEBEEPER(R.string.vfs_zxart_compo_realtimebeeper), OUT(
            R.string.vfs_zxart_compo_out
        ),
        WILD(R.string.vfs_zxart_compo_wild), EXPERIMENTAL(R.string.vfs_zxart_compo_experimental), OLDSCHOOL(
            R.string.vfs_zxart_compo_oldschool
        ),
        MAINSTREAM(R.string.vfs_zxart_compo_mainstream), PROGRESSIVE(R.string.vfs_zxart_compo_progressive), TS(
            R.string.vfs_zxart_compo_ts
        ),
        TSFM(R.string.vfs_zxart_compo_tsfm), RELATED(R.string.vfs_zxart_compo_related);

        companion object {
            fun get(id: String, ctx: Context) = id.uppercase().let { name ->
                entries.find { it.name == name }
            }?.run {
                ctx.getString(titleRes)
            } ?: id
        }
    }

    private object PartyCompoTracksComparator : Comparator<VfsObject> {
        override fun compare(lh: VfsObject, rh: VfsObject): Int {
            val lhPlace = (lh as BaseTrackFile).module.partyplace
            val rhPlace = (rh as BaseTrackFile).module.partyplace
            return lhPlace.compareTo(rhPlace)
        }
    }

    // custom ordering by party place
    private inner class PartyCompoDir(private val party: Party, private val compo: String) :
        StubObject(), VfsDir {
        private val id
            get() = Identifier.forPartyCompo(party, compo)

        override val uri: Uri
            get() = id.build()
        override val name
            get() = LocalizedCompoName.get(compo, context)
        override val parent
            get() = PartyDir(party)

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.COMPARATOR -> PartyCompoTracksComparator
            else -> super.getExtension(id)
        }

        override fun enumerate(visitor: VfsDir.Visitor) = catalog.queryPartyTracks(party) { obj ->
            if (obj.compo == compo) {
                val uri = Identifier.forTrack(id, obj).build()
                visitor.onFile(PartyTrackFile(uri, obj))
            }
        }
    }

    private inner class PartyTrackFile(uri: Uri, module: Track) : BaseTrackFile(uri, module) {
        override val parent
            get() = resolvePartyDir(uri, uri.pathSegments)
        override val size
            get() = module.partyplace.toString()
    }

    private object TopTracksComparator : Comparator<VfsObject> {
        override fun compare(lh: VfsObject, rh: VfsObject): Int {
            // assume that votes can be compared in lexicographical order
            return -java.lang.String.CASE_INSENSITIVE_ORDER.compare(
                (lh as VfsFile).size, (rh as VfsFile).size
            )
        }
    }

    // custom ordering by votes desc
    private inner class TopTracksDir : GroupingDir(Identifier.CATEGORY_TOP) {
        override val name
            get() = context.getString(R.string.vfs_zxart_toptracks_name)
        override val description
            get() = context.getString(R.string.vfs_zxart_toptracks_description)
        override val parent
            get() = this@VfsRootZxart

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.COMPARATOR -> TopTracksComparator
            else -> super.getExtension(id)
        }

        override fun enumerate(visitor: VfsDir.Visitor) = catalog.queryTopTracks(100) { obj ->
            val uri = Identifier.forTrack(id, obj).build()
            visitor.onFile(TopTrackFile(uri, obj))
        }

        override fun resolve(uri: Uri, path: List<String>) =
            Identifier.findTopTrack(uri, path)?.let { track ->
                TopTrackFile(uri, track)
            } ?: this

        private inner class TopTrackFile(uri: Uri, module: Track) : BaseTrackFile(uri, module) {
            override val parent
                get() = this@TopTracksDir
            override val size
                get() = module.votes
        }
    }

    private abstract class BaseTrackFile(override val uri: Uri, val module: Track) : StubObject(),
        VfsFile {
        override val name
            get() = uri.lastPathSegment ?: ""

        override val description
            get() = module.title

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.CACHE_PATH -> module.id.toString()
            VfsExtensions.DOWNLOAD_URIS -> RemoteCatalog.getTrackUris(module.id)
            VfsExtensions.SHARE_URL -> shareUrl
            else -> super.getExtension(id)
        }

        override val size
            get() = module.duration

        private val shareUrl
            get() = "https://zxart.ee/zxtune/action%%3aplay/tuneId%%3a${module.id}"
    }
}
