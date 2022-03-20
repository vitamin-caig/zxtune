package app.zxtune.fs.aminet

import android.net.Uri
import app.zxtune.fs.HtmlUtils
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.httpdir.Catalog
import app.zxtune.fs.httpdir.Path

class RemoteCatalog(http: MultisourceHttpProvider) : app.zxtune.fs.httpdir.RemoteCatalog(http) {

    override fun parseDir(path: Path, visitor: Catalog.DirVisitor) =
        if (path.getParent() == null) {
            parseRoot(visitor)
        } else {
            parseDir(path.getLocalId(), visitor)
        }

    private fun parseRoot(visitor: Catalog.DirVisitor) = HtmlUtils.parseDoc(readRoot())
        .select("li:has(a[href]:containsOwn(mods))").forEach { el ->
            // mods/[author] - [desc]
            val line = el.text().takeIf { it.startsWith("mods/") } ?: return@forEach
            val separator = line.indexOf(" - ")
            visitor.acceptDir(
                name = line.substring(5, separator),
                description = line.substring(separator + 3)
            )
        }

    private fun readRoot() =
        getBaseUri()
            .path("tree")
            .appendQueryParameter("path", "mods")
            .build()
            .let { uri ->
                http.getInputStream(uri)
            }

    private fun parseDir(path: String, visitor: Catalog.DirVisitor) =
        getBaseUri()
            .path("search")
            .appendQueryParameter("path", "mods/${path}")
            .let { uri ->
                parseDir(uri, visitor)
            }

    fun searchSupported() = http.hasConnection()

    fun find(query: String, visitor: Catalog.DirVisitor) =
        getBaseUri()
            .path("search")
            .appendQueryParameter("name", query)
            .appendQueryParameter("q_desc", "OR")
            .appendQueryParameter("desc", query)
            .appendQueryParameter("path[]", "mods")
            .let { uri ->
                parseDir(uri, visitor)
            }

    private fun parseDir(base: Uri.Builder, visitor: Catalog.DirVisitor) {
        //http://aminet.net/search?name=disco&path[]=mods&q_desc=OR&desc=disco
        var start = 0
        while (true) {
            HtmlUtils.parseDoc(readPage(base, start))
                .select("tr.lightrow,tr.darkrow")
                .also { list ->
                    if (list.isEmpty()) {
                        return@parseDir
                    } else {
                        start += list.size
                    }
                }
                .forEach { el ->
                    val tds = el.select("td>font")
                    val path = tds[0].child(0).attr("href")
                    if (path.startsWith("/mods/")) {
                        visitor.acceptFile(
                            name = path.substring(5),
                            description = tds[7].child(0).text(),
                            size = tds[4].text()
                        )
                    }
                }
        }
    }

    private fun readPage(base: Uri.Builder, start: Int) = base
        .appendQueryParameter("start", start.toString())
        .build()
        .let { uri ->
            http.getInputStream(uri)
        }
}

private fun getBaseUri() = Uri.Builder()
    .scheme("http")
    .authority("aminet.net")

