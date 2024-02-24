package app.zxtune.fs.vgmrips

import android.net.Uri
import app.zxtune.TimeStamp
import app.zxtune.fs.HtmlUtils
import app.zxtune.fs.Jsoup.findFirst
import app.zxtune.fs.Jsoup.findFirstText
import app.zxtune.fs.Jsoup.nextElementSibling
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

    override fun findPack(id: String, visitor: Catalog.Visitor<Track>): Pack? =
        findPackInternal("/packs/pack/${id}", visitor)

    override fun findRandomPack(visitor: Catalog.Visitor<Track>): Pack? =
        findPackInternal("/packs/random", visitor)

    private fun findPackInternal(path: String, visitor: Catalog.Visitor<Track>) =
        readDoc(buildUri(path)).let { doc ->
            val url = doc.findFirst("meta[property=og:url]")?.attr("content") ?: return null
            val id = getSuffix(url, "/packs/pack/")
            val title = doc.findFirstText("div.row>section>h1") ?: ""
            tryMakePack(id, title)?.apply {
                updateRating(this, doc)
                updateImage(this, doc, "/packs/images/large/")
                parsePackTracks(this, visitor, doc)
            }
        }

    open fun isAvailable() = http.hasConnection()

    companion object {
        @JvmStatic
        fun getRemoteUris(track: Track) =
            arrayOf(
                Cdn.vgmrips(track.location),
                buildBaseUri("packs/vgm/${track.location}").build()
            )

        @JvmStatic
        fun getImageRemoteUris(pack: Pack) = requireNotNull(pack.imageLocation).run {
            arrayOf(
                substringAfterLast('/').substringBeforeLast("_(").replace('_', ' ').let { stem ->
                    Cdn.vgmrips("${this}/${stem}.png")
                }, buildBaseUri("packs/images/large/${this}.png").build()
            )
        }
    }

    private fun readDoc(uri: Uri): Document = HtmlUtils.parseDoc(http.getInputStream(uri))

    private inner class CompaniesGrouping : Catalog.Grouping {

        override fun query(visitor: Catalog.Visitor<Group>) =
            readDoc(buildUri(COMPANIES_DIR_PATH))
                .select("table>tbody>tr>td.link>a[href*=net/packs/company]")
                .forEach { ref ->
                    val id = extractId(ref, COMPANIES_FILE_PREFIX)
                    val title = ref.attr("title")
                    val packs = tryGetPacksCountFromBadge(ref.parent?.parent, "a[href$=/developed]")
                    if (id.isNotEmpty() && title.isNotEmpty() && packs != 0) {
                        visitor.accept(Group(id, title, packs))
                    }
                }

        override fun queryPacks(
            id: String,
            visitor: Catalog.Visitor<Pack>,
            progress: ProgressCallback
        ) = parsePacks("${COMPANIES_FILE_PREFIX}${id}/developed", visitor, progress)
    }

    private inner class ComposersGrouping : Catalog.Grouping {

        override fun query(visitor: Catalog.Visitor<Group>) =
            readDoc(buildUri(COMPOSERS_DIR_PATH))
                .select("li.composer>a[href*=net/packs/composer/]")
                .forEach { ref ->
                    val id = extractId(ref, COMPOSERS_FILE_PREFIX)
                    val name = ref.ownText()
                    val packs = tryGetPacksCountFromBadge(ref)
                    if (id.isNotEmpty() && name.isNotEmpty() && packs != 0) {
                        visitor.accept(Group(id, name, packs))
                    }
                }

        override fun queryPacks(
            id: String,
            visitor: Catalog.Visitor<Pack>,
            progress: ProgressCallback
        ) = parsePacks("${COMPOSERS_FILE_PREFIX}${id}", visitor, progress)
    }

    private inner class ChipsGrouping : Catalog.Grouping {

        override fun query(visitor: Catalog.Visitor<Group>) {
            readDoc(buildUri(CHIPS_DIR_PATH))
                .select("div.chip>a[href*=net/packs/chip/]")
                .forEach { ref ->
                    val id = extractId(ref, CHIPS_FILE_PREFIX)
                    val name = ref.ownText()
                    val packs = tryGetPacksCountFromBadge(ref.parent)
                    if (id.isNotEmpty() && name.isNotEmpty() && packs != 0) {
                        visitor.accept(Group(id, name, packs))
                    }
                }
        }

        override fun queryPacks(
            id: String,
            visitor: Catalog.Visitor<Pack>,
            progress: ProgressCallback
        ) = parsePacks("${CHIPS_FILE_PREFIX}${id}", visitor, progress)
    }

    private inner class SystemsGrouping : Catalog.Grouping {

        override fun query(visitor: Catalog.Visitor<Group>) =
            readDoc(buildUri(SYSTEMS_DIR_PATH))
                .select("a[href*=net/packs/system/]:has(>img,div>span)")
                .forEach { ref ->
                    val id = extractId(ref, SYSTEMS_FILE_PREFIX)
                    val title = getSuffix(ref.attr("title"), SYSTEMS_TITLE_PREFIX)
                    val packs = tryGetPacksCountFromBadge(ref)
                    if (id.isNotEmpty() && title.isNotEmpty() && packs != 0) {
                        visitor.accept(Group(id, title, packs))
                    }
                }

        override fun queryPacks(
            id: String,
            visitor: Catalog.Visitor<Pack>,
            progress: ProgressCallback
        ) = parsePacks("${SYSTEMS_FILE_PREFIX}${id}", visitor, progress)
    }

    private fun parsePacks(
        path: String,
        visitor: Catalog.Visitor<Pack>,
        progress: ProgressCallback
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

private fun buildBaseUri(path: String) = Uri.Builder()
    .scheme("https")
    .authority("vgmrips.net")
    .path(path)

private fun buildUri(path: String) = buildBaseUri(path).build()

private fun buildPagedUri(path: String, page: Int) =
    buildBaseUri(path)
        .appendQueryParameter("p", page.toString())
        .build()

private fun extractId(ref: Element, prefix: String) =
    ref.attr("href").substringAfter(prefix, "")

private fun getSuffix(str: String, prefix: String): String = str.substringAfter(prefix, "")

private fun tryGetPacksCountFromBadge(el: Element?) = tryGetPacksCountFromBadge(el, "span.badge")

private fun tryGetPacksCountFromBadge(el: Element?, query: String) =
    el?.findFirstText(query)?.let {
        tryParseIntPrefix(it)
    } ?: 0

private fun tryParseIntPrefix(txt: String) = txt.takeWhile { it.isDigit() }.toIntOrNull() ?: 0

private fun parsePackTracks(result: Pack, visitor: Catalog.Visitor<Track>, doc: Element) =
    doc.select("table.playlist>tbody>tr:has(>td.title)")
        .forEach { track ->
            val number = tryParseIntPrefix(track.attr("data-order"))
            val title = track.findFirstText("td.title>a.beginPlay")
            val location = Uri.decode(getSuffix(track.attr("data-vgmurl"), "/packs/vgm/"))
            if (number > 0 && !title.isNullOrEmpty() && location.isNotEmpty()) {
                val duration =
                    TimeStamp.fromSeconds(tryParseIntPrefix(track.attr("data-duration")).toLong())
                visitor.accept(Track(number, title, duration, location))
                ++result.songs
            }
        }

private fun parsePack(el: Element) =
    el.findFirst("div.details")?.let { details ->
        tryParsePackFromDetails(details)
    }?.also { pack ->
        el.findFirst("div.image:has(>a:has(>img))")?.let {
            updateRating(pack, it)
            updateImage(pack, it, "/packs/images/small/")
        }
    }

private fun tryParsePackFromDetails(el: Element) =
    el.findFirst("a[href*=/packs/pack/]:gt(0)")?.let { ref ->
        tryMakePack(ref)
    }?.also { pack ->
        el.findFirstText("a.download[href*=/files/]")?.let { str ->
            //[X,songs,*,Y,KB]
            str.split(' ').takeIf { it.size == 5 }?.run {
                pack.songs = tryParseIntPrefix(first())
            }
        }
    }

private fun tryMakePack(el: Element) = with(el) {
    val id = getSuffix(attr("href"), "/packs/pack/")
    val title = text()
    tryMakePack(id, title)
}

private fun tryMakePack(id: String, title: String) =
    if (id.isNotEmpty() && title.isNotEmpty()) {
        Pack(id, title)
    } else {
        null
    }

private fun updateRating(pack: Pack, el: Element) = el.findFirst("div.stars")?.run {
    pack.score = parsePackScore(classNames())
    pack.ratings = nextElementSibling?.run { tryParseIntPrefix(text()) } ?: 0
}

private fun updateImage(pack: Pack, el: Element, imagePathPart: String) =
    el.findFirst("div.image>a>img[src*=packs/images/]")?.let { img ->
        pack.imageLocation = Uri.decode(getSuffix(img.attr("src"), imagePathPart))
            .removeSuffix(".png").removeSuffix(".PNG")
    }

private fun parsePackScore(classNames: Iterable<String>) = classNames
    .asSequence()
    .map { getSuffix(it, "stars") }
    .filter { it.isNotEmpty() }
    .map { tryParseIntPrefix(it) }
    .firstOrNull() ?: -1

private fun parsePacksProgress(el: Element) =
    el.findFirst("div.container:has(>div.clearfix)")?.run {
        // [Packs, XX, to, YY, of, ZZ, total]
        textNodes().asSequence()
            .map { it.text() }
            .filter { it.startsWith("Packs") }
            .map { it.split(' ') }
            .filter { it.size == 7 }
            .map {
                val done = HtmlUtils.tryGetInteger(it[3])
                val total = HtmlUtils.tryGetInteger(it[5])
                Pair(done, total)
            }
            .firstOrNull()
    }

