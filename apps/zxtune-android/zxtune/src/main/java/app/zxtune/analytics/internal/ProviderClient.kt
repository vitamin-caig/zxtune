package app.zxtune.analytics.internal

import android.content.Context
import androidx.lifecycle.ProcessLifecycleOwner
import androidx.lifecycle.lifecycleScope
import app.zxtune.Logger
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.launch
import kotlin.coroutines.CoroutineContext
import kotlin.coroutines.cancellation.CancellationException

private val LOG = Logger(ProviderClient::class.java.name)

internal class ProviderClient(ctx: Context, dispatcher: CoroutineContext = Dispatchers.IO) :
    UrlsSink {

    private val channel =
        Channel<String>(capacity = 1000, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val resolver = ctx.contentResolver

    init {
        ProcessLifecycleOwner.get().lifecycleScope.launch(dispatcher) {
            LOG.d { "Start analytics client" }
            for (msg in channel) {
                doPush(msg)
            }
        }.invokeOnCompletion {
            if (it == null || it is CancellationException) {
                LOG.d { "Stop analytics client" }
            } else {
                LOG.w(it) { "Terminated analytics client" }
            }
        }
    }

    override fun push(url: String) {
        channel.trySend(url)
    }

    private fun doPush(msg: String) = runCatching {
        resolver.call(Provider.URI, Provider.METHOD_PUSH, msg, null)
    }.onFailure {
        LOG.w(it) { "Failed to push url" }
    }

}
