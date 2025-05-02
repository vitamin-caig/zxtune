package app.zxtune.coverart

import android.content.Context
import android.net.Uri
import app.zxtune.core.Identifier

class CoverartProviderClient(ctx: Context) {
    private val resolver = ctx.contentResolver

    fun getMediaUris(id: Identifier) =
        resolver.call(Query.rootUri, Provider.METHOD_GET_MEDIA_URIS, id.toString(), null)

    companion object {
        fun getIconUriFor(uri: Uri) = Query.uriFor(Query.Case.ICON, uri.toString())
    }
}
