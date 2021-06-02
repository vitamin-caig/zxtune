package app.zxtune.fs

import android.net.Uri
import app.zxtune.io.Io.readFrom
import org.jsoup.Jsoup
import org.jsoup.nodes.Document
import org.jsoup.nodes.Element
import org.jsoup.nodes.Node
import java.io.IOException
import java.io.InputStream
import java.nio.charset.Charset

object HtmlUtils {
    @Throws(IOException::class)
    // Looks like jsoup works not well enough for streamed parsing when page size is more than
    // several kb
    @JvmStatic
    fun parseDoc(input: InputStream): Document = Jsoup.parse(readHtml(input), "")

    private fun readHtml(input: InputStream): String = input.use {
        with(readFrom(input)) {
            String(array(), 0, limit(), Charset.forName("UTF-8"))
        }
    }

    @JvmStatic
    fun getQueryInt(anchor: Element, param: String): Int? =
        Uri.parse(anchor.attr("href"))
            .getQueryParameter(param)?.let { tryGetInteger(it) }

    @JvmStatic
    fun tryGetInteger(raw: String?): Int? = raw?.trim()?.toIntOrNull()
}

object Jsoup {
    internal fun Element.findFirst(cssSelector: String): Element? = selectFirst(cssSelector)
    internal fun Element.findFirstText(cssSelector: String): String? =
        findFirst(cssSelector)?.text()

    internal val Element.parent: Element? get() = parent()
    internal val Element.nextSibling: Node? get() = nextSibling()
    internal val Element.nextElementSibling: Element? get() = nextElementSibling()
}
