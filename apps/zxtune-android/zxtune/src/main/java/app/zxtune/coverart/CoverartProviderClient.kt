package app.zxtune.coverart

import android.net.Uri

object CoverartProviderClient {
    fun getUriFor(uri: Uri) = Query.uriFor(uri)
}
