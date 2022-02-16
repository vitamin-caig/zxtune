package app.zxtune.fs.provider

import android.content.ContentProvider
import android.content.ContentValues
import android.net.Uri
import android.os.Handler
import android.os.Looper
import android.os.OperationCanceledException
import android.os.ParcelFileDescriptor
import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import app.zxtune.MainApplication
import java.io.FileNotFoundException
import java.util.concurrent.*

class Provider @VisibleForTesting internal constructor(
    private val resolver: Resolver,
    private val schema: SchemaSource
) : ContentProvider() {
    private val executor = Executors.newCachedThreadPool()
    private val operations = ConcurrentHashMap<Uri, OperationHolder>()
    private val handler = Handler(Looper.getMainLooper())

    constructor() : this(CachingResolver(cacheSize = 10), SchemaSourceImplementation())

    override fun onCreate() = context?.run {
        MainApplication.initialize(applicationContext)
        true
    } ?: false

    override fun query(
        uri: Uri,
        projection: Array<String>?,
        selection: String?,
        selectionArgs: Array<String>?,
        sortOrder: String?
    ) = runCatching {
        val existing = operations[uri]
        if (existing != null) {
            existing.status()
        } else {
            val op = createOperation(uri, projection, makeCallback(uri))
            OperationHolder(uri, op).start()
        }
    }.recover(StatusBuilder::makeError).getOrNull()

    private fun makeCallback(uri: Uri): AsyncQueryOperation.Callback = object :
        AsyncQueryOperation.Callback {
        override fun checkForCancel() {
            if (Thread.interrupted()) {
                LOG.d { "Interrupted query $uri" }
                throw OperationCanceledException()
            }
        }
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
            Query.TYPE_FILE -> FileOperation(path, resolver, projection)
            else -> throw UnsupportedOperationException("Unsupported uri $uri")
        }
    }

    override fun getType(uri: Uri) = Query.mimeTypeOf(uri)

    override fun insert(uri: Uri, values: ContentValues?): Uri? = null

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<String>?): Int =
        operations.remove(uri)?.run {
            cancel()
            1
        } ?: 0

    override fun update(
        uri: Uri,
        values: ContentValues?,
        selection: String?,
        selectionArgs: Array<String>?
    ): Int = 0

    override fun openFile(uri: Uri, mode: String): ParcelFileDescriptor? {
        try {
            if (Query.getUriType(uri) == Query.TYPE_FILE) {
                return FileOperation(Query.getPathFrom(uri), resolver, null).openFile(mode)
            }
        } catch (e: Exception) {
            LOG.w(e) { "Failed to open file $uri" }
        }
        throw FileNotFoundException(uri.toString())
    }

    private fun notifyUpdate(uri: Uri) = context?.contentResolver?.notifyChange(uri, null) ?: Unit

    private inner class OperationHolder constructor(
        private val uri: Uri,
        private val op: AsyncQueryOperation
    ) {
        private val task = FutureTask(op)
        private val update = Runnable {
            notifyUpdate(uri)
            scheduleUpdate()
        }

        fun start() = try {
            executor.execute(task)
            task[1, TimeUnit.SECONDS]
        } catch (e: TimeoutException) {
            schedule()
            op.status()
        }

        fun status() = if (task.isDone) {
            unschedule()
            notifyUpdate(uri)
            task.get()
        } else {
            op.status()
        }

        fun cancel() {
            task.cancel(true)
            unschedule()
        }

        private fun schedule() {
            operations[uri] = this
            scheduleUpdate()
        }

        private fun unschedule() {
            handler.removeCallbacks(update)
            operations.remove(uri)
        }

        private fun scheduleUpdate() {
            handler.postDelayed(update, 1000)
        }
    }

    companion object {
        private val LOG = Logger(Provider::class.java.name)
    }
}
