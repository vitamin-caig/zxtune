package app.zxtune.coverart

import android.content.ContentResolver
import android.content.UriMatcher
import android.net.Uri
import app.zxtune.BuildConfig

/*
 * ${path} is full data uri (including subpath in fragment) stored as string
 *
 * content://${authority}/raw/${imageid} - get blob for existing image
 * content://${authority}/res/${drawableRes} - get bitmap from drawable
 * content://${authority}/{image,icon}/${path} - optional blob for object by path
 */

internal object Query {

    private const val AUTHORITY = "${BuildConfig.APPLICATION_ID}.coverart"
    private val baseUriBuilder
        get() = Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(AUTHORITY)

    enum class Case(val path: String) {
        RAW("raw"), RES("res"), IMAGE("image"), ICON("icon"),
    }

    private val uriTemplate = UriMatcher(UriMatcher.NO_MATCH).apply {
        Case.entries.forEach {
            addURI(AUTHORITY, "${it.path}/*", it.ordinal)
        }
    }

    val rootUri: Uri = baseUriBuilder.build()

    fun getCase(uri: Uri) = Case.entries.getOrNull(uriTemplate.match(uri))

    fun uriFor(case: Case, id: String): Uri =
        baseUriBuilder.appendPath(case.path).appendPath(id).build()

    fun idFrom(uri: Uri) = uri.pathSegments.getOrNull(1)
}
