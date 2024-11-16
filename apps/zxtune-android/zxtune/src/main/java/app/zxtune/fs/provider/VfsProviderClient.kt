package app.zxtune.fs.provider

import android.content.Context
import android.database.ContentObserver
import android.database.Cursor
import android.net.Uri
import android.os.CancellationSignal
import app.zxtune.Releaseable
import app.zxtune.use

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

    @Throws(Exception::class)
    fun resolve(uri: Uri, cb: ListingCallback, signal: CancellationSignal? = null) =
        queryListing(Query.resolveUriFor(uri), cb, signal)

    private fun queryListing(
        resolverUri: Uri, cb: ListingCallback, userSignal: CancellationSignal?
    ) {
        val signal = userSignal ?: CancellationSignal()
        val notification = subscribeForChanges(resolverUri) {
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

    @Throws(Exception::class)
    fun list(uri: Uri, cb: ListingCallback, signal: CancellationSignal? = null) =
        queryListing(Query.listingUriFor(uri), cb, signal)

    @Throws(Exception::class)
    fun parents(uri: Uri, cb: ParentsCallback) {
        val resolverUri = Query.parentsUriFor(uri)
        query(resolverUri)?.use {
            getParents(it, cb)
        }
    }

    @Throws(Exception::class)
    fun search(uri: Uri, query: String, cb: ListingCallback, signal: CancellationSignal? = null) =
        queryListing(Query.searchUriFor(uri, query), cb, signal)

    // Notifications are propagated down-up, but since they sent to the root notifications
    // url, subscribe to it
    fun subscribeForNotifications(
        uri: Uri, cb: (Schema.Notifications.Object?) -> Unit
    ) = subscribeForChanges(Query.notificationUriFor(Uri.EMPTY)) {
        cb(getNotification(uri))
    }.also {
        cb(getNotification(uri))
    }

    fun getNotification(uri: Uri) = query(Query.notificationUriFor(uri))?.use {
        getNotification(it)
    }

    private fun query(uri: Uri, signal: CancellationSignal? = null) =
        resolver.query(uri, null, null, null, null, signal)

    private fun subscribeForChanges(uri: Uri, cb: () -> Unit): Releaseable {
        val observer = object : ContentObserver(null) {
            override fun onChange(selfChange: Boolean) = cb()
        }
        resolver.registerContentObserver(uri, false, observer)
        return Releaseable { resolver.unregisterContentObserver(observer) }
    }

    companion object {
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
