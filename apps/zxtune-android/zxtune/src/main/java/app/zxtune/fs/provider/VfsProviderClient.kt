package app.zxtune.fs.provider

import android.content.Context
import android.database.Cursor
import android.net.Uri

// TODO: use provider notifications instead of polling
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

    @Throws(Exception::class)
    fun resolve(uri: Uri, cb: ListingCallback) {
        val resolverUri = Query.resolveUriFor(uri)
        while (true) {
            val cursor = queryNotEmpty(resolverUri) ?: break
            cursor.use {
                if (getListing(it, cb)) {
                    return
                }
            }
            waitOrCancel(resolverUri)
        }
    }

    @Throws(Exception::class)
    fun list(uri: Uri, cb: ListingCallback) {
        val resolverUri = Query.listingUriFor(uri)
        while (true) {
            val cursor = queryNotEmpty(resolverUri) ?: break
            cursor.use {
                if (getListing(it, cb)) {
                    return
                }
            }
            waitOrCancel(resolverUri)
        }
    }

    @Throws(Exception::class)
    fun parents(uri: Uri, cb: ParentsCallback) {
        val resolverUri = Query.parentsUriFor(uri)
        while (true) {
            val cursor = queryNotEmpty(resolverUri) ?: break
            cursor.use {
                if (getParents(it, cb)) {
                    return
                }
            }
            waitOrCancel(resolverUri)
        }
    }

    @Throws(Exception::class)
    fun search(uri: Uri, query: String, cb: ListingCallback) {
        val resolverUri = Query.searchUriFor(uri, query)
        while (true) {
            checkForCancel(resolverUri)
            val cursor = query(resolverUri) ?: break
            if (cursor.count != 0) {
                cursor.use {
                    if (!getListing(cursor, cb)) {
                        return
                    }
                }
            } else {
                waitOrCancel(resolverUri)
            }
        }
    }

    private fun query(uri: Uri) = resolver.query(uri, null, null, null, null)

    private fun queryNotEmpty(uri: Uri) = query(uri)?.takeIf { it.count != 0 }

    private fun checkForCancel(resolverUri: Uri) {
        if (Thread.interrupted()) {
            resolver.delete(resolverUri, null, null)
            throw InterruptedException()
        }
    }

    private fun waitOrCancel(resolverUri: Uri) {
        try {
            Thread.sleep(1000)
        } catch (e: InterruptedException) {
            resolver.delete(resolverUri, null, null)
            throw e
        }
    }

    companion object {
        @JvmStatic
        fun getFileUriFor(uri: Uri) = Query.fileUriFor(uri)

        private fun getListing(cursor: Cursor, cb: ListingCallback): Boolean {
            while (cursor.moveToNext()) {
                when (val obj = Schema.Object.parse(cursor)) {
                    is Schema.Listing.Delimiter -> return false
                    is Schema.Listing.Dir -> obj.run {
                        cb.onDir(uri, name, description, icon, hasFeed)
                    }
                    is Schema.Listing.File -> obj.run {
                        cb.onFile(uri, name, description, details, tracks, isCached)
                    }
                    is Schema.Status.Error -> throw Exception(obj.error)
                    is Schema.Status.Progress -> obj.run {
                        cb.onProgress(done, total)
                        return false
                    }
                    else -> Unit
                }
            }
            return true
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
    }
}
