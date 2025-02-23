/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxart

import android.net.Uri
import android.sax.Element
import android.sax.RootElement
import android.text.Html
import app.zxtune.Logger
import app.zxtune.Util
import app.zxtune.fs.HtmlUtils.tryGetInteger
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.Xml
import app.zxtune.utils.ifNotNulls
import java.io.BufferedInputStream

private val LOG = Logger(RemoteCatalog::class.java.name)

open class RemoteCatalog(private val http: MultisourceHttpProvider) : Catalog {

    override fun queryAuthors(visitor: Catalog.Visitor<Author>) {
        LOG.d { "queryAuthors()" }
        val root = createAuthorsParserRoot(visitor)
        performQuery(Uris.forAuthors(), root)
    }

    override fun queryAuthorTracks(author: Author, visitor: Catalog.Visitor<Track>) {
        LOG.d { "queryAuthorTracks(author=${author.id})" }
        queryTracks(visitor, Uris.forTracksOf(author))
    }

    override fun queryParties(visitor: Catalog.Visitor<Party>) {
        LOG.d { "queryParties()" }
        val root = createPartiesParserRoot(visitor)
        performQuery(Uris.forParties(), root)
    }

    override fun queryPartyTracks(party: Party, visitor: Catalog.Visitor<Track>) {
        LOG.d { "queryPartyTracks(party=${party.id}" }
        queryTracks(visitor, Uris.forTracksOf(party))
    }

    override fun queryTopTracks(limit: Int, visitor: Catalog.Visitor<Track>) {
        LOG.d { "queryTopTracks()" }
        queryTracks(visitor, Uris.forTop(limit))
    }

    private fun queryTracks(visitor: Catalog.Visitor<Track>, uri: Uri) {
        val root = createModulesParserRoot(visitor)
        performQuery(uri, root)
    }

    open fun searchSupported() = http.hasConnection()

    override fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) {
        LOG.d { "findTracks(query=${query})" }
        val root = createModulesParserRoot(visitor)
        performQuery(Uris.forSearch(query), root)
    }

    private fun performQuery(uri: Uri, root: RootElement) =
        http.getInputStream(uri).let { httpStream ->
            Xml.parse(BufferedInputStream(httpStream), root.contentHandler)
        }

    companion object {
        @JvmStatic
        fun getTrackUris(id: Int) = arrayOf(Uris.forDownload(id))
    }
}

private class Uris {
    private val delegate = Uri.Builder()

    init {
        delegate.scheme("https").authority("zxart.ee") // no www. prefix!
    }

    fun action(action: String) = apply {
        delegate.appendPath("zxtune").appendEncodedPath("language:eng")
            .appendEncodedPath("action:${action}")
    }

    fun field(type: String, id: Int) = apply {
        delegate.appendEncodedPath("${type}:${id}")
    }

    fun field(type: String, value: String) = apply {
        delegate.appendEncodedPath("${type}:${Uri.encode(value)}")
    }

    fun download(id: Int) = run {
        delegate.appendPath("file")
        field("id", id)
    }

    fun build(): Uri = delegate.build()

    companion object {
        fun forAuthors() = Uris().action("authors").build()

        fun forTracksOf(author: Author) = forTracksOf("authorId", author.id)

        fun forParties() = Uris().action("parties").build()

        fun forTracksOf(party: Party) = forTracksOf("partyId", party.id)

        fun forTop(limit: Int) = Uris().action("topTunes").field("limit", limit).build()

        fun forSearch(query: String) = Uris().action("search").field("query", query).build()

        private fun forTracksOf(scope: String, id: Int) =
            Uris().action("tunes").field(scope, id).build()

        fun forDownload(id: Int) = Uris().download(id).build()
    }
}

private class AuthorBuilder {
    private var id: Int? = null
    private var nickname: String? = null
    private var name: String? = null

    fun setId(value: String?) {
        id = tryGetInteger(value)
    }

    fun setNickname(value: String?) {
        nickname = value
    }

    fun setName(value: String?) {
        name = value
    }

    fun captureResult() = ifNotNulls(id, nickname, name.orEmpty(), ::Author).also {
        id = null
        nickname = null
        name = null
    }
}

private class PartiesBuilder {
    private var id: Int? = null
    private var name: String? = null
    private var year = 0

    fun setId(value: String?) {
        id = tryGetInteger(value)
    }

    fun setName(value: String?) {
        name = value
    }

    fun setYear(value: String?) {
        year = tryGetInteger(value) ?: 0
    }

    fun captureResult() = ifNotNulls(id, name, year, ::Party).also {
        id = null
        name = null
        year = 0
    }
}

private class ModuleBuilder {
    private var id: Int? = null
    private var filename: String? = null
    private var title = ""
    private var internalAuthor = ""
    private var internalTitle = ""
    private var votes = ""
    private var duration = ""
    private var year = 0
    private var compo: String = ""
    private var partyplace = 0
    private var authorId: Int? = null

    init {
        reset()
    }

    fun setId(value: String?) {
        id = value?.toIntOrNull()
    }

    fun setFilename(value: String?) {
        filename = value?.trim()?.takeIf { it.isNotEmpty() }?.let { Html.fromHtml(it).toString() }
            ?: "unknown"
    }

    fun setTitle(value: String?) {
        title = value.orEmpty()
    }

    fun setInternalAuthor(value: String?) {
        internalAuthor = value.orEmpty()
    }

    fun setInternalTitle(value: String?) {
        internalTitle = value.orEmpty()
    }

    fun setVotes(value: String?) {
        votes = value.orEmpty()
    }

    fun setDuration(value: String?) {
        duration = value.orEmpty()
    }

    fun setYear(value: String?) {
        year = tryGetInteger(value) ?: 0
    }

    fun setCompo(value: String?) {
        compo = value.orEmpty()
    }

    fun setPartyplace(value: String?) {
        partyplace = tryGetInteger(value) ?: 0
    }

    fun setAuthorId(value: String?) {
        authorId = tryGetInteger(value)
    }

    fun captureResultAuthor() = ifNotNulls(authorId, "Author$authorId", "", ::Author).also {
        authorId = null
    }

    fun captureResult() = ifNotNulls(id, filename) { id, filename ->
        Track(
            id,
            filename,
            Util.formatTrackTitle(internalAuthor, internalTitle, title),
            votes,
            duration,
            year,
            compo,
            partyplace
        )
    }.also {
        reset()
    }

    private fun reset() {
        id = null
        filename = null
        year = 0
        partyplace = 0
        votes = ""
        duration = ""
        title = ""
        internalAuthor = ""
        internalTitle = ""
        compo = ""
        authorId = null
    }
}

private fun createAuthorsParserRoot(visitor: Catalog.Visitor<Author>) = createRootElement().apply {
    getChild("responseData").run {
        getChild("authors").getChild("author").run {
            val builder = AuthorBuilder()
            setEndElementListener {
                builder.captureResult()?.let { res ->
                    visitor.accept(res)
                }
            }
            getChild("id").setEndTextElementListener(builder::setId)
            getChild("title").setEndTextElementListener(builder::setNickname)
            getChild("realName").setEndTextElementListener(builder::setName)
        }
    }
}

private fun createPartiesParserRoot(visitor: Catalog.Visitor<Party>) = createRootElement().apply {
    getChild("responseData").run {
        getChild("parties").getChild("party").run {
            val builder = PartiesBuilder()
            setEndElementListener {
                builder.captureResult()?.let { res ->
                    visitor.accept(res)
                }
            }
            getChild("id").setEndTextElementListener(builder::setId)
            getChild("title").setEndTextElementListener(builder::setName)
            getChild("year").setEndTextElementListener(builder::setYear)
        }
    }
}

private fun createModulesParserRoot(visitor: Catalog.Visitor<Track>) =
    createModulesParserRoot { builder ->
        builder.captureResult()?.let {
            visitor.accept(it)
        }
    }

private fun createModulesParserRoot(visitor: Catalog.FoundTracksVisitor) =
    createModulesParserRoot { builder ->
        builder.captureResultAuthor()?.let { author ->
            builder.captureResult()?.let { track ->
                visitor.accept(author, track)
            }
        }
    }

private fun createModulesParserRoot(
    callback: (ModuleBuilder) -> Unit
) = createRootElement().apply {
    getChild("responseData").run {
        getChild("tunes").getChild("tune").run {
            val builder = ModuleBuilder()
            setEndElementListener {
                callback(builder)
            }
            bindXmlActions(this, builder)
        }
    }
}

private fun bindXmlActions(item: Element, builder: ModuleBuilder) = with(item) {
    getChild("id").setEndTextElementListener(builder::setId)
    //CDATA
    getChild("originalFileName").setEndTextElementListener(builder::setFilename)
    getChild("title").setEndTextElementListener(builder::setTitle)
    getChild("internalAuthor").setEndTextElementListener(builder::setInternalAuthor)
    getChild("internalTitle").setEndTextElementListener(builder::setInternalTitle)
    getChild("votes").setEndTextElementListener(builder::setVotes)
    getChild("time").setEndTextElementListener(builder::setDuration)
    getChild("year").setEndTextElementListener(builder::setYear)
    getChild("compo").setEndTextElementListener(builder::setCompo)
    getChild("partyplace").setEndTextElementListener(builder::setPartyplace)
    getChild("authors").getChild("id").setEndTextElementListener(builder::setAuthorId)
}

private fun createRootElement() = RootElement("response")
