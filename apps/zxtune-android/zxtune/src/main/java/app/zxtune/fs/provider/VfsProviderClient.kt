package app.zxtune.fs.provider

import android.content.Context
import android.database.ContentObserver
import android.database.Cursor
import android.net.Uri
import android.os.CancellationSignal
import androidx.tracing.trace
import app.zxtune.Logger
import app.zxtune.Releaseable
import app.zxtune.use
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.channels.trySendBlocking
import kotlinx.coroutines.flow.callbackFlow

/**
 * Threads scheme
 *
 *  User            Binder1           VfsProviderClient    Binder2
 *
 *  queryListing -> Provider.query -> Observer.onChange -> Provider.query
 *                    |                                     |
 *  Callback.result <-+               Callback.onProgress <-+
 *                                    (may cancel query)
 *
 */
class VfsProviderClient(ctx: Context) {
    interface ListingCallback {
        fun onProgress(status: Schema.Status.Progress)
        fun onDir(dir: Schema.Listing.Dir)
        fun onFile(file: Schema.Listing.File)
    }

    interface ParentsCallback {
        fun onObject(obj: Schema.Parents.Object)
    }

    private val resolver = ctx.contentResolver

    fun resolve(uri: Uri, cb: ListingCallback, signal: CancellationSignal? = null) =
        trace("Browser.resolve($uri)") {
            queryListing(Query.resolveUriFor(uri), cb, signal)
        }

    private fun queryListing(
        resolverUri: Uri, cb: ListingCallback, userSignal: CancellationSignal?
    ) {
        val signal = userSignal ?: CancellationSignal()
        val notification = subscribeForChanges(resolverUri, false) {
            query(resolverUri)?.use {
                runCatching { getListing(it, cb) }.onFailure {
                    signal.cancel()
                    // Do not throw anything!!!
                }
            }
        }
        notification.use {
            query(resolverUri, signal)?.use {
                getListing(it, cb)
            }
        }
    }

    fun list(uri: Uri, cb: ListingCallback, signal: CancellationSignal? = null) =
        trace("Browser.list($uri)") {
            queryListing(Query.listingUriFor(uri), cb, signal)
        }

    fun parents(uri: Uri, cb: ParentsCallback) = trace("Browser.parents($uri)") {
        val resolverUri = Query.parentsUriFor(uri)
        query(resolverUri)?.use {
            getParents(it, cb)
        }
        Unit
    }

    fun search(uri: Uri, query: String, cb: ListingCallback, signal: CancellationSignal? = null) =
        trace("Browser.search($uri, $query)") {
            queryListing(Query.searchUriFor(uri, query), cb, signal)
        }

    fun observeNotifications(uri: Uri) = callbackFlow {
        val resolverUri = Query.notificationUriFor(uri)
        val subscription = subscribeForChanges(resolverUri, true) {
            // Send updates as is
            trySendBlocking(getNotification(resolverUri))
        }
        // Send first only if there's notification
        getNotification(resolverUri)?.let {
            trySendBlocking(it)
        }
        awaitClose {
            subscription.release()
        }
    }

    private fun getNotification(resolverUri: Uri) = query(resolverUri)?.use {
        getNotification(it)
    }

    private fun query(uri: Uri, signal: CancellationSignal? = null) =
        resolver.query(uri, null, null, null, null, signal)

    private fun subscribeForChanges(
        uri: Uri, notifyForDescendants: Boolean, cb: () -> Unit
    ): Releaseable {
        val observer = object : ContentObserver(null) {
            override fun onChange(selfChange: Boolean) = cb()
        }
        LOG.d { "Subscribe to $uri" }
        resolver.registerContentObserver(uri, notifyForDescendants, observer)
        return Releaseable {
            LOG.d { "Unsubscribe from $uri" }
            resolver.unregisterContentObserver(observer)
        }
    }

    companion object {
        private val LOG = Logger(VfsProviderClient::class.java.name)

        @JvmStatic
        fun getFileUriFor(uri: Uri, size: Long) = Query.fileUriFor(uri, size)

        private fun getListing(cursor: Cursor, cb: ListingCallback) {
            while (cursor.moveToNext()) {
                when (val obj = Schema.Object.parse(cursor)) {
                    is Schema.Listing.Dir -> cb.onDir(obj)
                    is Schema.Listing.File -> cb.onFile(obj)
                    is Schema.Status.Error -> throw Exception(obj.error)
                    is Schema.Status.Progress -> cb.onProgress(obj)
                    else -> Unit
                }
            }
        }

        private fun getParents(cursor: Cursor, cb: ParentsCallback): Boolean {
            while (cursor.moveToNext()) {
                when (val obj = Schema.Parents.Object.parse(cursor)) {
                    is Schema.Status.Error -> throw Exception(obj.error)
                    is Schema.Parents.Object -> cb.onObject(obj)
                    else -> return false
                }
            }
            return true
        }

        private fun getNotification(cursor: Cursor) = if (cursor.moveToNext()) {
            when (val obj = Schema.Notifications.Object.parse(cursor)) {
                is Schema.Status.Error -> throw Exception(obj.error)
                is Schema.Notifications.Object -> obj
                else -> null
            }
        } else {
            null
        }
    }
}
