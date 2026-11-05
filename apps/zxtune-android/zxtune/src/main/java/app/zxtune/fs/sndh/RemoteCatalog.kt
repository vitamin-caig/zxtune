package app.zxtune.fs.sndh

import android.net.Uri
import androidx.core.util.Consumer
import app.zxtune.fs.HtmlUtils
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.http.MultisourceHttpProvider
import org.jsoup.nodes.Document
import org.jsoup.nodes.Element

private const val HOST = "sndh.atari.org"

class RemoteCatalog(private val http: MultisourceHttpProvider) : Catalog {

    override fun queryAuthors(visitor: Consumer<Author>) = readPage("composers")
        .select("a[href*='p=composer&name=']").forEach { ref ->
            val name = getQueryParameter(ref, "name") ?: return@forEach
            if (name.isNotEmpty()) {
                visitor.accept(Author(name, findFilesCount(ref)))
            }
        }

    override fun queryTracks(
        author: Author,
        visitor: Consumer<Track>,
    ) = readPage("composer", "name" to author.name).let { doc ->
        doc.select("tr:has(>td>small>a[data-tuneid])").forEach { row ->
            val id = row.selectFirst("a[href^='dl.php?ID=']")?.let {
                HtmlUtils.getQueryInt(it, "ID")
            } ?: return@forEach
            val title = row.selectFirst("a[href^='/?ID=']")?.text()
                ?.takeIf { it.isNotBlank() } ?: return@forEach
            val size = row.select("td").getOrNull(5)?.text().orEmpty()
            visitor.accept(Track(id, title, size))
        }
    }

    fun searchSupported() = http.hasConnection()

    override fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) = readPage("searchdone", "searchword" to query).let { doc ->
        doc.select("tr:has(>td>small>a[data-tuneid])").forEach { row ->
            val id = row.selectFirst("a[href^='dl.php?ID=']")?.let {
                HtmlUtils.getQueryInt(it, "ID")
            } ?: return@forEach
            val title = row.selectFirst("a[href^='/?ID=']")?.text()
                ?.takeIf { it.isNotBlank() } ?: return@forEach
            val author = row.selectFirst("a[href^='/?p=composer&name=']")
                ?.let { getQueryParameter(it, "name") }
                ?.takeIf { it.isNotEmpty() } ?: return@forEach
            // Search results have no file size, so keep it empty
            visitor.accept(Author(author), Track(id, title, ""))
        }
    }

    override fun findRandomTrack(): Track? {
        val doc = readPage("random")
        val id = doc.selectFirst("a[data-tuneid]")?.attr("data-tuneid")?.toIntOrNull()
            ?: return null
        val title = doc.findDetail("Title") ?: return null
        val size = doc.findDetail("File size").orEmpty()
        return Track(id, title, size)
    }

    private fun getQueryParameter(ref: Element, name: String) = Uri.parse("https://$HOST/${ref.attr("href").trimStart('/')}").getQueryParameter(name)

    private fun findFilesCount(ref: Element): Int = ref.parents().firstOrNull { it.tagName() == "tr" }?.select("td")?.getOrNull(1)?.text()
        ?.takeWhile(Char::isDigit)?.toIntOrNull() ?: 0

    private fun Document.findDetail(name: String) = select("tr").firstOrNull { row ->
        row.select("td").firstOrNull()?.text() == name
    }?.select("td")?.getOrNull(1)?.text()

    private fun readPage(page: String, vararg params: Pair<String, String>) = Uri.Builder().scheme("https").authority(HOST).appendQueryParameter("p", page).apply {
        params.forEach { (name, value) -> appendQueryParameter(name, value) }
    }.build().let {
        HtmlUtils.parseDoc(http.getInputStream(it))
    }

    companion object {
        fun getTrackUris(id: Int) = iterator {
            yield(Cdn.sndh(id))
            yield(Uri.parse("https://$HOST/dl.php?ID=$id"))
        }
    }
}
