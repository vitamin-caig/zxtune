package app.zxtune.fs.httpdir

import android.net.Uri
import app.zxtune.Util
import app.zxtune.fs.HtmlUtils
import app.zxtune.fs.Jsoup.findFirst
import app.zxtune.fs.Jsoup.findFirstText
import app.zxtune.fs.Jsoup.nextSibling
import app.zxtune.fs.http.MultisourceHttpProvider
import org.jsoup.nodes.Document
import org.jsoup.nodes.Element
import java.io.IOException
import java.io.InputStream

open class RemoteCatalog(protected val http: MultisourceHttpProvider) : Catalog {

    override fun parseDir(path: Path, visitor: Catalog.DirVisitor) =
        parseDir(http.getInputStream(path.getRemoteUris()), visitor)

    companion object {
        // used in testing
        @JvmStatic
        fun parseDir(input: InputStream, visitor: Catalog.DirVisitor) =
            with(HtmlUtils.parseDoc(input)) {
                parseDir(this, visitor)
            }
    }
}

private fun parseDir(doc: Document, visitor: Catalog.DirVisitor) {
    if (parseXmlIndex(doc, visitor) ||
        parseTableMarkup(doc, visitor) ||
        parsePreMarkup(doc, visitor)
    ) {
        return
    } else {
        throw IOException("Unsupported format of html page")
    }
}

/*
<list>
  <file mtime="YYYY-MM-DDThh:mm:ssZ" size="bytes">name</file>
  <directory mtime="YYYY-MM-DDThh:mm:ssZ">name</directory>
</list>
 */
private fun parseXmlIndex(doc: Document, visitor: Catalog.DirVisitor): Boolean {
    val list = doc.findFirst("list") ?: return false
    list.select("directory").forEach { row ->
        visitor.acceptDir(
            name = row.text(),
            description = row.attr("mtime")
        )
    }
    list.select("file").forEach { row ->
        visitor.acceptFile(
            name = row.text(),
            description = row.attr("mtime"),
            size = Util.formatSize(row.attr("size").toLongOrNull() ?: 0)
        )
    }
    return true
}

/*
    <table>
       ...
       <tr>
         ...
         <td>
           ...
           <a href="%-encoded name">name</a>
         </td>
         ...
         <td>
           ...
             " size "
           ...
         </td>
         ...
       </tr>
       ...
     </table>
 */
private fun parseTableMarkup(doc: Document, visitor: Catalog.DirVisitor): Boolean {
    var result = false
    for (row in doc.select("table tr:not(th)")) {
        val ref = row.findFirst("td a") ?: continue
        result = true
        val nameVal = findEntryHref(ref) ?: continue
        val name = trimTrailingSlash(nameVal)
        if (name == ".." || name == ".") {
            continue
        }
        val description = row.findFirstText(
            "td:has(a) ~ td:matches(^ *([0-9]{4}-" +
                    "([01][0-9]|[JFMASOND][a-z][a-z])-[0-3][0-9] " +
                    "[0-5][0-9](:[0-5][0-9])+) *$)"
        )?.trim() ?: ""
        row.findFirst("td:has(a) ~ td:matches(^ *([0-9.]+[KM]?) *$)")?.let { size ->
            visitor.acceptFile(name, description, size.text().trim())
        } ?: visitor.acceptDir(name, description)
    }
    return result
}

/*
    <pre>
      ...
      <img ...><a href="%-encoded">name</a>... "date time size"
      ...
    </pre>
*/
const val DIR_SIZE_MARKER = "-"

private fun parsePreMarkup(doc: Document, visitor: Catalog.DirVisitor): Boolean {
    var result = false
    for (name in doc.select("pre > img + a")) {
        val nameVal = findEntryHref(name) ?: continue
        val dateTimeSize = name.nextSibling?.toString()?.trim() ?: continue
        val lastSpace = dateTimeSize.lastIndexOf(" ")
        val dateTimeVal = dateTimeSize.substring(0, lastSpace).trim()
        val sizeVal = dateTimeSize.substring(lastSpace + 1).trim()
        if (DIR_SIZE_MARKER == sizeVal) {
            visitor.acceptDir(trimTrailingSlash(nameVal), dateTimeVal)
        } else {
            visitor.acceptFile(nameVal, dateTimeVal, sizeVal)
        }
        result = true
    }
    return result
}

private fun findEntryHref(anchor: Element): String? {
    val href = Uri.decode(anchor.attr("href"))
    val text = anchor.text()
    //text may be truncated/ellipsized/etc
    val minMatchedSymbols = 16
    val toMatch = minOf(href.length, text.length, minMatchedSymbols)
    return href.takeIf { href.regionMatches(0, text, 0, toMatch) }
}

private fun trimTrailingSlash(str: String) = str.removeSuffix("/")
