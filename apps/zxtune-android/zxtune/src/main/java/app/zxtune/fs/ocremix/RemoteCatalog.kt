package app.zxtune.fs.ocremix

import android.net.Uri
import app.zxtune.Logger
import app.zxtune.fs.HtmlUtils
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.api.Proxy
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.io.TorrentFile
import app.zxtune.utils.ProgressCallback
import app.zxtune.utils.ifNotNulls
import org.jsoup.nodes.Document
import org.jsoup.nodes.Element

private val LOG = Logger(RemoteCatalog::class.java.name)

class RemoteCatalog(private val http: MultisourceHttpProvider) : Catalog {
    override fun querySystems(visitor: Catalog.SystemsVisitor) {
        LOG.d { "querySystems()" }
        readPage(baseUri.scoped(null, EntityTypes.SYSTEMS)).select("table>tbody>tr.area-link")
            .forEach { row ->
                row.findIdNode("/system/")?.let { node ->
                    val image = row.findImagePath("/img-size/100/files/")
                    visitor.accept(System(System.Id(node.id), node.text), image)
                }
            }
    }

    override fun queryOrganizations(
        visitor: Catalog.Visitor<Organization>, progress: ProgressCallback
    ) {
        var total = 300 // only with games or remixes
        val uri = baseUri.scoped(null, EntityTypes.ORGANIZATIONS)
            .appendQueryParameter("sort", "gamecountdesc").build()
        var offset = 0
        do {
            LOG.d { "queryOrganizations(offset=${offset})" }
            if (offset > 0) {
                progress.onProgressUpdate(offset, total)
            }
            readPage(uri.withOffset(offset)).select("table>tbody>tr.area-link").also {
                offset += it.size
                if (offset > total) {
                    total = offset + 100
                }
            }.forEach { row ->
                ifNotNulls(
                    row.findIdNode("/org/"),
                    row.selectFirst("span.badge[title*=Game]")?.text()?.toIntOrNull()
                ) { org, _ ->
                    // TODO: pass also games count
                    visitor.accept(Organization(Organization.Id(org.id), org.text))
                } ?: run {
                    offset = total
                    return@forEach
                }
            }
        } while (offset < total)
    }

    override fun queryGames(
        scope: Catalog.Scope?, visitor: Catalog.GamesVisitor, progress: ProgressCallback
    ) {
        val uri = baseUri.scoped(scope, EntityTypes.GAMES).build()
        var offset = 0
        var total: Int? = null
        do {
            LOG.d { "queryGames(scope=${scope}, offset=${offset})" }
            val doc = readPage(uri.withOffset(offset))
            val rows = doc.select("table>tbody>tr.row1")
            if (total == null) {
                total = doc.findTotal() ?: rows.size
            } else {
                progress.onProgressUpdate(offset, total)
            }
            rows.forEach { row ->
                ifNotNulls(
                    row.findIdNode("/game/"), row.findIdNode("/system/"),
                ) { game, system ->
                    // May be optional, see game 714
                    val org = row.findIdNode("/org/")?.run {
                        Organization(Organization.Id(id), text)
                    }
                    val img = row.findImagePath("/img-size/500/files/")
                        ?: row.findImagePath("/img-size/100/files/") // if any...
                    visitor.accept(
                        Game(Game.Id(game.id), game.text),
                        System(System.Id(system.id), system["title"] ?: system.text),
                        org,
                        img,
                    )
                }
            }
            offset += rows.size
        } while (offset < (total ?: 0))
    }

    override fun queryRemixes(
        scope: Catalog.Scope?, visitor: Catalog.RemixesVisitor, progress: ProgressCallback
    ) {
        val uri = baseUri.scoped(scope, EntityTypes.REMIXES).build()
        var offset = 0
        var total: Int? = null
        do {
            LOG.d { "queryRemixes(scope=${scope}, offset=${offset})" }
            val doc = readPage(uri.withOffset(offset))
            val rows = doc.select("table>tbody>tr")
            if (offset == 0) {
                total = doc.findTotal()
            } else if (total != null) {
                progress.onProgressUpdate(offset, total)
            }
            var currentGame: Game? = null
            rows.forEach { row ->
                row.findIdNode("/game/")?.run {
                    currentGame = Game(Game.Id(id), text)
                }
                ifNotNulls(row.findIdNode("/remix/"), currentGame) { rmx, game ->
                    ++offset
                    visitor.accept(Remix(Remix.Id(rmx.id), rmx.text.trim('"')), game)
                }
            }
        } while (offset < (total ?: 0))
    }

    override fun queryAlbums(
        scope: Catalog.Scope?, visitor: Catalog.AlbumsVisitor, progress: ProgressCallback
    ) {
        val uri = baseUri.scoped(scope, EntityTypes.ALBUMS).build()
        var offset = 0
        var total: Int? = null
        do {
            LOG.d { "queryAlbums(scope=${scope}, offset=${offset})" }
            val doc = readPage(uri.withOffset(offset))
            val rows = doc.select("table>tbody>tr.area-link")
            if (total == null) {
                total = doc.findTotal() ?: rows.size
            } else {
                progress.onProgressUpdate(offset, total)
            }
            rows.forEach { row ->
                row.findIdNode("/album/")?.let { album ->
                    visitor.accept(
                        Album(Album.Id(album.id), album.text),
                        row.findImagePath("/thumbs/100//files/")
                            ?: row.findImagePath("/thumbs/100/files/"), // if any...
                    )
                }
            }
            offset += rows.size
        } while (offset < (total ?: 0))
    }

    override fun findRemixPath(id: Remix.Id) =
        readPage(baseUri + id).findPath("https://ocrmirror.org/files/")?.takeIf { it.isMusic }

    override fun queryAlbumTracks(id: Album.Id, visitor: Catalog.AlbumTracksVisitor) {
        val torrentPath =
            readPage(baseUri + id).findPath("https://bt.ocremix.org/")?.takeIf { it.isTorrent }
                ?: return
        readTorrent(torrentPath).forEachFile { path, length ->
            FilePath(ALBUMS_PATH + path).takeIf { it.isMusic }?.let {
                visitor.accept(it, length)
            }
        }
    }

    override fun queryGameDetails(id: Game.Id) = readPage(baseUri + id).let { page ->
        val chiptune = page.findIdNode("/chip/")?.text?.takeIf {
            it.startsWith("music/chiptunes")
        }?.let {
            FilePath(it)
        }
        val image = page.findImagePath("/img-size/500/files/")
        Game.Details(chiptune, image)
    }

    override fun queryAlbumImage(id: Album.Id) =
        readPage(baseUri + id).findImagePath("/thumbs/500/files/")

    private fun readPage(uri: Uri.Builder) = uri.build().let {
        HtmlUtils.parseDoc(http.getInputStream(arrayOf(Proxy.uriFor(it), it)))
    }

    private fun readTorrent(path: FilePath) = http.getInputStream(getRemoteUris(path)).use {
        TorrentFile.parse(it.readBytes())
    }

    companion object {
        private const val ALBUMS_PATH = "albums/"

        fun getRemoteUris(path: FilePath) = path.value.let {
            if (it.startsWith(ALBUMS_PATH)) arrayOf(Cdn.ocremix(it))
            else arrayOf(
                Cdn.ocremix(it), Uri.Builder().scheme("https").apply {
                    if (path.isTorrent) authority("bt.ocremix.org").path(it)
                    else authority("ocrmirror.org").path("files/$it")
                }.build()
            )
        }
    }
}

private class IdNode(val ref: Element, val id: String) {
    val text: String
        get() = ref.text()

    operator fun get(attr: String): String? = ref.attr(attr).takeIf { it.isNotEmpty() }
}

private fun Document.findTotal() = selectFirst("ul.pagination>li>a.page-link")?.run {
    // 'entities X to Y of TOTAL'
    ownText().split(' ').lastOrNull()?.toIntOrNull()
}

private fun Element.findIdNode(prefix: String, needText: Boolean = true) =
    selectFirst("a[href^=${prefix}]" + (if (needText) ":matchesOwn(\\S+)" else ""))?.let {
        val idStr = it.attr("href").run { substring(prefix.length) }
        IdNode(it, idStr)
    }

private fun Element.findPath(url: String) = findIdNode(prefix = url, needText = false)?.let {
    FilePath(it.id)
}

private fun Element.findImagePath(prefix: String) =
    selectFirst("img[src^=${prefix}]")?.attr("src")?.substring(prefix.length)?.let {
        FilePath(it)
    }

private val baseUri
    get() = Uri.Builder().scheme("https").authority("ocremix.org")

private infix operator fun Uri.Builder.plus(path: String) = appendPath(path)
private infix operator fun Uri.Builder.plus(id: Remix.Id) = appendPath("remix").appendPath(id.value)
private infix operator fun Uri.Builder.plus(id: Album.Id) =
    appendPath("album").appendEncodedPath(id.value)

private infix operator fun Uri.Builder.plus(id: Game.Id) =
    appendPath("game").appendEncodedPath(id.value)

private fun Uri.Builder.append(scope: Catalog.Scope) = when (scope) {
    is Catalog.SystemScope -> appendPath("system").appendEncodedPath(scope.id.value)
    is Catalog.OrganizationScope -> appendPath("org").appendEncodedPath(scope.id.value)
    is Catalog.GameScope -> this + scope.id
}

enum class EntityTypes {
    SYSTEMS, ORGANIZATIONS, GAMES, REMIXES, ALBUMS
}

// ../${suffix}/ but ../${scope}/${suffix}
private fun Uri.Builder.scoped(scope: Catalog.Scope?, type: EntityTypes): Uri.Builder = scope?.let {
    append(it).appendPath(type.name.lowercase())
} ?: appendPath(type.name.lowercase()).apply {
    // Avoid redirect ../systems/ -> ../systems
    if (type != EntityTypes.SYSTEMS) {
        appendPath("")
    }
}

private fun Uri.withOffset(offset: Int) =
    buildUpon().appendQueryParameter("offset", offset.toString())
