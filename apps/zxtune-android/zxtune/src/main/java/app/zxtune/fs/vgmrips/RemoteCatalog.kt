package app.zxtune.fs.vgmrips

import android.net.Uri
import androidx.annotation.VisibleForTesting
import app.zxtune.fs.HtmlUtils
import app.zxtune.fs.Jsoup.findFirst
import app.zxtune.fs.Jsoup.findFirstText
import app.zxtune.fs.Jsoup.parent
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import org.jsoup.nodes.Document
import org.jsoup.nodes.Element

private const val COMPANIES_DIR_PATH = "/packs/companies/"
private const val COMPANIES_FILE_PREFIX = "/packs/company/"
private const val COMPOSERS_DIR_PATH = "/packs/composers/"
private const val COMPOSERS_FILE_PREFIX = "/packs/composer/"
private const val CHIPS_DIR_PATH = "/packs/chips/"
private const val CHIPS_FILE_PREFIX = "/packs/chip/"
private const val SYSTEMS_DIR_PATH = "/packs/systems/"
private const val SYSTEMS_FILE_PREFIX = "/packs/system/"
private const val SYSTEMS_TITLE_PREFIX = "View games on "

open class RemoteCatalog(val http: MultisourceHttpProvider) : Catalog {

    private val companies by lazy { CompaniesGrouping() }
    private val composers by lazy { ComposersGrouping() }
    private val chips by lazy { ChipsGrouping() }
    private val systems by lazy { SystemsGrouping() }

    override fun companies(): Catalog.Grouping = companies

    override fun composers(): Catalog.Grouping = composers

    override fun chips(): Catalog.Grouping = chips

    override fun systems(): Catalog.Grouping = systems

    override fun findPack(id: Pack.Id) = findPackInternal("/packs/pack/${id.value}", null)

    override fun findRandomPack(visitor: Catalog.Visitor<FilePath>) =
        findPackInternal("/packs/random", visitor)

    @VisibleForTesting
    fun findPackInternal(path: String, visitor: Catalog.Visitor<FilePath>?): Pack? =
        readDoc(buildUri(path)).let { doc ->
            val id = doc.findFirst("meta[property=og:url]")?.extract("content", "/packs/pack/")
                ?: return null
            val title = doc.findFirstText("div.row>section>h1") ?: return null
            val archive = doc.findFirst("a[href*=/files/]:has(span.icon-download)")
                ?.extract("href", "/files/") ?: return null
            val image = doc.findFirst("div.image>a>img")?.extract("src", "/packs/images/large/")
            Pack(Pack.Id(id), title, FilePath(archive), image?.let { FilePath(it) }).also {
                visitor?.let {
                    parsePackTracks(doc, it)
                }
            }
        }

    open fun isAvailable() = http.hasConnection()

    companion object {
        fun getRemoteUris(path: FilePath) =
            when (path.value.substringAfterLast('.', "").lowercase()) {
                "zip" -> getUris("files/${path.value}")
                "vgz" -> getTrackUris(path.value)
                "png", "jpg" -> getImageUris(path.value)
                else -> emptyArray<Uri>()
            }

        private fun getUris(path: String) = arrayOf(
            Cdn.vgmrips(path), buildBaseUri(path).build()
        )

        private fun getTrackUris(path: String) = arrayOf(
            Cdn.vgmrips(path), buildBaseUri("packs/vgm/$path").build()
        )

        private fun getImageUris(path: String, size: String = "large") =
            getUris("packs/images/${size}/${path}")
    }

    private fun readDoc(uri: Uri): Document = HtmlUtils.parseDoc(http.getInputStream(uri))

    private inner class CompaniesGrouping : Catalog.Grouping {

        override fun query(visitor: Catalog.Visitor<Group>) =
            readDoc(buildUri(COMPANIES_DIR_PATH)).select("table>tbody>tr>td.link>a[href*=net/packs/company]")
                .forEach { ref ->
                    val id = ref.extract("href", COMPANIES_FILE_PREFIX) ?: return@forEach
                    val title = ref.attr("title")
                    val packs = tryGetPacksCountFromBadge(ref.parent?.parent, "a[href$=/developed]")
                    if (id.isNotEmpty() && title.isNotEmpty() && packs != 0) {
                        visitor.accept(Group(Group.Id(id), title, packs))
                    }
                }

        override fun queryPacks(
            id: Group.Id, visitor: Catalog.Visitor<Pack>, progress: ProgressCallback
        ) = parsePacks("${COMPANIES_FILE_PREFIX}${id.value}/developed", visitor, progress)
    }

    private inner class ComposersGrouping : Catalog.Grouping {

        override fun query(visitor: Catalog.Visitor<Group>) =
            readDoc(buildUri(COMPOSERS_DIR_PATH)).select("li.composer>a[href*=net/packs/composer/]")
                .forEach { ref ->
                    val id = ref.extract("href", COMPOSERS_FILE_PREFIX) ?: return@forEach
                    val name = ref.ownText()
                    val packs = tryGetPacksCountFromBadge(ref)
                    if (packs != 0) {
                        visitor.accept(Group(Group.Id(id), name, packs))
                    }
                }

        override fun queryPacks(
            id: Group.Id, visitor: Catalog.Visitor<Pack>, progress: ProgressCallback
        ) = parsePacks("${COMPOSERS_FILE_PREFIX}${id.value}", visitor, progress)
    }

    private inner class ChipsGrouping : Catalog.Grouping {

        override fun query(visitor: Catalog.Visitor<Group>) {
            readDoc(buildUri(CHIPS_DIR_PATH)).select("div.chip>a[href*=net/packs/chip/]")
                .forEach { ref ->
                    val id = ref.extract("href", CHIPS_FILE_PREFIX) ?: return@forEach
                    val name = ref.ownText()
                    val packs = tryGetPacksCountFromBadge(ref.parent)
                    if (packs != 0) {
                        visitor.accept(Group(Group.Id(id), name, packs))
                    }
                }
        }

        override fun queryPacks(
            id: Group.Id, visitor: Catalog.Visitor<Pack>, progress: ProgressCallback
        ) = parsePacks("${CHIPS_FILE_PREFIX}${id.value}", visitor, progress)
    }

    private inner class SystemsGrouping : Catalog.Grouping {

        override fun query(visitor: Catalog.Visitor<Group>) =
            readDoc(buildUri(SYSTEMS_DIR_PATH)).select("a[href*=net/packs/system/]:has(>img,div>span)")
                .forEach { ref ->
                    val id = ref.extract("href", SYSTEMS_FILE_PREFIX) ?: return@forEach
                    val title = ref.extract("title", SYSTEMS_TITLE_PREFIX) ?: return@forEach
                    val packs = tryGetPacksCountFromBadge(ref)
                    if (packs != 0) {
                        visitor.accept(Group(Group.Id(id), title, packs))
                    }
                }

        override fun queryPacks(
            id: Group.Id, visitor: Catalog.Visitor<Pack>, progress: ProgressCallback
        ) = parsePacks("${SYSTEMS_FILE_PREFIX}${id.value}", visitor, progress)
    }

    private fun parsePacks(
        path: String, visitor: Catalog.Visitor<Pack>, progress: ProgressCallback
    ) {
        for (page in 0..Int.MAX_VALUE) {
            val uri = buildPagedUri(path, page)
            parsePacksPage(uri, visitor)?.run {
                if (first != null && second != null) {
                    progress.onProgressUpdate(first!!, second!!)
                }
            } ?: break
        }
    }

    // done, total
    private fun parsePacksPage(uri: Uri, visitor: Catalog.Visitor<Pack>) = readDoc(uri).let { doc ->
        parsePacksProgress(doc)?.also {
            doc.select("div.result:has(>div.image,div.details)").forEach { ref ->
                parsePack(ref)?.run { visitor.accept(this) }
            }
        }
    }
}

private fun buildBaseUri(path: String) =
    Uri.Builder().scheme("https").authority("vgmrips.net").path(path)

private fun buildUri(path: String) = buildBaseUri(path).build()

private fun buildPagedUri(path: String, page: Int) =
    buildBaseUri(path).appendQueryParameter("p", page.toString()).build()

private fun Element.extract(attr: String, prefix: String) =
    attr(attr).substringAfter(prefix, "").takeIf { it.isNotEmpty() }?.let {
        Uri.decode(it)
    }

private fun tryGetPacksCountFromBadge(el: Element?) = tryGetPacksCountFromBadge(el, "span.badge")

private fun tryGetPacksCountFromBadge(el: Element?, query: String) = el?.findFirstText(query)?.let {
    tryParseIntPrefix(it)
} ?: 0

private fun tryParseIntPrefix(txt: String) = txt.takeWhile { it.isDigit() }.toIntOrNull() ?: 0

private fun parsePackTracks(doc: Element, visitor: Catalog.Visitor<FilePath>) =
    doc.select("table.playlist>tbody>tr:has(>td.title)").forEach { track ->
        track.extract("data-vgmurl", "/packs/vgm/")?.let {
            visitor.accept(FilePath(it))
        }
    }

private fun parsePack(el: Element): Pack? = el.run {
    val img = findFirst("div.image>a>img") ?: return null
    val download = findFirst("div.details a.download:has(>small)") ?: return null
    val id = img.parent?.extract("href", "/packs/pack/") ?: return null
    val title = img.attr("alt").takeIf { it.isNotEmpty() } ?: return null
    val archive = download.extract("href", "/files/") ?: return null
    val image = img.extract("src", "/packs/images/small/")
    //[X,songs,*,Y,KB]
    val details = download.text().split(' ').takeIf { it.size == 5 }
    val songs = details?.getOrNull(0)?.let { tryParseIntPrefix(it) } ?: 0
    val size = details?.takeLast(2)?.joinToString(" ") ?: ""
    Pack(Pack.Id(id), title, FilePath(archive), image?.let { FilePath(it) }, songs, size)
}

private fun parsePacksProgress(el: Element) =
    el.findFirst("div.container:has(>div.clearfix)")?.run {
        // [Packs, XX, to, YY, of, ZZ, total]
        textNodes().asSequence().map { it.text() }.filter { it.startsWith("Packs") }
            .map { it.split(' ') }.filter { it.size == 7 }.map {
                val done = HtmlUtils.tryGetInteger(it[3])
                val total = HtmlUtils.tryGetInteger(it[5])
                Pair(done, total)
            }.firstOrNull()
    }

