package app.zxtune.fs.provider

import android.content.ContentProvider
import android.content.ContentValues
import android.net.Uri
import android.os.CancellationSignal
import android.os.ParcelFileDescriptor
import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import app.zxtune.MainApplication
import java.io.FileOutputStream
import java.util.concurrent.ConcurrentHashMap

class Provider @VisibleForTesting internal constructor(
    private val resolver: Resolver,
    private val schema: SchemaSource
) : ContentProvider() {
    private val operations = ConcurrentHashMap<Uri, Operation>()

    // should be initialized in main thread
    private lateinit var notifications: NotificationsSource

    constructor() : this(CachingResolver(cacheSize = 10), SchemaSourceImplementation())

    override fun onCreate() = context?.run {
        MainApplication.initialize(applicationContext)
        notifications = NotificationsSource(this)
        true
    } ?: false

    override fun query(
        uri: Uri,
        projection: Array<String>?,
        selection: String?,
        selectionArgs: Array<String>?,
        sortOrder: String?
    ) = query(uri, projection, selection, selectionArgs, sortOrder, null)

    override fun query(
        uri: Uri,
        projection: Array<String>?,
        selection: String?,
        selectionArgs: Array<String>?,
        sortOrder: String?,
        signal: CancellationSignal?
    ) = operations[uri]?.status() ?: query(uri, projection, signal)

    private fun query(uri: Uri, projection: Array<String>?, signal: CancellationSignal?) =
        runCatching {
            if (Query.TYPE_NOTIFICATION == Query.getUriType(uri)) {
                queryNotification(uri)
            } else {
                val op = createOperation(uri, projection, makeCallback(uri, signal))
                Operation(uri, op).run()
            }
        }.recover(StatusBuilder::makeError).getOrNull()

    private fun queryNotification(uri: Uri) = resolver.resolve(Query.getPathFrom(uri))?.let {
        notifications.getFor(it)
    }

    private fun makeCallback(uri: Uri, signal: CancellationSignal?): AsyncQueryOperation.Callback =
        object : AsyncQueryOperation.Callback {
            init {
                signal?.setOnCancelListener {
                    LOG.d { "Canceled query for $uri" }
                    operations[uri]?.cancel()
                }
            }

            override fun checkForCancel() {
                signal?.throwIfCanceled()
            }

            override fun onStatusChanged() {
                context?.contentResolver?.notifyChange(uri, null)
            }
        }

    private inner class Operation(val uri: Uri, val op: AsyncQueryOperation) {
        private val thread = Thread.currentThread()

        fun run() = try {
            operations[uri] = this
            op.call()
        } finally {
            operations.remove(uri)
        }

        fun status() = op.status()

        fun cancel() = thread.interrupt()
    }

    private fun createOperation(
        uri: Uri,
        projection: Array<String>?,
        callback: AsyncQueryOperation.Callback
    ): AsyncQueryOperation {
        val path = Query.getPathFrom(uri)
        return when (Query.getUriType(uri)) {
            Query.TYPE_RESOLVE -> ResolveOperation(path, resolver, schema, callback)
            Query.TYPE_LISTING -> ListingOperation(path, resolver, schema, callback)
            Query.TYPE_PARENTS -> ParentsOperation(path, resolver, schema)
            Query.TYPE_SEARCH -> SearchOperation(
                path,
                resolver,
                schema,
                callback,
                Query.getQueryFrom(uri)
            )

            Query.TYPE_FILE -> FileOperation(path, Query.getSizeFrom(uri), resolver, projection)
            else -> throw UnsupportedOperationException("Unsupported uri $uri")
        }
    }

    override fun getType(uri: Uri) = Query.mimeTypeOf(uri)

    override fun insert(uri: Uri, values: ContentValues?): Uri? = null

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<String>?) = 0

    override fun update(
        uri: Uri,
        values: ContentValues?,
        selection: String?,
        selectionArgs: Array<String>?
    ): Int = 0

    override fun openFile(uri: Uri, mode: String): ParcelFileDescriptor {
        require(Query.getUriType(uri) == Query.TYPE_FILE)
        require("r" == mode) { "Invalid mode: $mode" }
        val path = Query.getPathFrom(uri)
        val size = Query.getSizeFrom(uri)
        return openPipeHelper(
            path,
            "application/octet",
            null,
            null
        ) { out, _, _, _, _ ->
            runCatching {
                FileOutputStream(out.fileDescriptor).use {
                    FileOperation(path, size, resolver, null).consumeContent(it.channel)
                }
            }.onFailure {
                LOG.w(it) { "Failed to open file for $uri" }
            }
        }
    }

    companion object {
        private val LOG = Logger(Provider::class.java.name)
    }
}
