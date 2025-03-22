package app.zxtune.coverart

import android.content.Context
import app.zxtune.core.Identifier

class CoverartProviderClient(ctx: Context) {
    private val resolver = ctx.contentResolver

    fun getMediaUris(id: Identifier) =
        resolver.call(Query.rootUri, Provider.METHOD_GET_MEDIA_URIS, id.toString(), null)
}
