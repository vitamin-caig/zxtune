package app.zxtune.fs.provider

import android.content.Context
import android.database.ContentObserver
import android.database.Cursor
import android.net.Uri
import app.zxtune.ui.utils.observeChanges
import app.zxtune.ui.utils.query
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.job
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking

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

    suspend fun resolve(uri: Uri, cb: ListingCallback) = fetchListing(Query.resolveUriFor(uri), cb)

    suspend fun list(uri: Uri, cb: ListingCallback) = fetchListing(Query.listingUriFor(uri), cb)

    suspend fun parents(uri: Uri, cb: ParentsCallback) = resolver.query(Query.parentsUriFor(uri)) {
        getParents(it, cb)
        Unit
    }

    suspend fun search(uri: Uri, query: String, cb: ListingCallback) = fetchListing(
        Query.searchUriFor(uri, query), cb
    )

    private suspend fun fetchListing(resolverUri: Uri, cb: ListingCallback) = coroutineScope {
        // onChange called in separate thread while main is blocked in primary call
        val observer = object : ContentObserver(null) {
            override fun onChange(selfChange: Boolean) = runBlocking {
                fetchListingPortion(resolverUri, cb)
                Unit
            }
        }
        resolver.registerContentObserver(resolverUri, false, observer)
        coroutineContext.job.invokeOnCompletion {
            resolver.unregisterContentObserver(observer)
        }
        launch {
            fetchListingPortion(resolverUri, cb)
        }.join()
    }

    private suspend fun fetchListingPortion(resolverUri: Uri, cb: ListingCallback) =
        resolver.query(resolverUri) {
            getListing(it, cb)
        }

    fun observeNotifications(uri: Uri) =
        // For some reason, notifyChange for root uri is not propagated to descendant subscribers
        resolver.observeChanges(Query.notificationUriFor(Uri.EMPTY)).map {
            getNotification(uri)
        }

    suspend fun getNotification(uri: Uri) = resolver.query(Query.notificationUriFor(uri)) {
        getNotification(it)
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
