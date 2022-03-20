/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modland

import android.net.Uri
import app.zxtune.Logger
import app.zxtune.fs.HtmlUtils
import app.zxtune.fs.Jsoup.findFirstText
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import app.zxtune.utils.StubProgressCallback
import org.jsoup.nodes.Document
import java.io.IOException
import java.util.regex.Pattern

/**
 * There's additional api gate for xbmc plugin at
 * http://www.exotica.org.uk/mediawiki/extensions/ExoticASearch/Modland_xbmc.php
 * but it seems to be not working and has no such wide catalogue as http gate does.
 */

private val LOG = Logger(RemoteCatalog::class.java.name)
private val PAGINATOR: Pattern =
    Pattern.compile("Browsing (.+?) - (\\d+) results? - showing page (\\d+) of (\\d+).")

open class RemoteCatalog(val http: MultisourceHttpProvider) : Catalog {

    private val authors by lazy { BaseGrouping("aut", "Modules from author ") }
    private val collections by lazy { BaseGrouping("col", "Modules from collection ") }
    private val formats by lazy { BaseGrouping("for", "Modules in format ") }

    override fun getAuthors(): Catalog.Grouping = authors

    override fun getCollections(): Catalog.Grouping = collections

    override fun getFormats(): Catalog.Grouping = formats

    private inner class BaseGrouping(val tag: String, val headerPrefix: String) : Catalog.Grouping {
        override fun queryGroups(
            filter: String,
            visitor: Catalog.GroupsVisitor,
            progress: ProgressCallback
        ) {
            LOG.d { "queryGroups(type=${tag}, filter=${filter})" }
            loadPages(getGroupsUriBuilder(tag, filter), object : PagesVisitor {

                private var total = 0
                private var done = 0

                override fun onPage(title: String, results: Int, doc: Document): Boolean {
                    if (total == 0) {
                        total = results
                        visitor.setCountHint(total)
                    }
                    done += parseGroups(doc, visitor)
                    progress.onProgressUpdate(done, total)
                    return true
                }
            })
        }

        override fun getGroup(id: Int): Group {
            LOG.d { "getGroup(type=${tag}, id=${id})" }
            val resultRef = arrayOfNulls<Group>(1)
            loadPages(getTracksUriBuilder(tag, id)) { header, results, _ ->
                if (header.startsWith(headerPrefix)) {
                    val name = header.substring(headerPrefix.length)
                    resultRef[0] = Group(id, name, results)
                }
                false
            }
            return resultRef[0] ?: throw IOException("Failed to find group with id=${id}")
        }

        override fun queryTracks(
            id: Int,
            visitor: Catalog.TracksVisitor,
            progress: ProgressCallback
        ) {
            LOG.d { "queryGroupTracks(type=${tag}, id=${id})" }
            loadPages(getTracksUriBuilder(tag, id), object : PagesVisitor {

                private var total = 0
                private var done = 0

                override fun onPage(title: String, results: Int, doc: Document): Boolean {
                    if (total == 0) {
                        total = results
                        visitor.setCountHint(total)
                    }
                    val tracks = parseTracks(doc, visitor)
                    done += tracks
                    progress.onProgressUpdate(done, total)
                    return tracks != 0
                }
            })
        }

        override fun getTrack(id: Int, filename: String): Track {
            LOG.d { "getGroupTrack(type=${tag}, id=${id}, filename=${filename})" }
            val resultRef = arrayOfNulls<Track>(1)
            queryTracks(id, {
                if (it.filename == filename) {
                    resultRef[0] = it
                    false
                } else {
                    true
                }
            }, StubProgressCallback.instance())
            return resultRef[0]
                ?: throw IOException("Failed to get track '${filename}' with id=${id}")
        }
    }

    companion object {

        @JvmStatic
        fun getTrackUris(id: String) = arrayOf(
            Cdn.modland(id),
            Uri.parse("http://ftp.amigascne.org/mirrors/ftp.modland.com$id")
        )
    }

    private fun interface PagesVisitor {
        fun onPage(title: String, results: Int, doc: Document): Boolean
    }

    private fun loadPages(query: Uri.Builder, visitor: PagesVisitor) {
        for (pg in 1..Int.MAX_VALUE) {
            val doc = HtmlUtils.parseDoc(readPage(query, pg))
            val hdr = doc.findFirstText("table>caption") ?: break
            val matcher = PAGINATOR.matcher(hdr)
            if (!matcher.find()) break
            LOG.d { "Load page: ${hdr}" }
            val header = matcher.group(1) ?: break
            val results = matcher.group(2) ?: break
            val page = matcher.group(3) ?: break
            val pagesTotal = matcher.group(4) ?: break
            if (pg != page.toInt()) {
                throw IOException("Invalid paginator structure")
            }
            if (visitor.onPage(header, results.toInt(), doc) && pg < pagesTotal.toInt()) {
                continue
            }
            break
        }
    }

    private fun readPage(base: Uri.Builder, start: Int) =
        base.appendQueryParameter("pg", start.toString()).build().let { uri ->
            http.getInputStream(uri)
        }
}

private fun getMainUriBuilder() = Uri.Builder()
    .scheme("https")
    .authority("www.exotica.org.uk")
    .path("mediawiki/index.php")
    .appendQueryParameter("title", "Special:Modland")

private fun getGroupsUriBuilder(type: String, filter: String) =
    getMainUriBuilder()
        .appendQueryParameter("md", "b_${type}")
        .appendQueryParameter("st", filter)

private fun getTracksUriBuilder(type: String, id: Int) =
    getMainUriBuilder()
        .appendQueryParameter("md", type)
        .appendQueryParameter("id", id.toString())

private fun parseGroups(doc: Document, visitor: Catalog.GroupsVisitor): Int {
    var result = 0
    for (el in doc.select("table:has(>caption)>tbody>tr:has(>td>a[href*=md=])")) {
        val nameEl = el.child(0).child(0)
        val countEl = el.child(1)
        val id = HtmlUtils.getQueryInt(nameEl, "id") ?: continue
        val name = nameEl.text()
        val tracks = HtmlUtils.tryGetInteger(countEl.text()) ?: continue
        if (name.isNotEmpty()) {
            visitor.accept(Group(id, name, tracks))
            ++result
        }
    }
    return result
}

private fun parseTracks(doc: Document, visitor: Catalog.TracksVisitor): Int {
    var result = 0
    for (el in doc.select("table:has(>caption)>tbody>tr:has(>td>a[href*=file=pub/])")) {
        val pathEl = el.child(0).child(0)
        val sizeEl = el.child(4)
        val href = pathEl.attr("href")
        val pathPos = href.indexOf("pub/modules/")
        val path = "/${href.substring(pathPos)}"
        val size = HtmlUtils.tryGetInteger(sizeEl.text()) ?: continue
        if (!visitor.accept(Track(path, size))) {
            return 0
        }
        ++result
    }
    return result
}
