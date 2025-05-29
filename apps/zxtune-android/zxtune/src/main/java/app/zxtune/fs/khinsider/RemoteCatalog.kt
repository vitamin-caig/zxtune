package app.zxtune.fs.khinsider

import android.net.Uri
import androidx.core.net.toUri
import androidx.core.util.Consumer
import app.zxtune.Logger
import app.zxtune.fs.HtmlUtils
import app.zxtune.fs.Jsoup.findFirst
import app.zxtune.fs.Jsoup.nextElementSibling
import app.zxtune.fs.Jsoup.parent
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import org.jsoup.nodes.Document

class RemoteCatalog(private val http: MultisourceHttpProvider) : Catalog {
    override fun queryScopes(type: Catalog.ScopeType, visitor: Consumer<Scope>) = when (type) {
        Catalog.ScopeType.SERIES -> queryScopes(null, "div#homepagePopularSeries>ul>li>a", visitor)
        Catalog.ScopeType.PLATFORMS -> queryScopes("console-list", "div#pageContent>a", visitor)
        Catalog.ScopeType.TYPES -> queryScopes(null, "h3:containsOwn(By Type)+ul>li>a", visitor)
        Catalog.ScopeType.YEARS -> queryScopes("album-years", "div#pageContent>a", visitor)
    }

    private fun queryScopes(path: String? = null, selector: String, visitor: Consumer<Scope>) =
        readPage(path?.let { Scope.Id(it) }).select(selector).forEach { row ->
            val id = row.attr("href").substringAfter("/", "")
            val title = row.ownText()
            if (id.isNotEmpty() && title.isNotEmpty()) {
                visitor.accept(Scope(Scope.Id(id), title))
            }
        }

    override fun queryAlbums(
        scope: Scope.Id, visitor: Consumer<AlbumAndDetails>, progress: ProgressCallback
    ) = (1..Int.MAX_VALUE).forEach { page ->
        val doc = readPage(scope, page)
        doc.select("table>tbody>tr:has(td.albumIcon)").forEach { row ->
            val icon = row.findFirst("td.albumIcon>a>img[src*=/thumbs_small/]")?.attr("src")
                ?.replace("/thumbs_small/", "/")?.let {
                    FilePath(it.toUri())
                }
            val ref =
                row.findFirst("td.albumIcon~td>a[href^=/game-soundtracks/album/]") ?: return@forEach
            val id = ref.attr("href").substringAfter("/game-soundtracks/album/", "")
            val title = ref.ownText()
            val details = DetailsBuilder().apply {
                var el = ref.parent?.nextElementSibling?.takeIf { it.`is`("td:not([class])") }
                while (el != null) {
                    el.text().takeUnless { it.isEmpty() || it.isBlank() }?.let {
                        add(it)
                    }
                    el = el.nextElementSibling
                }
            }.build()
            if (id.isNotEmpty() && title.isNotEmpty()) {
                visitor.accept(AlbumAndDetails(Album(Album.Id(id), title), details, icon))
            }
        }

        val lastPage = doc.findFirst("li.pagination-end>a.pagenav")?.run {
            attr("href").substringAfter("?page=").toIntOrNull() ?: page
        } ?: return@queryAlbums
        progress.onProgressUpdate(page, lastPage)
        if (lastPage == page) {
            return@queryAlbums
        }
    }

    override fun queryAlbumDetails(
        album: Album.Id, visitor: Consumer<TrackAndDetails>
    ) = queryAlbumDetails(
        if (album == Catalog.RANDOM_ALBUM) RANDOM_ALBUM else Scope.Id("game-soundtracks/album/${album.value}"),
        visitor
    )

    private fun queryAlbumDetails(
        scope: Scope.Id, trackVisitor: Consumer<TrackAndDetails>
    ): AlbumAndDetails? = readPage(scope).run {
        val id = findFirst("td.clickable-row>a[href^=/game-soundtracks/album/]")?.attr("href")
            ?.substringAfter("/game-soundtracks/album/")?.substringBefore('/')
            ?: return@queryAlbumDetails null
        val title = findFirst("div#pageContent>h2")?.ownText() ?: return@queryAlbumDetails null
        val image = findFirst("div.albumImage>a:has(>img)")?.attr("href")?.let {
            FilePath(it.toUri())
        }
        val album = Album(Album.Id(id), title)
        val details = DetailsBuilder().apply {
            val panel = findFirst("div#pageContent>p[align=left]") ?: return@apply
            panel.wholeText().splitToSequence("\r\n").filter { it.isNotBlank() && it.isNotEmpty() }
                .associate {
                    it.split(':', limit = 2).map { it.trim() }.run {
                        get(0) to (getOrNull(1) ?: "")
                    }
                }.run {
                    get("Platforms")?.let {
                        add(it)
                    }
                    get("Album type")?.let {
                        add(it)
                    }
                    get("Year")?.let {
                        add(it)
                    }
                }
        }.build()
        parseTracks(this, album, trackVisitor)
        AlbumAndDetails(album, details, image)
    }

    private fun parseTracks(doc: Document, album: Album, visitor: Consumer<TrackAndDetails>) =
        doc.select("table#songlist>tbody>tr:has(td)").forEachIndexed { idx, row ->
            val refs = row.select("td>a")
            if (refs.size < 4) {
                return@forEachIndexed
            }
            val title = refs[0].ownText().takeIf { it.isNotEmpty() } ?: return@forEachIndexed
            val duration = refs[1].ownText().takeIf { it.isNotEmpty() } ?: return@forEachIndexed
            val size = refs[2].ownText().takeIf { it.isNotEmpty() } ?: return@forEachIndexed
            val id = refs[3].attr("href").substringAfterLast('/').takeIf { it.isNotEmpty() }
                ?: return@forEachIndexed
            visitor.accept(
                TrackAndDetails(
                    album, Track(Track.Id(Uri.decode(Uri.decode(id))), title), idx, duration, size
                )
            )
        }

    override fun findTrackLocation(
        album: Album.Id, track: Track.Id
    ) =
        readPage(Scope.Id("game-soundtracks/album/${album.value}/${Uri.encode(track.value)}")).findFirst(
            "p>a:has(>span.songDownloadLink)"
        )?.attr("href")?.toUri()

    fun isNetworkAvailable() = http.hasConnection()

    private fun readPage(scope: Scope.Id? = null, page: Int? = null) = baseUri.apply {
        scope?.run {
            appendEncodedPath(value)
        }
        page?.let {
            appendQueryParameter("page", it.toString())
        }
    }.build().let {
        LOG.d { "Load page $it" }
        // TODO: Proxy?
        val input = if (scope === RANDOM_ALBUM)
        // https -> http -> http redirects require manual resolving
            http.getObject(arrayOf(it)).input
        else http.getInputStream(it)
        HtmlUtils.parseDoc(input)
    }

    companion object {
        private val LOG = Logger(RemoteCatalog::class.java.name)
        private val RANDOM_ALBUM = Scope.Id("random-album")

        private val baseUri
            get() = Uri.Builder().scheme("https").authority("downloads.khinsider.com")

        fun getRemoteUris(path: FilePath) = arrayOf(
            Cdn.khinsider(requireNotNull(path.value.path).substringAfter('/')), path.value
        )

        fun getRemoteUri(album: Album.Id, track: Track.Id) = Cdn.khinsider(
            "soundtracks/${album.value}/${track.value}"
        )
    }
}

private class DetailsBuilder {
    private val content = StringBuilder()

    fun add(segment: String) = with(content) {
        if (isNotEmpty()) {
            append(DELIMITER)
        }
        append(segment)
        Unit
    }

    fun build(): String = content.toString()

    companion object {
        const val DELIMITER = " / "
    }
}