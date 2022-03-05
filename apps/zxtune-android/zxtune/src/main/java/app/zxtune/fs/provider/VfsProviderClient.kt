package app.zxtune.fs.provider

import android.content.Context
import android.database.ContentObserver
import android.database.Cursor
import android.net.Uri
import android.os.CancellationSignal
import android.os.Handler
import android.os.HandlerThread
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
    // TODO: use Schema objects?
    interface StatusCallback {
        fun onProgress(done: Int, total: Int)
    }

    interface ListingCallback : StatusCallback {
        fun onDir(uri: Uri, name: String, description: String, icon: Int?, hasFeed: Boolean)
        fun onFile(
            uri: Uri,
            name: String,
            description: String,
            details: String,
            tracks: Int?,
            cached: Boolean?
        )
    }

    interface ParentsCallback : StatusCallback {
        fun onObject(uri: Uri, name: String, icon: Int?)
    }

    private val resolver = ctx.contentResolver
    private val handler by lazy {
        Handler(HandlerThread("VfsProviderClient").apply { start() }.looper)
    }

    @Throws(Exception::class)
    fun resolve(uri: Uri, cb: ListingCallback, signal: CancellationSignal? = null) =
        queryListing(Query.resolveUriFor(uri), cb, signal)

    private fun queryListing(
        resolverUri: Uri,
        cb: ListingCallback,
        userSignal: CancellationSignal?
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

    fun subscribeForNotifications(
        uri: Uri,
        cb: (Schema.Notifications.Object?) -> Unit
    ): Releaseable = Query.notificationUriFor(uri).let { resolverUri ->
        subscribeForChanges(resolverUri) {
            cb(getNotification(resolverUri))
        }.also {
            cb(getNotification(resolverUri))
        }
    }

    private fun getNotification(resolverUri: Uri) = query(resolverUri)?.use {
        getNotification(it)
    }

    private fun query(uri: Uri, signal: CancellationSignal? = null) =
        resolver.query(uri, null, null, null, null, signal)

    private fun subscribeForChanges(uri: Uri, cb: () -> Unit): Releaseable {
        val observer = object : ContentObserver(handler) {
            override fun onChange(selfChange: Boolean) = cb()
        }
        resolver.registerContentObserver(uri, false, observer)
        return Releaseable { resolver.unregisterContentObserver(observer) }
    }

    companion object {
        @JvmStatic
        fun getFileUriFor(uri: Uri) = Query.fileUriFor(uri)

        private fun getListing(cursor: Cursor, cb: ListingCallback) {
            while (cursor.moveToNext()) {
                when (val obj = Schema.Object.parse(cursor)) {
                    is Schema.Listing.Dir -> obj.run {
                        cb.onDir(uri, name, description, icon, hasFeed)
                    }
                    is Schema.Listing.File -> obj.run {
                        cb.onFile(uri, name, description, details, tracks, isCached)
                    }
                    is Schema.Status.Error -> throw Exception(obj.error)
                    is Schema.Status.Progress -> obj.run {
                        cb.onProgress(done, total)
                    }
                    else -> Unit
                }
            }
        }

        private fun getParents(cursor: Cursor, cb: ParentsCallback): Boolean {
            while (cursor.moveToNext()) {
                when (val obj = Schema.Parents.Object.parse(cursor)) {
                    is Schema.Status.Error -> throw Exception(obj.error)
                    is Schema.Parents.Object -> obj.run { cb.onObject(uri, name, icon) }
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
