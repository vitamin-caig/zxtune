/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.amp

import android.net.Uri
import app.zxtune.Logger
import app.zxtune.fs.HtmlUtils
import app.zxtune.fs.Jsoup.findFirst
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.http.MultisourceHttpProvider
import org.jsoup.nodes.Document

/**
 * Authors:
 * http://amp.dascene.net/newresult.php?request=list&search=${letter}&position=${offset} letter='0-9',a..z
 * <p>
 * references to author's info at
 * http://amp.dascene.net/detail.php?view=${authorid}
 * <p>
 * references to author's track at
 * http://amp.dascene.net/detail.php?detail=modules&view=${authorid}
 * <p>
 * references tracks at
 * http://amp.dascene.net/downmod.php?index=${trackid}
 * <p>
 * Countries:
 * http://amp.dascene.net/newresult.php?request=country&search=${countryid} id=1..64
 * <p>
 * Groups:
 * http://amp.dascene.net/newresult.php?request=groupid&search=${groupid} id=1..7149
 * <p>
 * list all:
 * http://amp.dascene.net/newresult.php?request=groups
 * <p>
 * Search:
 * http://amp.dascene.net/newresult.php?request=module&search=${whatever}
 */

private val LOG = Logger(RemoteCatalog::class.java.name)

class RemoteCatalog(val http: MultisourceHttpProvider) : Catalog {

    override fun queryGroups(visitor: Catalog.GroupsVisitor) =
        getQueryUriBuilder("groups", "").build().let { uri ->
            LOG.d { "queryGroups()" }
            readDoc(uri).select("tr>td>a[href^=newresult.php?request=groupid]").forEach { el ->
                HtmlUtils.getQueryInt(el, "search")?.let { id ->
                    visitor.accept(Group(id, el.text()))
                }
            }
        }

    override fun queryAuthors(handleFilter: String, visitor: Catalog.AuthorsVisitor) =
        getQueryUriBuilder("list", handleFilter).let { uri ->
            LOG.d { "queryAuthors(handleFilter=${handleFilter})" }
            queryAuthorsInternal(uri, visitor)
        }

    override fun queryAuthors(country: Country, visitor: Catalog.AuthorsVisitor) =
        getQueryUriBuilder("country", country.id.toString()).let { uri ->
            LOG.d { "queryAuthors(country=${country.id})" }
            queryAuthorsInternal(uri, visitor)
        }

    override fun queryAuthors(group: Group, visitor: Catalog.AuthorsVisitor) =
        getQueryUriBuilder("groupid", group.id.toString()).let { uri ->
            LOG.d { "queryAuthors(group=${group.id})" }
            queryAuthorsInternal(uri, visitor)
        }

    private fun queryAuthorsInternal(uri: Uri.Builder, visitor: Catalog.AuthorsVisitor) =
        loadPages(uri) { doc ->
            parseAuthors(doc, visitor)
        }

    override fun queryTracks(author: Author, visitor: Catalog.TracksVisitor) =
        getAuthorTracksUri(author.id).let { uri ->
            LOG.d { "queryTracks(author=${author.id})" }
            readDoc(uri).select("table>tbody>tr:has(>td>a[href^=downmod.php])").forEach { el ->
                val nameEl = el.child(0).child(0)
                val sizeEl = el.child(3)
                val id = HtmlUtils.getQueryInt(nameEl, "index") ?: return@forEach
                val size = getSize(sizeEl.text()) ?: return@forEach
                visitor.accept(Track(id, nameEl.text(), size))
            }
        }

    fun searchSupported() = http.hasConnection()

    override fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) =
        getQueryUriBuilder("module", query).let { uri ->
            LOG.d { "findTracks(query=${query})" }
            loadPages(uri)
            { doc ->
                parseFoundTracks(doc, visitor)
            }
        }

    companion object {
        @JvmStatic
        fun getTrackUris(id: Int) = arrayOf(
            Cdn.amp(id),
            getMainUriBuilder()
                .appendPath("downmod.php")
                .appendQueryParameter("index", id.toString())
                .build()
        )
    }

    private fun interface PagesVisitor {
        fun onPage(doc: Document)
    }

    private fun loadPages(query: Uri.Builder, visitor: PagesVisitor) {
        var offset = 0
        while (true) {
            val doc = readDoc(getPageUrl(query, offset))
            val paginator =
                doc.findFirst("table>caption")?.takeIf { it.childrenSize() <= 2 } ?: break
            visitor.onPage(doc)

            val next = paginator.findFirst(
                "a[href^=newresult.php?request=]:has(img[src$=/right.gif])"
            )?.let { HtmlUtils.getQueryInt(it, "position") } ?: break
            if (next == offset) {
                break
            }
            offset = next
        }
    }

    private fun readDoc(base: Uri) =
        HtmlUtils.parseDoc(http.getInputStream(base))
}

private fun getPageUrl(base: Uri.Builder, start: Int) =
    base.appendQueryParameter("position", start.toString()).build().also {
        LOG.d { "Load page: $it" }
    }

private fun getMainUriBuilder() =
    Uri.Builder()
        .scheme("https")
        .authority("amp.dascene.net")

private fun getQueryUriBuilder(request: String, search: String) =
    getMainUriBuilder()
        .appendPath("newresult.php")
        .appendQueryParameter("request", request)
        .appendQueryParameter("search", search)

private fun getAuthorTracksUri(id: Int) =
    getMainUriBuilder()
        .appendPath("detail.php")
        .appendQueryParameter("detail", "modules")
        .appendQueryParameter("view", id.toString())
        .build()

private fun parseAuthors(doc: Document, visitor: Catalog.AuthorsVisitor) {
    for (el in doc.select("table:has(>caption)>tbody>tr>td>table>tbody")) {
        val handleEl = el.findFirst("a[href^=detail.php]") ?: continue
        val realNameEl = el.findFirst("td:containsOwn(Real Name) + td") ?: continue
        val id = HtmlUtils.getQueryInt(handleEl, "view") ?: continue
        val name = handleEl.text()
        val realName = realNameEl.text()
        visitor.accept(Author(id, name, realName))
    }
}

private fun parseFoundTracks(doc: Document, visitor: Catalog.FoundTracksVisitor) {
    for (el in doc.select("table:has(>caption)>tbody>tr:has(>td>a[href^=downmod.php])")) {
        val nameEl = el.child(0).child(0)
        val authorEl = el.child(1).child(0)
        val sizeEl = el.child(3)
        val trackId = HtmlUtils.getQueryInt(nameEl, "index") ?: continue
        val trackName = nameEl.text()
        val authorId = HtmlUtils.getQueryInt(authorEl, "view") ?: continue
        val authorHandle = authorEl.text()
        val size = getSize(sizeEl.text()) ?: continue
        visitor.accept(
            Author(authorId, authorHandle, ""),
            Track(trackId, trackName, size)
        )
    }
}

private fun getSize(sizeKb: String) = if (sizeKb.endsWith("Kb")) {
    HtmlUtils.tryGetInteger(sizeKb.dropLast(2))
} else {
    null
}
