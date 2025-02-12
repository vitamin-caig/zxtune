package app.zxtune.fs.api

import android.net.Uri
import app.zxtune.BuildConfig

private val ROOT = Uri.parse(BuildConfig.PROXY_ROOT)

object Proxy {
    fun uriFor(uri: Uri): Uri = ROOT.buildUpon().apply {
        require(uri.scheme == "https")
        appendPath(uri.host)
        uri.encodedPath.takeUnless { it.isNullOrEmpty() }?.let {
            appendEncodedPath(it.substringAfter('/'))
        }
        uri.encodedQuery?.let {
            encodedQuery(it)
        }
    }.build()
}
