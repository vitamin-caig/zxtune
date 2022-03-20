package app.zxtune.analytics.internal

import android.content.ContentResolver
import android.content.Context
import app.zxtune.Logger
import app.zxtune.utils.AsyncWorker

private val LOG = Logger(ProviderClient::class.java.name)

internal class ProviderClient(ctx: Context) : UrlsSink {

    private val worker = AsyncWorker("IASender")
    private val resolver: ContentResolver = ctx.contentResolver

    override fun push(url: String) {
        worker.execute { doPush(url) }
    }

    private fun doPush(msg: String) {
        try {
            resolver.call(Provider.URI, Provider.METHOD_PUSH, msg, null)
        } catch (e: Exception) {
            LOG.w(e) { "Failed to push url" }
        }
    }
}
