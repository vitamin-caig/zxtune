package app.zxtune.ui.utils

import android.content.ContentResolver
import android.database.ContentObserver
import android.database.Cursor
import android.net.Uri
import android.os.CancellationSignal
import android.os.OperationCanceledException
import androidx.tracing.trace
import androidx.tracing.traceAsync
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.channels.trySendBlocking
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlin.coroutines.cancellation.CancellationException
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException

fun ContentResolver.observeChanges(
    uri: Uri, notifyForDescendants: Boolean = true
) = callbackFlow {
    traceAsync("Observe $uri", uri.hashCode()) {
        val notify = {
            trySendBlocking(Unit)
            Unit
        }
        val observer = object : ContentObserver(null) {
            override fun onChange(selfChange: Boolean) = notify()
        }
        registerContentObserver(uri, notifyForDescendants, observer)
        notify()
        awaitClose {
            unregisterContentObserver(observer)
        }
    }
}

suspend fun <T> ContentResolver.query(
    uri: Uri,
    projection: Array<String>? = null,
    selection: String? = null,
    selectionArgs: Array<String>? = null,
    sortOrder: String? = null,
    convert: (Cursor) -> T,
) = suspendCancellableCoroutine { cont ->
    val signal = CancellationSignal()
    cont.invokeOnCancellation {
        signal.cancel()
    }
    try {
        val res = trace("Query $uri") {
            query(uri, projection, selection, selectionArgs, sortOrder, signal)?.use {
                convert(it)
            }
        }
        cont.resume(res)
    } catch (e: Throwable) {
        if (e is OperationCanceledException) {
            throw CancellationException(e)
        } else {
            cont.resumeWithException(e)
        }
    }
}
