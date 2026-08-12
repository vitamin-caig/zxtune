package app.zxtune.utils

import android.content.ContentResolver
import android.content.ContentValues
import android.database.ContentObserver
import android.database.Cursor
import android.net.Uri
import android.os.Bundle
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

@JvmInline
@JvmExposeBoxed
@OptIn(ExperimentalStdlibApi::class)
value class ContentUri(val raw: Uri) {
    override fun toString() = raw.toString()

    operator fun get(pathIndex: Int) = raw.pathSegments.getOrNull(pathIndex)

    operator fun get(paramName: String) = raw.getQueryParameter(paramName)

    companion object {
        fun from(uri: Uri) = ContentUri(uri)
    }
}

fun ContentResolver.observeChanges(
    uri: ContentUri, notifyForDescendants: Boolean = true
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

fun ContentResolver.registerContentObserver(
    uri: ContentUri, notifyForDescendants: Boolean, observer: ContentObserver
) = registerContentObserver(uri.raw, notifyForDescendants, observer)

suspend fun <T> ContentResolver.query(
    uri: ContentUri,
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
            query(uri.raw, projection, selection, selectionArgs, sortOrder, signal)?.use {
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

fun ContentResolver.notifyChange(uri: ContentUri, observer: ContentObserver?) =
    notifyChange(uri.raw, observer)

fun ContentResolver.insert(uri: ContentUri, values: ContentValues?) = insert(uri.raw, values)

fun ContentResolver.delete(
    uri: ContentUri, where: String? = null, selectionArgs: Array<String>? = null
) = delete(uri.raw, where, selectionArgs)

fun ContentResolver.call(
    uri: ContentUri, method: String, arg: String? = null, extras: Bundle? = null
) = call(uri.raw, method, arg, extras)

fun Cursor.setNotificationUri(resolver: ContentResolver, uri: ContentUri) =
    setNotificationUri(resolver, uri.raw)
