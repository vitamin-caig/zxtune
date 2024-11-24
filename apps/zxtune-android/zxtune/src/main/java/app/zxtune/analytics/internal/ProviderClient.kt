package app.zxtune.analytics.internal

import android.content.Context
import app.zxtune.Logger
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Semaphore

private val LOG = Logger(ProviderClient::class.java.name)

internal class ProviderClient(ctx: Context, dispatcher: CoroutineDispatcher = Dispatchers.IO) :
    UrlsSink {

    @OptIn(ExperimentalCoroutinesApi::class)
    private val scope = CoroutineScope(SupervisorJob() + dispatcher.limitedParallelism(1))
    private val limit = Semaphore(10)
    private val resolver = ctx.contentResolver

    override fun push(url: String) {
        if (limit.tryAcquire()) {
            scope.launch {
                doPush(url)
                limit.release()
            }
        } else {
            LOG.d { "Drop event $url due to busy backend" }
        }
    }

    private fun doPush(msg: String) = runCatching {
        resolver.call(Provider.URI, Provider.METHOD_PUSH, msg, null)
    }.onFailure {
        LOG.w(it) { "Failed to push url" }
    }

}
