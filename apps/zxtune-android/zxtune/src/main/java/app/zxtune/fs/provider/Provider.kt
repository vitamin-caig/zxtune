package app.zxtune.fs.provider

import android.content.ContentProvider
import android.content.ContentValues
import android.net.Uri
import android.os.CancellationSignal
import android.os.ParcelFileDescriptor
import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import app.zxtune.MainApplication
import app.zxtune.fs.feed
import app.zxtune.utils.ContentUri
import app.zxtune.utils.notifyChange
import app.zxtune.utils.openOutputPipe
import kotlinx.coroutines.Dispatchers
import java.util.concurrent.ConcurrentHashMap

class Provider @VisibleForTesting internal constructor(
    private val resolver: Resolver, private val schema: SchemaSource
) : ContentProvider() {
    private val operations = ConcurrentHashMap<ContentUri, Operation>()

    // should be initialized in main thread
    private lateinit var notifications: NotificationsSource

    constructor() : this(CachingResolver(cacheSize = 10), SchemaSource())

    override fun onCreate() = context?.run {
        MainApplication.initialize(applicationContext)
        notifications = NotificationsSource(this)
        true
    } ?: false

    override fun shutdown() {
        notifications.shutdown()
        super.shutdown()
    }

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
    ) = Query.parse(uri).let {
        operations[it.providerUri]?.status() ?: query(it, projection, signal)
    }

    private fun query(query: Query, projection: Array<String>?, signal: CancellationSignal?) =
        runCatching {
            when (query.type) {
                Query.Type.NOTIFICATION -> queryNotification(query)
                Query.Type.FEED -> queryFeed(query)
                else -> {
                    val op =
                        createOperation(query, projection, makeCallback(query.providerUri, signal))
                    Operation(query.providerUri, op).run()
                }
            }
        }.recover(StatusBuilder::makeError).getOrNull()

    private fun queryNotification(query: Query) = resolver.resolve(query.path)?.let {
        notifications.getFor(it)
    }

    @Suppress("UNCHECKED_CAST")
    private fun queryFeed(query: Query) = resolver.resolve(query.path)?.feed?.let { feed ->
        if (feed.hasNext()) {
            ListingCursorBuilder().apply {
                addFile(feed.next())
            }.getResult(schema)
        } else {
            null
        }
    }

    private fun makeCallback(
        uri: ContentUri,
        signal: CancellationSignal?
    ): AsyncQueryOperation.Callback =
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

    private inner class Operation(val uri: ContentUri, val op: AsyncQueryOperation) {
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
        query: Query, projection: Array<String>?, callback: AsyncQueryOperation.Callback
    ): AsyncQueryOperation {
        return when (query.type) {
            Query.Type.RESOLVE -> ResolveOperation(query, resolver, schema, callback)
            Query.Type.LISTING -> ListingOperation(query, resolver, schema, callback)
            Query.Type.SEARCH -> SearchOperation(query, resolver, schema, callback)

            Query.Type.FILE -> FileOperation(query, resolver, projection)
            else -> throw UnsupportedOperationException("Unsupported uri ${query.providerUri}")
        }
    }

    override fun getType(uri: Uri) = Query.parse(uri).type?.mime

    override fun insert(uri: Uri, values: ContentValues?): Uri? = null

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<String>?) = 0

    override fun update(
        uri: Uri, values: ContentValues?, selection: String?, selectionArgs: Array<String>?
    ): Int = 0

    override fun openFile(uri: Uri, mode: String): ParcelFileDescriptor {
        val query = Query.parse(uri)
        require(query.type == Query.Type.FILE)
        require("r" == mode) { "Invalid mode: $mode" }
        return openOutputPipe(Dispatchers.IO) {
            FileOperation(query, resolver, null).consumeContent(it)
        }
    }

    companion object {
        private val LOG = Logger(Provider::class.java.name)
    }
}
